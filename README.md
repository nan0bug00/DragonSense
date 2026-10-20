# DragonSense

Gives SkyrimNet NPCs ears for things typically only perceived by the player.

Two things, so far:

- **Dragon roars.** The plugin watches the dragon flight-roar sound records, ignores kill and resurrection roars, and raises its own `dragonsense_event` when one plays. A dragon in combat is always classified near; otherwise within 2,000 units is near and anything further is distant.
- **The Greybeards' summons.** Watched from `MQGreybeardCall`'s startup stage, so it narrates when the shout actually plays rather than when the quest ends.

Each roar episode is a coin flip: half become a stored event the NPCs can remember later, half become an authored narration that provokes a reaction now.

No ESP. One DLL plus two trigger files, all inside one mod folder.

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) and SKSE64
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/108752) 0.25 or newer

## Install

Install the mod folder with your mod manager and enable it. The plugin registers its event type with SkyrimNet when a save loads, and the trigger files are read from `SKSE/Plugins/SkyrimNet/external/stoptryharding.dragonsense/`.

## Languages

English is built in. The other eleven ship as separate packs in the release zip:

Russian, Japanese, Portuguese, Spanish, French, German, Italian, Polish, Korean, Vietnamese, Chinese (Simplified), Chinese (Traditional).

Install the pack you want alongside the main mod and let it win the file conflict — in MO2, place it after DragonSense in the left pane. Each pack provides the same layer path with translated text, so exactly one can be active.

## Building

Requires Visual Studio's C++ build tools, CMake and vcpkg.

```
git clone -b ng https://github.com/alandtse/CommonLibVR.git lib/commonlibsse-ng
cmake --preset default
cmake --build build
```

The CommonLibSSE-NG checkout is not committed; clone it into `lib/` as shown. `scripts/deploy.ps1` copies the built DLL, the PDB and the mod content into an MO2 instance.

## Credit

The event-type registration pattern — dispatching `SkyrimNetApi.RegisterEventSchema` through the Papyrus VM, on save load, with `RE::MakeFunctionArguments` — was copied from [AgencyEngine](https://github.com/cleanestpoison/AgencyEngine) by cleanestpoison. Thank you.

## License

MIT. See `LICENSE`.
