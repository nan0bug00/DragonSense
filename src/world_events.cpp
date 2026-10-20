// World moments that deserve narration but have no SkyrimNet event of their own.
//
// The Greybeards' summons is a quest stage, and quest events are disabled in this
// load order (deliberately: enabling them would put every quest stage in front of
// the reaction system). So the plugin listens to the engine's own quest-stage
// notification instead - the same one Papyrus's RegisterForQuestStage uses - and
// raises a `custom` event, which is enabled. No global config is touched and no
// hook is installed: a sink is the lightest tool that can see this.
//
// Stage 10 of MQGreybeardCall is its startup stage, which is where the quest's
// script owns the shout sound, so it fires at the moment the shout plays.

#include "world_events.h"

#include "skyrimnet_bridge.h"

namespace
{
    constexpr RE::FormID    kGreybeardCallQuest = 0x10EC61;  // MQGreybeardCall
    constexpr std::uint16_t kGreybeardCallStage = 10;

    class QuestStageSink final : public RE::BSTEventSink<RE::TESQuestStageEvent>
    {
    public:
        RE::BSEventNotifyControl ProcessEvent(
            const RE::TESQuestStageEvent* a_event,
            RE::BSTEventSource<RE::TESQuestStageEvent>*) override
        {
            if (!a_event) {
                return RE::BSEventNotifyControl::kContinue;
            }

            if (a_event->formID != kGreybeardCallQuest || a_event->stage != kGreybeardCallStage) {
                return RE::BSEventNotifyControl::kContinue;
            }

            logger::info("greybeard call: stage {}-{} observed", a_event->formID, a_event->stage);

            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player) {
                logger::warn("greybeard call: no player, dropping");
                return RE::BSEventNotifyControl::kContinue;
            }

            if (!SkyrimNetBridge::MemorySystemReady()) {
                logger::warn("greybeard call: no save loaded, dropping");
                return RE::BSEventNotifyControl::kContinue;
            }

            if (SkyrimNetBridge::RegisterTypedEvent(
                    "dragonsense_event",
                    R"({"kind":"greybeard_call","mode":"narrate"})",
                    player->GetFormID())) {
                logger::info("greybeard call: event raised");
            } else {
                logger::warn("greybeard call: SkyrimNet refused the event");
            }

            return RE::BSEventNotifyControl::kContinue;
        }
    };

    QuestStageSink g_questStageSink;
}

namespace WorldEvents
{
    void OnDataLoaded()
    {
        auto* holder = RE::ScriptEventSourceHolder::GetSingleton();
        if (!holder) {
            logger::error("no script event source holder; quest stages cannot be watched");
            return;
        }

        holder->AddEventSink<RE::TESQuestStageEvent>(&g_questStageSink);
        logger::info("quest stage sink registered (watching MQGreybeardCall stage 10)");
    }
}
