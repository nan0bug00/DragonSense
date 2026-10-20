#pragma once

#include <cstdint>
#include <string>

// The SkyrimNet headers define their function pointers at namespace scope rather
// than declaring them extern, so exactly one translation unit may include them.
// This is that unit: everything else calls through here.
namespace SkyrimNetBridge
{
    struct Diagnostics
    {
        int  version{ 0 };
        bool registerEvent{ false };
        bool addWorldKnowledge{ false };
        bool memoryReadyFn{ false };
        bool formToUuid{ false };
        bool actorName{ false };
        bool resolved{ false };
    };

    // Loads SkyrimNet and resolves the exported functions. Safe to call repeatedly;
    // the work happens once.
    Diagnostics Resolve();

    bool IsResolved();
    int  Version();
    bool MemorySystemReady();

    // FormID -> SkyrimNet UUID -> display name. Returns an empty string on failure.
    std::string ActorNameForForm(std::uint32_t a_formId, std::uint64_t* a_outUuid);

    // Registers a `custom` event. With a zero audience count SkyrimNet treats the
    // originator's neighbourhood as the audience.
    bool RegisterCustomEvent(const char* a_description, const char* a_data,
                             std::uint32_t a_originatorFormId);

    // Registers an authored narration line, perceived by the originator's
    // neighbourhood (which includes the player).
    bool RegisterNarration(const char* a_text, std::uint32_t a_originatorFormId);

    // Registers our own event type with SkyrimNet, the way DLL-based add-ons do:
    // by dispatching SkyrimNetApi.RegisterEventSchema through the Papyrus VM.
    bool RegisterEventSchema(const char* a_type, const char* a_displayName, const char* a_description,
                             const char* a_fieldsJson, const char* a_renderJson);

    // Raises an event of a type already registered with SkyrimNet.
    bool RegisterTypedEvent(const char* a_type, const char* a_json, std::uint32_t a_originatorFormId);
}
