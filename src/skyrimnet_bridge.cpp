#include "skyrimnet_bridge.h"

#include "skyrimnet/PublicAPI.h"

#include <mutex>
#include <string>
#include <vector>

namespace
{
    std::once_flag g_once;
    SkyrimNetBridge::Diagnostics g_diagnostics;

    void ResolveOnce()
    {
        if (!FindFunctions()) {
            return;
        }

        g_diagnostics.version = PublicGetVersion ? PublicGetVersion() : 0;
        g_diagnostics.registerEvent = PublicRegisterEvent != nullptr;
        g_diagnostics.addWorldKnowledge = PublicAddWorldKnowledge != nullptr;
        g_diagnostics.memoryReadyFn = PublicIsMemorySystemReady != nullptr;
        g_diagnostics.formToUuid = PublicFormIDToUUID != nullptr;
        g_diagnostics.actorName = PublicGetActorNameByUUID != nullptr;
        g_diagnostics.resolved = true;
    }
}

namespace SkyrimNetBridge
{
    Diagnostics Resolve()
    {
        std::call_once(g_once, ResolveOnce);
        return g_diagnostics;
    }

    bool IsResolved()
    {
        return Resolve().resolved;
    }

    int Version()
    {
        return Resolve().version;
    }

    bool MemorySystemReady()
    {
        return Resolve().memoryReadyFn && PublicIsMemorySystemReady && PublicIsMemorySystemReady();
    }

    std::string ActorNameForForm(std::uint32_t a_formId, std::uint64_t* a_outUuid)
    {
        if (a_outUuid) {
            *a_outUuid = 0;
        }

        const auto diagnostics = Resolve();
        if (!diagnostics.formToUuid || !diagnostics.actorName) {
            return {};
        }

        const auto uuid = PublicFormIDToUUID(a_formId);
        if (a_outUuid) {
            *a_outUuid = uuid;
        }
        if (uuid == 0) {
            return {};
        }

        return PublicGetActorNameByUUID(uuid);
    }

    std::string EscapeJson(const char* a_text)
    {
        std::string out;
        for (const char* p = a_text; p && *p; ++p) {
            switch (*p) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += *p; break;
            }
        }
        return out;
    }

    bool RegisterCustomEvent(const char* a_description, const char* a_data,
                             std::uint32_t a_originatorFormId)
    {
        if (!Resolve().registerEvent || !PublicRegisterEvent) {
            return false;
        }
        if (!a_description || !*a_description) {
            return false;
        }

        std::string payload;
        if (a_data && *a_data) {
            payload = std::string{ R"({"description":")" } + a_description +
                      R"(","data":")" + a_data + R"("})";
        } else {
            payload = std::string{ R"({"description":")" } + a_description + R"("})";
        }

        const int id = PublicRegisterEvent("custom", payload.c_str(), a_originatorFormId, 0, nullptr, 0);
        return id != 0;
    }

    bool RegisterNarration(const char* a_text, std::uint32_t a_originatorFormId)
    {
        if (!Resolve().registerEvent || !PublicRegisterEvent || !a_text || !*a_text) {
            return false;
        }

        const std::string payload = std::string{ R"({"narration":")" } + EscapeJson(a_text) + R"("})";
        const int         id = PublicRegisterEvent("direct_narration", payload.c_str(),
                                                   a_originatorFormId, 0, nullptr, 0);
        return id != 0;
    }

    bool RegisterEventSchema(const char* a_type, const char* a_displayName, const char* a_description,
                             const char* a_fieldsJson, const char* a_renderJson)
    {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (!vm) {
            logger::warn("schema: no Papyrus VM yet");
            return false;
        }

        // Papyrus fills defaulted arguments at compile time, so a C++ dispatch has
        // to supply all nine. Ephemeral + a short TTL keeps the source event out of
        // history while still letting triggers see it; scene-context copies stay off
        // because the source is plumbing, not prose.
        auto* args = RE::MakeFunctionArguments(
            RE::BSFixedString{ a_type ? a_type : "" },
            RE::BSFixedString{ a_displayName ? a_displayName : "" },
            RE::BSFixedString{ a_description ? a_description : "" },
            RE::BSFixedString{ a_fieldsJson ? a_fieldsJson : "[]" },
            RE::BSFixedString{ a_renderJson ? a_renderJson : "{}" },
            true,    // isEphemeral
            60000,   // defaultTTLMs
            false,   // shortLivedEnabled
            false);  // interrupt

        RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> result;
        const bool ok = vm->DispatchStaticCall(RE::BSFixedString{ "SkyrimNetApi" },
                                               RE::BSFixedString{ "RegisterEventSchema" }, args, result);
        delete args;
        return ok;
    }

    bool RegisterTypedEvent(const char* a_type, const char* a_json, std::uint32_t a_originatorFormId)
    {
        if (!Resolve().registerEvent || !PublicRegisterEvent || !a_type || !*a_type) {
            return false;
        }

        const int id = PublicRegisterEvent(a_type, a_json ? a_json : "{}", a_originatorFormId, 0, nullptr, 0);
        if (id == 0) {
            // Expected for an ephemeral type: nothing is stored, so there is no id.
            logger::info("event '{}' accepted with no stored id (ephemeral)", a_type);
        }
        return true;
    }
}
