# DragonSense — handoff: the Bleak Falls bats moment

A context document for someone (human or model) asked to solve one detection problem.
It describes the add-on, what already works, and exactly what is needed for the bats
feature, including what has been tried and why it was rejected.

---

## 1. What DragonSense is

A SkyrimNet add-on that gives NPCs awareness of world-scale moments that are otherwise
player-only. It ships as **one mod archive**: an SKSE plugin DLL plus a SkyrimNet
content plugin (triggers), with a FOMOD that installs one of thirteen languages.

Current moments:

| Moment | Detection | Notes |
|---|---|---|
| Dragon roar (near/distant) | Hook on `BGSSoundDescriptorForm`'s `BSISoundDescriptor::DoResolve`, matching sound-record FormIDs | Distance to nearest live dragon; in combat = always near. Coin flip: stored event or narration |
| Greybeards' summons | `TESQuestStageEvent` sink: `MQGreybeardCall` (0010EC61) stage 10 | Detects the shout, not the quest end |
| Ustengrav rising statues | `TESActivateEvent` sink: four placed statue activator refs (085CEA, 085CF2, 085CF5, 085CFD) | `dunUstengravStatueScript` has `OnActivate`; this is scene-accurate |
| Bats in Bleak Falls Sanctum | **Not implemented** | This document |

All detections raise a `dragonsense_event` (a schema the plugin registers with
SkyrimNet) carrying `kind` and `mode`. Trigger YAMLs turn that into `direct_narration`
or `persistent_generic`. The plugin never writes to the save; removing the mod needs no
cleanup.

## 2. Repository state

- Repo: `https://github.com/nan0bug00/DragonSense`
- Working branch: `feature/fomod-and-immersion-pack` (uncommitted changes present:
  asterisks removed from narration lines, Ustengrav switched to the activation sink,
  bats disabled)
- Source: `src/` — `main.cpp`, `skyrimnet_bridge.cpp`, `dragon_roar.cpp`, `world_events.cpp`
- Content: `plugins/<language>/` — one SkyrimNet content plugin per language
- Build: CMake + Ninja + vcpkg, MSVC, CommonLibSSE-NG (alandtse `ng`)
- Packaging: `scripts/build-fomod.ps1` assembles the archive; `scripts/deploy.ps1`
  deploys to a separate `DragonSense - Test` mod

Toolchain notes:

- `src/skyrimnet/PublicAPI.h` may be included by exactly **one** translation unit
  (`skyrimnet_bridge.cpp`) because it defines, rather than declares, its globals.
- Event schema is registered on `kNewGame` / `kPostLoadGame`, not `kDataLoaded`
  (SkyrimNetApi is not bound at `kDataLoaded`).
- Game tested on Skyrim SE 1.6.1170, SKSE 2.2.6, SkyrimNet beta27.

## 3. What is needed: the bats moment

When the player enters the bat swarm in **Bleak Falls Sanctum** and the bats burst out,
NPCs near the player should receive a narration once. It must not fire for bats anywhere
else, and it must not fire for other things that use the same assets.

## 4. Verified facts

**The scene.** The bat swarm is an activator with `FXbatScaredSCRIPT`:

```
EVENT OnTriggerEnter(ObjectReference akActionRef)
    if (akActionRef == game.getPlayer())
        mySFX.play(self)
        weapLg.fire(self, myAmmo)
        playAnimationAndWait("MothTakeoff", "End")
```

So the scene signal is: *the player entering the trigger volume of a specific placed
reference*.

**Records.**

| Thing | FormID | Notes |
|---|---|---|
| Activator base | `02C692:Skyrim.esm` | `FXbatGroupACT`, script `FXbatScaredSCRIPT` |
| Placed refs in BleakFallsBarrow02 (Sanctum) | `037B0D`, `037B0E` | Two swarms in the cell |
| Sound marker | `01F03F:Skyrim.esm` | `AMBBats` → sound record `03E5D6` |
| Cell | `0371DD:Skyrim.esm` | `BleakFallsBarrow02`, "Bleak Falls Sanctum" |

The base activator is placed **fourteen** times: `E3demoBleakFallsBarrow` (×2),
`BleakFallsBarrow02` (×2), `Kagrenzel01`, `BloodletThrone01`, `Bthardamz03`,
`PinepeakCavernExterior`, `BlindCliffCaveExteriorUpper`, `Labyrinthian04origin`, plus
three placements with no editor ID.

**Why the obvious signals are rejected.**

- Sound record `03E5D6` — shared by every bat swarm; another mod could use it anywhere.
- Animation event `MothTakeoff` — used by `FXbatScaredSCRIPT`, `dunWEFXBatActivator`,
  and `FXmothScaredSCRIPT` (moths). Not bats-specific.
- Sound plus a player-cell check — still keyed to a shared asset, not the scene.

**Why the trigger-enter event was not used.** `ScriptEventSourceHolder` exposes
`BSTEventSource<TESTriggerEnterEvent>` (offset 0x1028), but neither alandtse
`CommonLibVR` `ng` nor CharmedBaryon `CommonLibSSE-NG` defines the struct's members —
it is only forward-declared. Reading it means guessing the layout, which would also need
re-probing on every runtime (1.5.97, 1.6.1170, 1.6.1179 GOG, 1.7.104, VR).

## 5. Constraints

- **Scene-accurate**: must be tied to the specific set piece, not to a shared sound,
  animation name, or script name.
- **Load-order robust**: vanilla FormIDs are stable; a mod may override the cell and
  move or re-place the swarm, so prefer base-object + cell matching over hardcoded refs
  where the chosen mechanism allows it.
- **Cross-version without probing each runtime**: the owner will not build separate
  modlists (SE, AE, GOG, VR) to validate. A solution that depends on a memory layout
  learned by observation is acceptable only if it is stable across runtimes.
- **No ESP preferred**; a small ESP is acceptable if it is the only robust route, but it
  must be justified.
- **No save writes**, no co-save, removable at any time.
- Existing plugin code is C++ against CommonLibSSE-NG (alandtse `ng`); Papyrus is
  possible only by shipping an ESP.

## 6. Avenues worth exploring

1. **Papyrus remote events.** If `RegisterForRemoteEvent(ref, "OnTriggerEnter")` is
   supported, a start-game quest in a tiny ESP could listen on `037B0D`/`037B0E` and call
   `SkyrimNetApi.RegisterEvent` directly. Version-independent, no memory layout, fully
   scene-accurate. *Needs verification that the event is remotely registerable.*
2. **ESP override of the two placed references** to attach a script, or an ESP-added
   trigger volume in the Sanctum. Cross-version by construction, but edits a vanilla cell.
3. **Animation events filtered by the emitting reference.** `MothTakeoff` is shared, but
   if a sink can be attached to those two refs' animation graphs and the *ref* is checked,
   the name no longer matters. *Needs verification that an activator dispatches
   `BSAnimationGraphEvent` to a sink that can be registered from a plugin.*
4. **Hook `TESObjectREFR::PlayAnimation`** (non-virtual) and filter on `this`. Needs an
   Address Library ID; stable if the ID exists across versions.
5. **VM-level interception** of the script event dispatch for that ref/script.
6. **Proximity polling** of the two refs while inside `BleakFallsBarrow02` — cheap but
   inelegant, and fires on proximity rather than the actual trigger.

## 7. Deliverable

A written proposal, not a patch:

- the chosen mechanism and why it satisfies the constraints,
- an honest cross-version analysis (which runtimes it works on, and what would need
  testing),
- failure modes (silent miss vs. wrong narration vs. crash),
- an implementation sketch against CommonLibSSE-NG (alandtse `ng`) or Papyrus,
- anything that would need empirical verification before it can be trusted.

Do not assume the existing code is correct; question it. The plugin's existing moments
are documented above with their exact mechanisms so they can be used as precedent.
