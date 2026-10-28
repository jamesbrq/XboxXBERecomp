# Grabbed by the Ghoulies SDK test project

Builds the player's extracted Xbox image against this SDK checkout. XDK 5659 bring-up is experimental; no game files or generated code are included.

The current local patched build targets the full `ghoulies_walktest.iso` mod. Its matching mod XBE is the compile input, including the executable `.ghmod` section; the runtime loads the complete ISO from `out/build/patched/assets`. The extracted mod folder contains overlay files only and cannot serve as the runtime's complete game folder. Previous cutscene-only source/configuration backups remain under ignored `out/backups`.

The selected full-mod XBE SHA1 is `7193b60bc75240bdc0e75e184bbbb22b56e8e397`. `config/fullmod-functions.inc` adds its 121 named assembly routine entries, checked against the mod builder, including indirect transition and special-weapon callbacks. Its assembly helpers also require condition flags to survive guest calls and returns; the recompiler now exchanges those flags through the guest context. A synthetic regression (no game data) exercises direct, indirect and tail calls, plus flag inputs to helpers:

```powershell
python ../recomp/call_flags_test.py --compiler out/build/patched/compiler/xbe_recomp.exe --out out/call-flags-test
```

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

`run.ps1` defaults to ordinary manual play: no periodic captures, kernel tracing, verbose reports or WAV recording. F3 still saves a frame. Use `-Diagnostics` for verbose/kernel/performance logs, `-AudioDump` for a float WAV, and `-CaptureEvery 30` only when periodic GPU readbacks are wanted. These diagnostics can introduce stutters. The run folder records SDK revision/dirty state, executable/config hashes and XBE identity when the game exits. Only `-Timed` requests a close after the time limit. Crash dumps remain beside the executable in `CrashDumps`.

Compiled shaders persist under `out/build/patched/cache/shaders-v1`. Source, entry point, shader profile and compiler flags determine each key; invalid/truncated cache files are recompiled. This avoids repeated shader compilation on later launches, but previously unseen shaders and driver pipeline creation can still stall. Existing captured HLSL can be compiled into this cache with `testing/runtime/warm_shaders` without starting the game or creating a graphics device.

Stopped audio streams are rearmed on an explicit resume/synchronized start, after retiring late packets from the old cue. Late packets alone still cannot restart stopped music. The green death-smoke occlusion issue remains unconfirmed: captures now include particle depth scale/range/offset values, and incompatible/missing depth targets produce bounded warnings. See [ISSUES.md](ISSUES.md) for evidence and remaining validation.

Check intro movies, menu lighting, character lighting, room transitions and a playable room. For sound, test overlapping effects, music changes, pause/resume and room changes. Compare `-NoHrtf` and `-NoReverb` separately when diagnosing sound. `-DebugGpu` uses the D3D12 debug layer if Windows Graphics Tools are installed. `-InputScript path` reads an inline SDK controller script from that file.

Local validation reached the menu with stable geometry, visible shaded book covers and a normal exit. Gameplay and music transition behavior remain manual validation items.

`build.bat` and `run.ps1` use `out/build/patched` by default. `run.ps1` waits until you close the game yourself; use `-Timed -Seconds 120` for an explicitly timed test, or `-Build release` only for a requested non-patched comparison. Omit `-InputScript` to use a physical controller. Scripted tests deliberately override physical controllers. Immediate-mode UI colors are retained across Begin/End, fixing white save-slot/name-entry boxes.

Synthetic vertex and small DXT regression tests (no game files):

```powershell
cmake -S ../vertex -B out/vertex-tests -G Ninja -DCMAKE_CXX_COMPILER=clang++
cmake --build out/vertex-tests
ctest --test-dir out/vertex-tests --output-on-failure
```

Stream lifecycle and persistent shader-cache tests are in `testing/runtime`; configure/build that directory and run CTest. They do not launch Ghoulies. On Windows its `warm_shaders` utility also validates actual HLSL compilation and cache round-trips from existing captures.
