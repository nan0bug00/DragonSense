# DragonSense

![DragonSense cover](plugins/en/cover.png)

Gives SkyrimNet NPCs ears for things typically only perceived by the player.

A collection of one-off moments — the kind that happen once in a playthrough and then never again, so almost nobody writes a trigger for them:

- **Dragon roars.** Fight one, or hear one across the valley. A dragon in combat is always classified near; otherwise within 2,000 units is near and anything further is distant.
- **The Greybeards' summons.** Watched from `MQGreybeardCall`'s startup stage, so it narrates when the shout actually plays rather than when the quest ends.
- **Bats.** A colony bursts out of the dark close enough to feel the air move.
- **Ustengrav's tomb.** Stone grinds against stone as something ancient shifts in the dark.

Roar episodes are a coin flip: half become a stored event the NPCs can remember later, half become an authored narration that provokes a reaction now. The one-off moments always narrate.

One mod, one archive: the SKSE plugin plus its SkyrimNet content plugin, which SkyrimNet picks up as an external layer. No ESP.

## Requirements

- Skyrim Special Edition / Anniversary Edition (1.6.x) and SKSE64
- [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
- [SkyrimNet](https://www.nexusmods.com/skyrimspecialedition/mods/108752) 0.25 or newer

## Download and install

[Latest release](https://github.com/nan0bug00/DragonSense/releases/latest) — one archive.

Install it with your mod manager. The installer asks which language you want, then places the DLL and that language's plugin. SkyrimNet registers the plugin on start-up with an **External** badge on its Installed Plugins page.

The Hub entry for DragonSense is a listing that links here, since the download has to carry the DLL.

## Languages

English, Russian, Japanese, Portuguese, Spanish, French, German, Italian, Polish, Korean, Vietnamese, Chinese (Simplified), Chinese (Traditional).

Pick one in the installer. To change language later, reinstall the archive and choose again.

## Building

Requires Visual Studio's C++ build tools, CMake and vcpkg.

```
git clone -b ng https://github.com/alandtse/CommonLibVR.git lib/commonlibsse-ng
cmake --preset default
cmake --build build
```

The CommonLibSSE-NG checkout is not committed; clone it into `lib/` as shown. `scripts/build-fomod.ps1` assembles the release archive from `plugins/`.

`scripts/deploy.ps1` deploys to a separate **DragonSense - Test** mod — test builds never touch the stable install. Writing to the stable mod requires `-AllowStable`, which is for release verification only. Enable the test mod and disable the stable one while testing.

## Layout

- `src/` — the SKSE plugin.
- `plugins/<language>/` — SkyrimNet content plugins. All of them go into the archive; the FOMOD installs one.

## Credit

The event-type registration pattern — dispatching `SkyrimNetApi.RegisterEventSchema` through the Papyrus VM, on save load, with `RE::MakeFunctionArguments` — was copied from [AgencyEngine](https://github.com/cleanestpoison/AgencyEngine) by cleanestpoison. Thank you.

## License

MIT. See `LICENSE`.
