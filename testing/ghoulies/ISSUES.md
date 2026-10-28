# Audio, particle depth and stutter investigation

Work performed without a game launch or boot test.

## Pause-panel striped backgrounds

F3 capture `runs/20261027-222745-058/223430` shows striped borders on the Grand Hallway pause panels. Draws 958 and 972 are untextured fixed-function line lists (96 and 72 vertices, handle 0x43) with alpha blending; the capture contains no polygon-stipple draws. At the 3x render resolution, native D3D12's one-host-pixel lines leave gaps between adjacent console-pixel rows used to build the panel's rounded/faded border.

Upscaled, unlit fixed-function lines (including XYZRHW lines) now use a line-input geometry shader to expand the primitive to one console pixel across. It retains endpoint varyings and depth, rejects degenerate/projectively invalid endpoints, and disables triangle culling for the generated strip. Native-resolution lines and programmable/lit line paths remain unchanged. The pipeline key distinguishes expanded lines. This addresses line coverage rather than changing panel opacity or removing its fade.

The regression compiles and reflects the geometry shader to verify line input, triangle-strip output and four output vertices. All four runtime regression executables pass, and the patched EXE/DLL rebuilt without a game launch. The running game remains open on the previous files, preserved as `Ghoulies.before-ui-lines.*`. Manual visual validation is pending restart.

Later captures `20261027-224211-574/224318`, `224320`, and `224336` still contain the same fixed-function handle-0x43 panel line draws. The expansion selector also gated on the lighting render state; a stale enabled lighting flag could exclude these position/diffuse-only vertices even though they have no normal input. The shared selector now includes fixed-function lines without normals independently of that flag. A regression exercises lighting enabled with no normals, native-resolution exclusion, and preservation of programmable/lit-normal line paths. Captures now explicitly report `console_line` and lighting state for line draws. This correction and the mode-3 audio fix were rebuilt together without a boot test; process 30888 remained running on preserved `Ghoulies.before-menu-pause3.*` files.

## Follow-up: menu confirm SFX and Grand Hallway failure

### Confirmed live failure and prepared asset repair

### Subsequent live challenge stall: entrance swap remains active

The repaired ISO avoids the missing-resource fatal path but exposes an earlier routing conflict. During manual play, the user reported challenge music with no counter, away from the big front door in Grand Hallway. Read-only process inspection found `dc_phase=2`, `dc_row=0x5a6fdc`, `dc_failed=0`, an empty live spawn queue, and native event state 1 waiting for door 2. Door 2 remained in state 0; door 1 was in event state 5. `route_swap_on=1`, with `route_entry=0x5a9bb0` mapping physical entry door 2 and vanilla entrance door 1. `route_e1` therefore redirects the challenge's door-2 animation to door 1, while the native event machine still checks door 2. The same redirection explains the earlier square_2_highleft missing animation. The seven asset aliases are a defensive fallback, not the root routing correction.

The companion mod's `route.py` documents that the swap is for entrance-script operations only and should end at the close-behind/entrance-done operation. Challenge-generated UpdateDoor calls currently traverse that same remapper. A correction must scope remapping to entrance operations (and preserve entrance completion), so challenge door ids and native event checks agree. Do not skip the native wait or mark the challenge cleared: that would hide the mismatch and alter progression. Trigger-distance behavior also remains to be verified separately. The live game was left open and no process memory was modified.

The patched host now overrides `route_e1` only for the exact selected walktest XBE SHA-1. Because this pre-hook runs before UpdateDoor's prologue, the original UpdateDoor return address is at guest esp+4. Calls from the four verified native event-loop call sites and the executable mod section bypass entrance remapping; other callers delegate to the original implementation, retaining entrance-script swapping and its completion behavior. This also covers mod cleanup/repose calls after `dc_row` is cleared, rather than relying solely on a challenge-active flag. The native event machine still runs and checks objectives normally.

### Pause/unpause report after the routing correction

Read-only inspection of the next running build found `g_paused` at `0x510ce8` cleared, global flags `0x02000000` (door camera), and a responsive native window. Native event state is still 1, door id 2, with `dc_phase=2`. This time door 2 correctly receives target state 5, while door 1 stays in state 0 with target 1: the routing correction is active. Door 2's current state remains 0 and its animation time at +0x2c remains 0 (duration +0x34 is approximately 3.8333 seconds), so the native special-case check for door 2 in `0x5b7c0` cannot advance the event. Camera-freeze counters remain set (`0x510cec=1`, `0x4dea24=2`). These observations do not establish whether pausing caused the animation to stop or the event had already stalled. Guest stack/global snapshots were retained in ignored `out/research/pause-freeze-*.bin`. No speculative counter reset or challenge-completion bypass was applied; the process remains open and unmodified. Animation scheduling/resume remains unresolved.

Further inspection confirmed the active door's update gate (+0xb8) is enabled, its per-animation hold count (+0x158) is zero, frame delta is 1/60 and speed is 1. The native door routine `0x5aa90` calls the normal animation update and releases its own door-camera freeze when the animation finishes. The exact earlier scene-state scheduling failure is not yet established.

The walktest-only host frame override now samples the waiting door animation around the complete original frame. If the same mod challenge is still in native introduction state 1, its active door targets event state 5, a door camera owns the freeze, pause/menu/cutscene flags are clear, and animation time has not advanced, it calls the original native door routine once on a private register context. Positive finite timestep/speed and a valid duration are required; long/zero timesteps are rejected. It never assigns animation time, clears counters, marks objectives complete, or skips the native event wait. Normal animation updates therefore cannot be doubled. This is a bounded recovery for the observed scheduling stall, not proof of the underlying pause transition's cause.

Tests cover stalled recovery, native progress without duplicate ticking, pause/menu/cutscene exclusions, inactive/combat challenge exclusions, invalid timestep, and preservation of the outer frame's registers/flags/stack return slot. All three regression executables pass, and patched EXE/DLL linkage was rebuilt without booting the game. Gameplay validation remains pending. The running previous build stays open under renamed `Ghoulies.before-animation-resume.*` files.

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

### Latest menu logs: PAUSENOACTIVATE was treated as resume

Run `20261027-224211-574` finally records short stream cues: e.g. stream `B0164800` is prepared with Pause(3), receives mono PCM 22050-Hz packets, then receives Pause(0). Other menu streams follow the same order. Existing code set `paused` only for mode 1, so mode 3 permitted playback during preparation; one later resume reports an already-empty packet queue. The selected SDK implementation at `0x1cb290` has a distinct mode-3 inactive-voice hold path (sets bit 0x20), consumed by its explicit mode-0 start path. Mode 3 is DSSTREAMPAUSE_PAUSENOACTIVATE, not resume.

The host stream policy now holds modes 1 and 3. Only explicit resume/synchronized start clears the hold, and mode 3 cannot rearm a released stream. Regression tests reproduce the logged Pause(3) -> Process -> Pause(0) sequence and check both packet acceptance while held and release-state preservation. This fixes a concrete start-order defect evidenced in the menu path; audible menu-confirm recovery still requires manual confirmation.

### Follow-up: preserve packets queued before explicit resume

The newest ordinary run log contains no buffer-start reports, so resident-buffer logging has not identified the missing A-button cue. The stream path was reviewed again. The earlier explicit-resume correction called `FlushStream` after rearming a stopped stream; that flush discards packets already queued for the next cue. `Process` is permitted before the owner's explicit resume, so this can erase a reused short SFX while still returning success to the caller.

Explicit `Pause(0)`/`Pause(2)` now clears the stop latch without flushing the queue. If the stream has queued data and was inactive, it resets its cursor/filter state, activates playback, and starts its submixes. Pausing alone does not rearm; Process alone still cannot restart released music, and mixer stop completion still flushes late old-cue packets. The queue-before-resume case remains timing-sensitive if the caller leaves a stopped stream's packets pending across a mixer pass; this change does not claim to solve every reuse order.

Tests now cover queue-before-resume activation, synchronized starts, inactive resume-before-queue, preserving a playing cue when cancelling a pending release, and pause-only blocking. All four runtime regressions pass. A capped ordinary-log trace of short stream starts and explicit resumes was added; setup-only pauses do not consume its resume report budget. This should identify whether the menu cue is submitted, synchronized/paused, or absent on the next manual test. The patched EXE/DLL rebuilt without a boot test and the user's current game remains open under preserved `Ghoulies.before-sfx-reuse.*` files. Missing menu SFX recovery is not yet confirmed by gameplay.

An asynchronous stream stop left `released` latched. `Process` accepted packets but refused playback, and `Pause(0)`/`Pause(2)` never cleared the latch. Stream reuse now retires old packets and resets stop state on those explicit start/resume operations. Automatic playback on late packets remains blocked, preserving the music-stop fix. Tests cover stop deadlines, repeated stops, late-packet blocking, explicit resume, synchronized reuse and reset.

This fixes a concrete lifecycle defect, not proof that every reported missing SFX shares that cause. Synchronized starts still warrant examination: the 5659 configuration has no resolved `IDirectSound_SynchPlayback` hook. With `-Diagnostics`, stream reports now include active/paused/sync/pending-stop/released state and packet counts to distinguish held voices from empty or silent input.

## Green smoke on ghoulie deaths

### Captured depth-reconstruction defect and fix

### Follow-up: live depth alias and square boundary

Newer F3 capture `runs/20261027-220934-543/221131` confirms `SV_Depth` is active but shows residual glow through the stair railing. Draw 1387 reconstructs depth using texture stage 1 at `83739000`, declared as LIN_A8R8G8B8 even though that memory is the offscreen depth surface. Draw 1388 composites colour from `83865000` over rectangle approximately [120,252.384]-[208,315.204]. The texture lookup previously keyed colour and depth separately, so this colour alias could miss the live depth surface and use a stale colour/guest copy.

For a DOT_ZW pass whose two dot operands read the same colour alias of a known live depth target, the backend now binds that depth resource as ordinary scalar data, with point sampling rather than a shadow-comparison sampler. Shader generation uses its already-normalized depth value, respecting the backend's reversed convention, instead of treating the float depth as packed colour dot inputs. The selection is limited to the LIN_A8R8G8B8 alias and a valid DOTPRODUCT/DOT_ZW pair with matching input stages; normal colour textures and shadow comparisons keep their existing paths.

An isolated partial colour clear associated with a depth target now records normalized texel-centre bounds. Sampling that colour target clamps its filter footprint to those bounds, avoiding old pixels outside the effect rectangle. Full/partial-channel/multi-rect clears reset the bound; a subsequent draw using a larger viewport invalidates it. This does not change geometry clipping or expand the glow rectangle.

Regression shaders cover direct depth reads in both depth conventions, absence of the packed-colour ratio in this path, and compilation of bounded colour sampling. All four runtime tests pass. The patched EXE/DLL rebuilt without a boot test; visual correctness remains to be confirmed manually. Loaded files were preserved as `Ghoulies.before-effect-clipping.*`. A later process check found the previous run had exited with code `0x80000003`, `forcedTermination=false`; no close or stop command was issued during this work.

Manual F3 capture `runs/20261027-215335-403/215615` contains the reported effect. Draws 724 and 806 are depth-only screen quads (`cw 0`, depth write enabled) sampling the effects depth surface at `83739000`. Their captured pixel shader `ps_48AC3917B02CFD83.hlsl` calculates `dp2` and `dp3` but has no depth output. The translator grouped Xbox `PS_TEXTUREMODES_DOT_ZW` (0xA) with ordinary dotproduct (0x11), discarding the depth replacement side effect. These passes consequently used the quad's interpolated depth instead of the depth reconstructed from the texture, corrupting subsequent occlusion.

Pixel shader generation now emits an `SV_Depth` output for a valid DOT_ZW preceded by DOTPRODUCT. It writes previous/current dot-product ratio, rejects out-of-range (including non-finite) results, and reverses the final depth when the backend uses reversed depth. Ordinary dotproducts and invalid predecessor combinations retain interpolated depth. This follows NVIDIA's [NV_texture_shader depth-replacement specification](https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_shader.txt), section 3.8.13.1.21.

The colour-channel clears in this capture were also inspected: the backend already handles the Xbox F0 channel mask. Old menu pixels outside the small offscreen cleared region do not by themselves establish a clear defect; no clear changes were made.

New native tests generate and compile the captured project2d/dotproduct/DOT_ZW stage pattern, then use D3D shader reflection to verify a depth output in normal and reversed depth modes and quad variants. Ordinary dotproduct and invalid predecessor controls have no depth output. All four runtime regressions pass. Patched EXE/DLL rebuilt without a game launch; the current process was preserved through renamed `Ghoulies.before-effect-depth.*` files. Correct effect occlusion still requires manual validation on the next launch. Shader source changes automatically invalidate the corresponding disk cache entry.

The user identified the death smoke specifically. The available gameplay capture at `runs/20261027-025133-864/025234` contains 68 blended draws: all have depth testing enabled with LESS_EQUAL; 57 disable depth writes. It does not establish a death-smoke frame, so it cannot identify the faulty draw or justify changing its depth test/sorting.

No speculative occlusion change was applied. F3 captures now record depth mode, normalization scale, viewport depth range and shader depth parameters, alongside the existing vertices, constants, shaders and bound surfaces. Missing/incompatible depth surfaces produce warnings. A manual capture while the smoke overlaps scenery/player is needed to discriminate particle projection/offset errors from target binding or intentionally foreground effects. Automatic captures remain off by default.

## Random stutters

Existing logs show synchronous shader compilation coinciding with large frame gaps, including a 776 ms gap near 25.5 seconds. Periodic captures at approximately 30 and 60 seconds also perform synchronous GPU readbacks and texture dumping. Most intervening frames hold 60 FPS.

Normal launches now omit periodic captures, verbose/kernel tracing and WAV recording. Shader bytecode persists across processes in a checksum-validated cache. A source/profile/entry/flags change invalidates the key; corrupt or incomplete files fall back to compilation. This reduces repeat shader stalls, not first-use shader or driver pipeline costs. No asynchronous skipping of draws was introduced.

Without booting the game, the offline utility compiled 113 unique shaders from existing captures and reused 441 repeated entries, with zero failures. A second pass reused all 554 entries without compilation. Native stream/cache regression tests and the full patched build passed. Actual audio recovery, death-smoke occlusion and gameplay frame-time improvements remain manual validation items.
