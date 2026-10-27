# Grabbed by the Ghoulies SDK test project

Builds the player's extracted Xbox image against the SDK in this checkout. This is a bring-up project, not a verified working port. No game files or generated code are included.

From PowerShell in this folder:

```powershell
.\setup.ps1 -GameFiles 'H:\GhidraProjects\ghoulies-decomp\gbtg'
.\build.bat
.\run.ps1 -Seconds 120
```

Setup creates an ignored `assets` junction to your extracted game folder. Keep that folder available. The build uses Visual Studio's C++ tools, Clang, Ninja, and network access for pinned dependencies; it scans the XBE and writes `config/game.toml` on the first build. Unsupported XDK builds stop at scanning. Zero or ambiguous symbols require investigation before this can be considered a working port. Keep resolved symbols local until checked for game-derived content.

The local `gbtg/default.xbe` reports XDK **5659** for XAPI, Direct3D, DirectSound and XACT. This SDK currently has only **4831** and **5849** tables. `preflight.ps1` (also run by `build.bat`) deliberately stops until verified `xdk/5659/layout.toml` and `signatures.toml` exist. Do not substitute another build's offsets. The next bring-up work is to derive the 5659 layout and symbol addresses from this image, then verify that the new stream hooks are actually resolved in its generated `image_symbols.h`. PR #9's fallback placeholders alone do not hook unknown symbols.

`run.ps1` records verbose kernel and performance logs, frame captures every 15 seconds, a float WAV, the SDK commit, and the XBE hash in an ignored timestamped `runs` folder. It requests a normal window close after the time limit, then stops a hung process. A zero exit code only means the process closed normally; it does not prove correct rendering or audio. Crash dumps remain beside the executable under `CrashDumps`.

For rendering: check intro movies, menu placement, viewport edges, character lighting, room transitions, and one playable room. Press F3 for an additional capture. For sound: enter gameplay, trigger overlapping effects, music transitions, pause/resume, and a room change. Confirm both audible output and absence of guest faults in the logs. Compare `-NoHrtf` and `-NoReverb` runs separately if sound fails. `-DebugGpu` enables the D3D12 debug layer when Windows Graphics Tools are installed. `-InputScript path` accepts the SDK's controller script format; no gameplay sequence is assumed yet.

For a baseline comparison, use a separate SDK checkout at `0fd0044`, the same game release, resolved config, settings, and manual actions. Run baseline and merged builds separately; this project always uses its own enclosing SDK checkout. The Linux headless harness cannot validate D3D12 rendering or audible output.
