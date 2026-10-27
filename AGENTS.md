# Ghoulies development preferences

- Work on and launch the patched Ghoulies build by default: `testing/ghoulies/out/build/patched/Ghoulies.exe`.
- The selected full mod is `H:/GhidraProjects/ghoulies-decomp/build/ghoulies_walktest.iso`, with its matching XBE in `build/walktest/game`. Include the executable `.ghmod` section in recompilation. Runtime assets must use the full ISO; the extracted mod folder is an overlay and lacks unchanged game assets.
- Keep the cutscene-skip patch enabled. Use the non-patched build only when the user specifically requests it.
- Launch for hands-on testing with physical controller input, without an input script or automatic close. Leave the game open unless the user requests closing it.
