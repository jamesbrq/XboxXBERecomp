# Grabbed by the Ghoulies SDK test project

Builds the player's extracted Xbox image against this SDK checkout. XDK 5659 bring-up is experimental; no game files or generated code are included.

From PowerShell in this folder:

```powershell
.\setup.ps1 -GameFiles 'H:\GhidraProjects\ghoulies-decomp\gbtg'
.\build.bat
.\run.ps1
```

Setup creates an ignored `assets` junction. The build needs Visual Studio C++ tools, Clang, Ninja, and network access for pinned dependencies. It scans the XBE into ignored `config/game.toml`; keep the `.example` template outside the runtime compiler's `*.toml` search. Preflight requires tables for the image's actual XDK version.

The verified local image is title `4D530053`, XDK **5659**, SHA1 `B119C16A023759628803DCB9EA797D0603E0C4ED`. The 5659 tables contain function fingerprints and layout offsets derived from that image. Unlinked or unproven entries remain unresolved; this is not complete support for every 5659 title. See `../../xdk/5659/README.md`.

This project enables CPU vertex conversion to avoid observed corruption in the raw vertex path. The renderer expands small DXT textures into their original dimensions instead of padding them, preserving the two-pixel lighting ramps used by the menu. The intro-to-menu crash was an unresolved BinkWait hook. Asynchronous stream stops and status reporting adapt upstream PR #3 to the merged PR #9 mixer; music stopping still needs gameplay transition checks.

`prepare-config.ps1`, run by `build.bat` after scanning, adds the confirmed gameplay callback at `0x7FFB0`. It is installed dynamically by `0x7FE10` and was missed by automatic discovery, causing an indirect-call trap from `0x72AD2` during gameplay. When building directly with CMake after a new scan, run `prepare-config.ps1` first. The override is kept in `config/extra-functions.inc`, outside the compiler's full-config search.

`run.ps1` records logs, frame captures (`-CaptureEvery 5` for shorter intervals), a float WAV, SDK revision/dirty state, executable/config hashes, and the XBE hash in ignored timestamped `runs` folders. It requests a normal close after the time limit, then stops a hung process. A normal exit does not prove correct gameplay or audio. Crash dumps remain beside the executable in `CrashDumps`.

Check intro movies, menu lighting, character lighting, room transitions and a playable room. For sound, test overlapping effects, music changes, pause/resume and room changes. Compare `-NoHrtf` and `-NoReverb` separately when diagnosing sound. `-DebugGpu` uses the D3D12 debug layer if Windows Graphics Tools are installed. `-InputScript path` reads an inline SDK controller script from that file.

Local validation reached the menu with stable geometry, visible shaded book covers and a normal exit. Gameplay and music transition behavior remain manual validation items.

`build.bat` and `run.ps1` use `out/build/patched` by default. `run.ps1` waits until you close the game yourself; use `-Timed -Seconds 120` for an explicitly timed test, or `-Build release` only for a requested non-patched comparison. Omit `-InputScript` to use a physical controller. Scripted tests deliberately override physical controllers. Immediate-mode UI colors are retained across Begin/End, fixing white save-slot/name-entry boxes.

Synthetic vertex and small DXT regression tests (no game files):

```powershell
cmake -S ../vertex -B out/vertex-tests -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build out/vertex-tests
ctest --test-dir out/vertex-tests --output-on-failure
```
