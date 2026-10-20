#include "dragon_roar.h"
#include "skyrimnet_bridge.h"
#include "world_events.h"

#include <mutex>

namespace
{
    constexpr auto kPluginName = "DragonSense"sv;
    constexpr auto kPluginVersion = "0.2.0"sv;

    std::once_flag g_roarOnce;

    void InitializeLog()
    {
        auto path = SKSE::log::log_directory();
        if (!path) {
            SKSE::stl::report_and_fail("Failed to find standard logging directory"sv);
        }

        *path /= std::format("{}.log", kPluginName);

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
        auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

        log->set_level(spdlog::level::info);
        log->flush_on(spdlog::level::info);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%H:%M:%S:%e] %v"s);
    }

    void ProbeSkyrimNet(std::string_view a_phase)
    {
        logger::info("--- SkyrimNet probe: {} ---", a_phase);

        const auto diagnostics = SkyrimNetBridge::Resolve();
        if (!diagnostics.resolved) {
            logger::warn("probe: SkyrimNet.dll not found");
            return;
        }

        logger::info("probe: API v{}", diagnostics.version);
        logger::info("probe: registerEvent={} addWorldKnowledge={} memoryReadyFn={} formToUuid={} actorName={}",
                     diagnostics.registerEvent,
                     diagnostics.addWorldKnowledge,
                     diagnostics.memoryReadyFn,
                     diagnostics.formToUuid,
                     diagnostics.actorName);
        logger::info("probe: memory system ready = {}", SkyrimNetBridge::MemorySystemReady());

        std::uint64_t uuid = 0;
        const auto    name = SkyrimNetBridge::ActorNameForForm(0x14, &uuid);
        logger::info("probe: player uuid = {} name = '{}'", uuid, name);
    }

    void OnMessage(SKSE::MessagingInterface::Message* a_msg)
    {
        if (!a_msg) {
            return;
        }

        // Forms exist here, but no save is loaded yet.
        if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
            ProbeSkyrimNet("kDataLoaded");

            std::call_once(g_roarOnce, [] {
                DragonRoar::OnDataLoaded();
                WorldEvents::OnDataLoaded();
            });
        }

        // SkyrimNet keeps event schemas in its DLL, not in the save, so they are
        // lost on every load and have to be re-registered. This message arrives on
        // the main thread with the Papyrus VM up, which is what the dispatch needs;
        // at kDataLoaded the SkyrimNetApi script is not bound yet.
        if (a_msg->type == SKSE::MessagingInterface::kNewGame ||
            a_msg->type == SKSE::MessagingInterface::kPostLoadGame) {
            ProbeSkyrimNet("save loaded");

            const bool schema = SkyrimNetBridge::RegisterEventSchema(
                "dragonsense_event",
                "DragonSense Event",
                "A dragon roar or other world announcement, raised by the DragonSense plugin.",
                R"([{"name":"kind","type":0,"required":true,"description":"roar_near, roar_distant or greybeard_call."},)"
                R"({"name":"mode","type":0,"required":true,"description":"narrate or remember."}])",
                R"({"recent_events":"Dragon event: {{kind}}","raw":"{{kind}}",)"
                R"("compact":"{{kind}}","verbose":"Dragon event: {{kind}}"})");
            logger::info("event schema registration dispatched = {}", schema);
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
    InitializeLog();

    logger::info("{} v{} loading", kPluginName, kPluginVersion);

    SKSE::Init(a_skse, false);  // false: we installed our own logger above

    if (a_skse->IsEditor()) {
        logger::error("refusing to load in the Creation Kit");
        return false;
    }

    const auto* messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnMessage)) {
        logger::error("failed to register the messaging listener");
        return false;
    }

    logger::info("{} loaded; waiting for kDataLoaded", kPluginName);
    return true;
}
