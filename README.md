# DragonSense

Gives SkyrimNet NPCs ears for things typically only perceived by the player.

Two things, so far:

- **Dragon roars.** The plugin watches the dragon flight-roar sound records, ignores kill and resurrection roars, and raises a DragonSense event when one plays. A dragon in combat is always classified near; otherwise within 2,000 units is near and anything further is distant.
- **The Greybeards' summons.** Watched from `MQGreybeardCall`'s startup stage, so it narrates when the shout actually plays rather than when the quest ends.

Each roar episode is a coin flip: half become a stored event the NPCs can remember later, half become an authored narration that provokes a reaction now.

This is a SkyrimNet add-on: one SKSE plugin for the detection, plus a SkyrimNet content plugin for the words. It does nothing without SkyrimNet.

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) and SKSE64
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/108752) 0.25 or newer

## Install

1. Install the DragonSense mod with your mod manager and enable it. It carries the DLL.
2. Install **one** language plugin from SkyrimNet's Plugin Hub (Plugins in the dashboard): DragonSense for English, or one of the translations. Only one should be enabled.

## Languages

English, Russian, Japanese, Portuguese, Spanish, French, German, Italian, Polish, Korean, Vietnamese, Chinese (Simplified), Chinese (Traditional).

Each is its own plugin with its own id, so they don't collide. Enable one, disable any other.

## Building

Requires Visual Studio's C++ build tools, CMake and vcpkg.

```
git clone -b ng https://github.com/alandtse/CommonLibVR.git lib/commonlibsse-ng
cmake --preset default
cmake --build build
```

The CommonLibSSE-NG checkout is not committed; clone it into `lib/` as shown. `scripts/deploy.ps1` copies the built DLL and the English plugin into an MO2 instance for testing.

## Layout

- `src/` — the SKSE plugin.
- `plugins/<language>/` — SkyrimNet content plugins, one per language, each ready to publish to the Hub. The DLL is packaged from `build/` by the release workflow.

## Credit

The event-type registration pattern — dispatching `SkyrimNetApi.RegisterEventSchema` through the Papyrus VM, on save load, with `RE::MakeFunctionArguments` — was copied from [AgencyEngine](https://github.com/cleanestpoison/AgencyEngine) by cleanestpoison. Thank you.

## License

MIT. See `LICENSE`.
