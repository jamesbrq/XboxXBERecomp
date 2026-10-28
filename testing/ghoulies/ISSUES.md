# Audio, particle depth and stutter investigation

Work performed without a game launch or boot test.

## Follow-up: menu confirm SFX and Grand Hallway failure

### Confirmed live failure and prepared asset repair

### Subsequent live challenge stall: entrance swap remains active

The repaired ISO avoids the missing-resource fatal path but exposes an earlier routing conflict. During manual play, the user reported challenge music with no counter, away from the big front door in Grand Hallway. Read-only process inspection found `dc_phase=2`, `dc_row=0x5a6fdc`, `dc_failed=0`, an empty live spawn queue, and native event state 1 waiting for door 2. Door 2 remained in state 0; door 1 was in event state 5. `route_swap_on=1`, with `route_entry=0x5a9bb0` mapping physical entry door 2 and vanilla entrance door 1. `route_e1` therefore redirects the challenge's door-2 animation to door 1, while the native event machine still checks door 2. The same redirection explains the earlier square_2_highleft missing animation. The seven asset aliases are a defensive fallback, not the root routing correction.

The companion mod's `route.py` documents that the swap is for entrance-script operations only and should end at the close-behind/entrance-done operation. Challenge-generated UpdateDoor calls currently traverse that same remapper. A correction must scope remapping to entrance operations (and preserve entrance completion), so challenge door ids and native event checks agree. Do not skip the native wait or mark the challenge cleared: that would hide the mismatch and alter progression. Trigger-distance behavior also remains to be verified separately. The live game was left open and no process memory was modified.

The patched host now overrides `route_e1` only for the exact selected walktest XBE SHA-1. Because this pre-hook runs before UpdateDoor's prologue, the original UpdateDoor return address is at guest esp+4. Calls from the four verified native event-loop call sites and the executable mod section bypass entrance remapping; other callers delegate to the original implementation, retaining entrance-script swapping and its completion behavior. This also covers mod cleanup/repose calls after `dc_row` is cleared, rather than relying solely on a challenge-active flag. The native event machine still runs and checks objectives normally.

The regression exercises all four event callers, mod callers and section boundaries, verifies guest stack/register/flag/memory preservation on bypass, and verifies delegation for ordinary script/replay callers. All three native regression tests pass. The patched EXE and DLL were rebuilt and their export/import linkage to the override was verified without a boot test. The running old EXE/DLL were renamed to `Ghoulies.before-door-routing.*` to permit rebuilding without closing the user's game. Gameplay confirmation and the separate trigger-distance report remain pending the next manual launch.

The next manual run also displayed dirty-disc. Read-only inspection of the still-running process recovered the guest stack: `0x5b6ed -> 0x5baf4 -> 0x4d6b5 -> ExitGame(2)`. The fatal lookup is `aid_cutscene_ghoulies_doors_square_2_highleft_openevent`; its associated camera name is `aid_cutscene_ghoulies_doors_cameras_centralhallway_1openevent`. Both are absent. This identifies a missing door-transition resource, rather than a failed disc read or the carried chair itself.

`repair-door-assets.py` creates a separate copy of the selected full mod ISO, adding seven aliases in the Central Hallway bundle for this unsupported event-door shape and camera. They reuse the existing noentry/open animation and event assets, following the companion mod tools' fallback approach. It verifies all 1,144 file payloads: only that room bundle changes; the XBE, executable mod section and cutscene skip remain identical. The source ISO is never edited. The repaired copy is staged as `assets/00-ghoulies_walktest-doorfix.iso` beside the patched executable, so the loader's sorted selection uses it on the next launch. The currently open process keeps its original disc handle and is left untouched. No boot test was performed; correct door progression and fallback appearance remain to be tested manually. This does not fix or validate missing menu SFX.

To reproduce (destination must not already exist):

```powershell
python testing/ghoulies/repair-door-assets.py H:/GhidraProjects/ghoulies-decomp/build/ghoulies_walktest.iso testing/ghoulies/out/repairs/ghoulies_walktest-doorfix.iso --mod-tools H:/GhidraProjects/ghoulies-decomp/tools/modding
```

The manual run `20261027-210008-572` entered the game's dirty-disc handler at 40.725 seconds (loading `dirtydisk_bitmap`), then exited with code 0 when closed. There is no new native exception dump. The user was walking in the Grand Hallway carrying a chair. Queries for `demand/0a27b844` and `demand/0b05eae4` immediately precede the handler; both files are absent from the base extraction and full mod ISO. This is correlation, not a confirmed missing dependency: optional demand lookups also fail during normal play.

The launcher was setting disabled switches to `$null` through .NET. On this machine that creates present, empty environment variables; native presence checks therefore enabled kernel tracing, performance reporting, GPU debugging, and the no-HRTF/no-reverb switches despite the recorded options being false. Disabled switches now use explicit removal, including when restoring the parent environment. A native-child inheritance regression verifies this without starting the game.

The missing A-button menu sound remains unconfirmed. The first 64 buffer starts now retain format, data/region, routing, volume, pause and synchronized-start state in ordinary logs, so short menu cues cannot disappear between periodic mixer reports. Held synchronized buffers explicitly report the unresolved SynchPlayback hook. Short file reads also retain path, offset, byte count and caller without requiring full kernel tracing. These are diagnostic changes, not a claimed fix for the menu sound or the Grand Hallway failure. The patched executable was rebuilt without booting it.

## Missing sound effects

An asynchronous stream stop left `released` latched. `Process` accepted packets but refused playback, and `Pause(0)`/`Pause(2)` never cleared the latch. Stream reuse now retires old packets and resets stop state on those explicit start/resume operations. Automatic playback on late packets remains blocked, preserving the music-stop fix. Tests cover stop deadlines, repeated stops, late-packet blocking, explicit resume, synchronized reuse and reset.

This fixes a concrete lifecycle defect, not proof that every reported missing SFX shares that cause. Synchronized starts still warrant examination: the 5659 configuration has no resolved `IDirectSound_SynchPlayback` hook. With `-Diagnostics`, stream reports now include active/paused/sync/pending-stop/released state and packet counts to distinguish held voices from empty or silent input.

## Green smoke on ghoulie deaths

The user identified the death smoke specifically. The available gameplay capture at `runs/20261027-025133-864/025234` contains 68 blended draws: all have depth testing enabled with LESS_EQUAL; 57 disable depth writes. It does not establish a death-smoke frame, so it cannot identify the faulty draw or justify changing its depth test/sorting.

No speculative occlusion change was applied. F3 captures now record depth mode, normalization scale, viewport depth range and shader depth parameters, alongside the existing vertices, constants, shaders and bound surfaces. Missing/incompatible depth surfaces produce warnings. A manual capture while the smoke overlaps scenery/player is needed to discriminate particle projection/offset errors from target binding or intentionally foreground effects. Automatic captures remain off by default.

## Random stutters

Existing logs show synchronous shader compilation coinciding with large frame gaps, including a 776 ms gap near 25.5 seconds. Periodic captures at approximately 30 and 60 seconds also perform synchronous GPU readbacks and texture dumping. Most intervening frames hold 60 FPS.

Normal launches now omit periodic captures, verbose/kernel tracing and WAV recording. Shader bytecode persists across processes in a checksum-validated cache. A source/profile/entry/flags change invalidates the key; corrupt or incomplete files fall back to compilation. This reduces repeat shader stalls, not first-use shader or driver pipeline costs. No asynchronous skipping of draws was introduced.

Without booting the game, the offline utility compiled 113 unique shaders from existing captures and reused 441 repeated entries, with zero failures. A second pass reused all 554 entries without compilation. Native stream/cache regression tests and the full patched build passed. Actual audio recovery, death-smoke occlusion and gameplay frame-time improvements remain manual validation items.
