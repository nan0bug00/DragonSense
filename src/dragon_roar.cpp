// Detects a dragon's flight roar and hands it to SkyrimNet as an event.
//
// What the probe runs established, and why this file is shaped the way it is:
//   - the engine resolves a sound record through BGSSoundDescriptorForm's
//     BSISoundDescriptor interface (slot 1 of vtable [1]), and the Resolution
//     carries the record's own FormID, so no object dereference is needed;
//   - records are matched by FormID, which an override does not change;
//   - resolution happens on a worker thread, never the game thread, so the
//     observation is queued and the world is only touched from a task;
//   - one roar resolves *both* a near and a distant record, so the variant cannot
//     decide distance. Distance is measured to the nearest live dragon instead.
//
// Nothing here writes to the game: it reads a sound record's identity off a hook
// and asks SkyrimNet to register an event.

#include "dragon_roar.h"

#include "skyrimnet_bridge.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <random>
#include <string_view>
#include <vector>

namespace
{
    constexpr RE::FormID kDragonKeyword = 0x035D59;  // ActorTypeDragon
    constexpr float      kVeryCloseDistance = 2000.0f;  // near even out of combat
    constexpr auto       kNearCooldown = std::chrono::seconds(60);
    constexpr auto       kDistantCooldown = std::chrono::seconds(60);

    constexpr std::size_t kSampleLimit = 25;
    constexpr std::size_t kCensusEvery = 500;

    enum class RoarClass
    {
        kNear,
        kDistant
    };

    struct RoarEntry
    {
        RE::FormID  formID;
        std::string_view name;
        RoarClass   roarClass;
    };

    // Skyrim.esm records for the dragon flight roar, plus the markers that point
    // at them. Kill and resurrection roars are deliberately absent.
    constexpr RoarEntry kRoarTable[] = {
        { 0x03F35F, "NPCDragonFlightRoarSD"sv, RoarClass::kNear },
        { 0x0A7EA8, "NPCDragonFlightRoarObstructedSD"sv, RoarClass::kNear },
        { 0x0172A4, "NPCDragonFlightRoarPaarthurnax"sv, RoarClass::kNear },
        { 0x0FF7C9, "NPCDragonFlightRoarDistant01SD"sv, RoarClass::kDistant },
        { 0x0FF7C8, "NPCDragonFlightRoarDistant02SD"sv, RoarClass::kDistant },
        { 0x0172A2, "NPCDragonFlightRoarDistantBSD"sv, RoarClass::kDistant },
        { 0x03CB88, "NPCDragonFlightRoar (marker)"sv, RoarClass::kNear },
        { 0x0A7EA9, "NPCDragonFlightRoarObstructed (marker)"sv, RoarClass::kNear },
        { 0x0FF7CA, "NPCDragonFlightRoarDistant01 (marker)"sv, RoarClass::kDistant },
        { 0x0FF7CB, "NPCDragonFlightRoarDistant02 (marker)"sv, RoarClass::kDistant },
    };

    struct Observation
    {
        RE::FormID formID;
        RoarClass  variantClass;
    };

    std::vector<Observation>     g_pending;
    std::mutex                   g_pendingMutex;
    std::atomic<bool>            g_episodeScheduled{ false };
    std::atomic<std::uint32_t>   g_resolveCount{ 0 };
    std::atomic<std::uint32_t>   g_samplesLogged{ 0 };
    std::atomic<std::uint32_t>   g_roarCount{ 0 };
    std::mutex                   g_logMutex;
    std::uint32_t                g_mainThreadId = 0;

    std::chrono::steady_clock::time_point g_lastNear{};
    std::chrono::steady_clock::time_point g_lastDistant{};

    const char* ClassName(RoarClass a_class)
    {
        return a_class == RoarClass::kDistant ? "distant" : "near";
    }

    // Half the roars are remembered as a stored event, half provoke a reaction.
    bool CoinFlip()
    {
        static std::mt19937               engine{ std::random_device{}() };
        static std::bernoulli_distribution dist{ 0.5 };
        return dist(engine);
    }

    const RoarEntry* FindRoar(RE::FormID a_formID)
    {
        for (const auto& entry : kRoarTable) {
            if (entry.formID == a_formID) {
                return &entry;
            }
        }
        return nullptr;
    }

    // Nearest live dragon, preferring one that is actually fighting. The fight is
    // the signal for "near": a dragon circling at distance to make a pass is still
    // the fight in front of you, while a roar across the valley is not.
    // Game thread only: it walks the process lists.
    struct DragonProbe
    {
        float distance{ -1.0f };
        bool  inCombat{ false };
    };

    DragonProbe ProbeDragons()
    {
        auto* player = RE::PlayerCharacter::GetSingleton();
        auto* lists = RE::ProcessLists::GetSingleton();
        if (!player || !lists) {
            return {};
        }

        auto* keyword = RE::TESForm::LookupByID<RE::BGSKeyword>(kDragonKeyword);
        if (!keyword) {
            return {};
        }

        const auto playerPos = player->GetPosition();

        float nearestAny = -1.0f;
        float nearestFighting = -1.0f;

        for (const auto& handle : lists->highActorHandles) {
            auto actor = handle.get();
            if (!actor || actor->IsDead()) {
                continue;
            }
            auto* race = actor->GetRace();
            if (!race || !race->HasKeyword(keyword)) {
                continue;
            }

            const float distance = playerPos.GetDistance(actor->GetPosition());

            if (nearestAny < 0.0f || distance < nearestAny) {
                nearestAny = distance;
            }
            if (actor->IsInCombat() && (nearestFighting < 0.0f || distance < nearestFighting)) {
                nearestFighting = distance;
            }
        }

        if (nearestFighting >= 0.0f) {
            return { nearestFighting, true };
        }
        return { nearestAny, false };
    }

    void EmitEpisode(RoarClass a_class, float a_distance)
    {
        const bool isNear = a_class == RoarClass::kNear;

        auto& last = isNear ? g_lastNear : g_lastDistant;
        const auto cooldown = isNear ? kNearCooldown : kDistantCooldown;

        const auto now = std::chrono::steady_clock::now();
        if (last.time_since_epoch().count() != 0 && now - last < cooldown) {
            const auto remaining =
                std::chrono::duration_cast<std::chrono::seconds>(cooldown - (now - last)).count();
            logger::info("episode [{}]: suppressed, {}s of cooldown left", ClassName(a_class), remaining);
            return;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            logger::warn("episode [{}]: no player, dropping", ClassName(a_class));
            return;
        }

        const char* kind = isNear ? "roar_near" : "roar_distant";
        const char* mode = CoinFlip() ? "narrate" : "remember";

        char payload[96];
        std::snprintf(payload, sizeof(payload), R"({"kind":"%s","mode":"%s"})", kind, mode);

        if (!SkyrimNetBridge::RegisterTypedEvent("dragonsense_event", payload, player->GetFormID())) {
            logger::warn("episode [{}]: SkyrimNet refused the event (memory ready={})",
                         ClassName(a_class),
                         SkyrimNetBridge::MemorySystemReady());
            return;
        }

        last = now;
        logger::info("episode [{}]: raised mode={} (nearest dragon was {})",
                     ClassName(a_class),
                     mode,
                     a_distance >= 0.0f ? std::to_string(static_cast<int>(a_distance)) + " units" : "not found");
    }

    // Runs on the game thread: this is the only place that touches the world.
    void ProcessEpisode()
    {
        g_episodeScheduled.store(false);

        std::vector<Observation> observations;
        {
            std::scoped_lock lock{ g_pendingMutex };
            observations.swap(g_pending);
        }

        if (observations.empty()) {
            return;
        }

        if (!SkyrimNetBridge::MemorySystemReady()) {
            logger::info("episode: {} roar(s) dropped, no save loaded yet", observations.size());
            return;
        }

        const auto dragon = ProbeDragons();

        // The variant cannot decide this: a single roar resolves the near and the
        // distant record both. It is only a fallback for when no dragon is found.
        bool sawNearVariant = false;
        for (const auto& observation : observations) {
            if (observation.variantClass == RoarClass::kNear) {
                sawNearVariant = true;
            }
        }

        RoarClass roarClass;
        if (dragon.distance >= 0.0f) {
            // A dragon in combat is always near: it is the fight in front of you,
            // wherever it happens to be circling at that moment.
            const bool veryClose = dragon.distance <= kVeryCloseDistance;
            roarClass = (dragon.inCombat || veryClose) ? RoarClass::kNear : RoarClass::kDistant;
        } else {
            roarClass = sawNearVariant ? RoarClass::kNear : RoarClass::kDistant;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        const bool playerFighting = player && player->IsInCombat();

        logger::info("episode: {} roar resolve(s), nearest dragon {}, in combat={}, player in combat={}, classified [{}]",
                     observations.size(),
                     dragon.distance >= 0.0f
                         ? std::to_string(static_cast<int>(dragon.distance)) + " units"
                         : "not found"s,
                     dragon.inCombat,
                     playerFighting,
                     ClassName(roarClass));

        EmitEpisode(roarClass, dragon.distance);
    }

    void ScheduleEpisode()
    {
        if (g_episodeScheduled.exchange(true)) {
            return;
        }

        const auto* tasks = SKSE::GetTaskInterface();
        if (!tasks) {
            g_episodeScheduled.store(false);
            return;
        }

        tasks->AddTask([] {
            ProcessEpisode();
        });
    }

    void OnResolve(const RE::BSISoundDescriptor::Resolution& a_resolution)
    {
        const auto count = ++g_resolveCount;
        const auto* entry = FindRoar(a_resolution.formID);

        if (entry) {
            const auto roarNumber = ++g_roarCount;
            {
                std::scoped_lock lock{ g_logMutex };
                logger::info("ROAR[{}] #{} {:<38} form={:08X} thread={} main={}",
                             ClassName(entry->roarClass),
                             roarNumber,
                             entry->name,
                             a_resolution.formID,
                             ::GetCurrentThreadId(),
                             ::GetCurrentThreadId() == g_mainThreadId);
            }

            {
                std::scoped_lock lock{ g_pendingMutex };
                g_pending.push_back(Observation{ a_resolution.formID, entry->roarClass });
            }
            ScheduleEpisode();
            return;
        }

        bool logSample = g_samplesLogged.load() < kSampleLimit;
        if (logSample) {
            g_samplesLogged.fetch_add(1);
        } else if (count % kCensusEvery == 0) {
            logSample = true;
        }

        if (!logSample) {
            return;
        }

        std::scoped_lock lock{ g_logMutex };
        logger::info("resolve #{:<7} form={:08X} thread={} (not a roar record)",
                     count,
                     a_resolution.formID,
                     ::GetCurrentThreadId());
    }

    struct HookResolve
    {
        // `a_this` is opaque: the call is chained with exactly the pointer it
        // arrived with, and identity comes from the Resolution.
        static bool thunk(void* a_this, RE::BSISoundDescriptor::Resolution& a_resolution)
        {
            const bool result = func(a_this, a_resolution);
            OnResolve(a_resolution);
            return result;
        }

        static inline REL::Relocation<decltype(thunk)> func;
    };

    void InstallHook()
    {
        static REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_BGSSoundDescriptorForm[1] };
        HookResolve::func = vtable.write_vfunc(1, HookResolve::thunk);
        logger::info("sound record hook installed (slot 1 of VTABLE_BGSSoundDescriptorForm[1])");
    }

    void LogTable()
    {
        std::size_t present = 0;
        for (const auto& entry : kRoarTable) {
            const bool exists = RE::TESForm::LookupByID(entry.formID) != nullptr;
            if (exists) {
                ++present;
            }
            logger::info("watch [{:<7}] {:<38} form={:08X} -> {}",
                         ClassName(entry.roarClass),
                         entry.name,
                         entry.formID,
                         exists ? "present" : "ABSENT");
        }
        logger::info("roar table: {} records, {} present", std::size(kRoarTable), present);
    }
}

namespace DragonRoar
{
    void OnDataLoaded()
    {
        g_mainThreadId = ::GetCurrentThreadId();
        logger::info("main thread id = {}", g_mainThreadId);

        LogTable();
        InstallHook();

        logger::info("dragon roar detection armed");
    }
}
