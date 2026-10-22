// The Bleak Falls Sanctum bat swarm.
//
// The scene is a trigger volume: FXbatScaredSCRIPT's OnTriggerEnter fires only when
// the entrant is the player. The engine turns TESTriggerEnterEvent into a script event
// (SkyrimVM is a sink for it), and delivers it through IVirtualMachine::SendEvent. The
// event struct itself is not defined by CommonLib, so this hooks SendEvent instead and
// inspects the call: the receiving reference, its base object, its cell, and the actor
// passed as the first argument.
//
// All checks mirror the vanilla script, so this is tied to the set piece rather than to
// a sound, an animation name or a script name.

#include "bats_moment.h"

#include "skyrimnet_bridge.h"

#include <atomic>
#include <cstdint>

namespace
{
    constexpr RE::FormID kBatBaseFormID = 0x02C692;      // FXbatGroupACT
    constexpr RE::FormID kSanctumCellFormID = 0x0371DD;  // BleakFallsBarrow02

    RE::TESForm*               g_batBase = nullptr;
    RE::TESForm*               g_sanctum = nullptr;
    RE::BSFixedString          g_onTriggerEnter;
    std::atomic_bool           g_fired{ false };
    std::atomic<std::uint32_t> g_seen{ 0 };

    constexpr std::uint32_t kSampleLimit = 20;

    void Fire()
    {
        SKSE::GetTaskInterface()->AddTask([] {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || !SkyrimNetBridge::MemorySystemReady()) {
                logger::info("bats: matched, but there is no live player/save to narrate to");
                return;
            }

            if (SkyrimNetBridge::RegisterTypedEvent(
                    "dragonsense_event",
                    R"({"kind":"bleakfalls_bats","mode":"narrate"})",
                    player->GetFormID())) {
                logger::info("bats: event raised");
            } else {
                logger::warn("bats: SkyrimNet refused the event");
            }
        });
    }

    void Inspect(RE::BSScript::IVirtualMachine* a_vm, RE::VMHandle a_handle,
                 RE::BSScript::IFunctionArguments* a_args)
    {
        auto* policy = a_vm->GetObjectHandlePolicy();
        if (!policy || !a_args) {
            return;
        }

        auto* ref = static_cast<RE::TESObjectREFR*>(
            policy->GetObjectForHandle(RE::TESObjectREFR::FORMTYPE, a_handle));

        RE::BSScrapArray<RE::BSScript::Variable> args;
        (*a_args)(args);
        auto* entered = args.empty() ? nullptr : args[0].Unpack<RE::TESObjectREFR*>();

        const auto  seen = ++g_seen;
        auto*       base = ref ? ref->GetBaseObject() : nullptr;
        auto*       cell = ref ? ref->GetParentCell() : nullptr;

        const bool relevant = (base && base->GetFormID() == kBatBaseFormID) ||
                              (cell && cell->GetFormID() == kSanctumCellFormID);

        // Sample the first few trigger events of any kind, so the log proves the hook is
        // receiving them at all; after that only log what concerns this scene.
        if (seen <= kSampleLimit || relevant) {
            logger::info("bats: OnTriggerEnter #{} ref={:08X} base={:08X} cell={:08X} entered={:08X}",
                         seen,
                         ref ? ref->GetFormID() : 0u,
                         base ? base->GetFormID() : 0u,
                         cell ? cell->GetFormID() : 0u,
                         entered ? entered->GetFormID() : 0u);
        }

        if (!ref || !base || base->GetFormID() != kBatBaseFormID) {
            return;
        }
        if (!cell || cell->GetFormID() != kSanctumCellFormID) {
            logger::info("bats: swarm outside the sanctum, ignored (cell {:08X})",
                         cell ? cell->GetFormID() : 0u);
            return;
        }
        if (!entered || entered != RE::PlayerCharacter::GetSingleton()) {
            logger::info("bats: entered by {:08X}, not the player; ignored",
                         entered ? entered->GetFormID() : 0u);
            return;
        }
        if (g_fired.exchange(true)) {
            return;
        }

        logger::info("bats: sanctum swarm triggered by the player");
        Fire();
    }

    struct SendEventHook
    {
        static void thunk(RE::BSScript::IVirtualMachine* a_vm, RE::VMHandle a_handle,
                          const RE::BSFixedString& a_event, RE::BSScript::IFunctionArguments* a_args)
        {
            if (a_event == g_onTriggerEnter && !g_fired.load(std::memory_order_relaxed)) {
                Inspect(a_vm, a_handle, a_args);
            }

            func(a_vm, a_handle, a_event, a_args);
        }

        static inline REL::Relocation<decltype(thunk)> func;
    };

    void Install()
    {
        auto* handler = RE::TESDataHandler::GetSingleton();
        if (!handler) {
            logger::error("bats: no data handler; moment disabled");
            return;
        }

        g_batBase = handler->LookupForm(kBatBaseFormID, "Skyrim.esm");
        g_sanctum = handler->LookupForm(kSanctumCellFormID, "Skyrim.esm");
        if (!g_batBase || !g_sanctum) {
            logger::error("bats: could not resolve FXbatGroupACT or BleakFallsBarrow02; moment disabled");
            return;
        }

        g_onTriggerEnter = "OnTriggerEnter";

        // IVirtualMachine::SendEvent is index 0x24. VR inserts two virtuals earlier in
        // the table, which the library marks with SKYRIM_REL_VR_VIRTUAL; 0x26 is the VR
        // position and is untested.
        const auto index = REL::Module::IsVR() ? 0x26 : 0x24;

        static REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_BSScript__Internal__VirtualMachine[0] };
        SendEventHook::func = vtable.write_vfunc(index, SendEventHook::thunk);

        logger::info("bats: SendEvent hooked at index 0x{:X} (VR={}), base={:08X}, cell={:08X}",
                     index,
                     REL::Module::IsVR(),
                     g_batBase->GetFormID(),
                     g_sanctum->GetFormID());
    }
}

namespace BatsMoment
{
    void OnDataLoaded()
    {
        Install();
    }

    void OnLoad()
    {
        g_fired.store(false);
        logger::info("bats: one-shot guard reset for this save");
    }
}
