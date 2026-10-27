# XDK 5659 bring-up

Derived from the retail Grabbed by the Ghoulies `default.xbe`, title 4D530053, SHA1 B119C16A023759628803DCB9EA797D0603E0C4ED. Upstream PRs were checked before implementation; none added 5659 support.

`layout.toml` records proven structure offsets with disassembly locations. `signatures.toml` contains scanner fingerprints, constants and call relationships, with no executable bytes. Empty signatures denote unlinked or unresolved functions. Index-buffer and palette members remain unproven and are omitted; paths that require them retain explicit checks.

The table is a starting point for this title, not a claim of full XDK coverage. To verify against a locally owned image:

```
xbe_symbols path/to/default.xbe xdk --library Ghoulies.dll --out local-game.toml
```

Check resolved functions and generated `HAS_*` values before using another release. Keep the resolved game configuration, image and generated code out of version control. The test project is in `testing/ghoulies`.

Upstream rendering/runtime PR #8 and stream PR #9 were merged earlier. The stream-stop changes adapt PR #3 (2953ea17c94f432530499b0a5145facb18e8a206) to the newer mixer. Menu validation also required BinkWait/BinkNextFrame fingerprints, SetVertexShaderInput handling, CPU vertex conversion for this test title, and preserving small DXT texture dimensions.
