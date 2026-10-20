# DragonSense

Gives SkyrimNet NPCs ears for things typically only perceived by the player.

Two things, so far:

- **Dragon roars.** The plugin watches the dragon flight-roar sound records, ignores the ones that are a kill or a resurrection, classifies the roar by how far the nearest live dragon is, and raises its own `dragonsense_event`. A dragon in combat within a cell and a half counts as near; anything else is distant; a dragon within half a cell is near regardless.
- **The Greybeards' summons.** Watched from `MQGreybeardCall`'s startup stage, so it narrates when the shout actually plays rather than when the quest ends.

Each roar episode is a coin flip: half become a stored event the NPCs can remember later, half become an authored narration that provokes a reaction now. The two lines are in the trigger files, so they can be reworded without rebuilding anything.

No ESP. One DLL plus two trigger files, all inside one mod folder.

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) and SKSE64
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/108752) 0.25 or newer

## Install

Install the mod folder with your mod manager and enable it. That's it — the plugin registers its event type with SkyrimNet when a save loads, and the trigger files are read from `SKSE/Plugins/SkyrimNet/external/steamuser.dragonsense/`.

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
