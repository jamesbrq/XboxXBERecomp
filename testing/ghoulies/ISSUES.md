# Audio, particle depth and stutter investigation

Work performed without a game launch or boot test.

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
