# MCC game-file setup

Both launchers select the private MCC import; **MCC is the sole permitted
runtime asset donor**. Normal startup no longer reads Xbox or Vista source
registries, supplies an Xbox executable, or repairs an old Vista loading bank.
The original Xbox ISO and TU1.5 patch remain reference material
for development and validation. The user's October21 reaffirmation also
supersedes earlier supplied-movie, custom prompt-atlas and original loading-tone
exceptions. Preserve those files as references; they are not normal import,
reuse, package or runtime dependencies.
Retained original-donor native shader/program payloads remain a
[D01 provenance blocker](../multiplayer/docs/online-completion-goal.md);
selecting an MCC cache does not establish complete MCC-derived payload ownership.

Run `./Halo2-Launcher`. First launch opens MCC setup and searches installed
Steam libraries, including configured external libraries. Windows discovery
also checks Steam registration and standard Xbox Games installation folders.
`HALO2_MCC_ROOT` supplies a preferred installation. If detection cannot find
Halo 2 Classic, browse to the MCC installation, its `halo2` folder, or its
`h2_maps_win64_dx11` folder. Choose **Prepare MCC cache**.

Terminal preparation works without Tk:

```sh
./Halo2-Launcher --prepare-mcc-gameplay
# Nonstandard installation:
./Halo2-Launcher --setup --mcc-source '/path/to/Halo The Master Chief Collection' --setup-only
# Report the selected cache and remaining runtime blocker:
./Halo2-Launcher --launcher-info
```

Setup copies MCC content into `assets/imported/mcc-<id>/halo2/`, preserving
relative filenames, case, empty directories, and any `tags/` and `data/` trees
inside the selected donor. It retains textures, bump maps, audio, fonts, menu
resources and map content. Programs and shader binaries/caches are excluded;
existing port shaders are preserved. Separate external Workshop/mod folders
are not automatically imported by this installation scan.

Normal source preparation selects the authenticated MCC V3 loading bank,
including decoded MCC audio and its MCC-derived bitmap/mask owners, and native
MCC control-font glyphs. Old Vista loading banks, supplied tones and separate
prompt atlases are not fallbacks. These source/component checks do not qualify
a fresh complete import, delivered loading interval or physical playback; see
[loading presentation](MCC_LOADING_PRESENTATION.md). Authentic MCC Classic
intro/credits/attract roles remain unassigned and unavailable.

Imports are staged, verified and published before `assets/mcc-source.tsv` is
replaced atomically. Failed imports preserve the previous selection. Setup
reuses an existing private import only after checking the full copied file
inventory, directory tree, donor hashes, converted output hashes and installed
source snapshot. It never mounts the live MCC installation as the runtime
asset owner. Original files and earlier imports are preserved.

**Preparing a cache does not qualify complete gameplay.** A separately admitted
MCC frontend may enable title/menus, and a separately admitted native test may
enable its bounded Lockout route. An invalid explicit frontend selection fails
closed; updates do not bypass those checks. Full gameplay/parity, complete
material/shader and movie ownership, and fresh package/launcher acceptance remain
open. There is no automatic Xbox/Vista fallback. See
[MCC gameplay conversion](MCC_GAMEPLAY_CONVERSION.md) for implementation and
validation boundaries.

Once completed and verified, converted definitions must enter the normal
vanilla internal tag path with their authored identities. MCC remains an
offline donor, rather than a mod overlay. Conversion must preserve OG Xbox
TU1.5 gameplay/visual behavior and the existing shaders.

Historical reference import helpers remain available to developer audits.
They are not normal onboarding inputs. The launcher rejects `--xbox-source`,
`--vista-source`, `--gameplay-xbe`, `--menu-vista-settings`, and the old separate
HUD/menu import switches. Developer/reference native flags do not qualify a
delivered route. Use the MCC setup command above.

The setup and home windows require Python 3.11+ with Tk. They use hand-drawn
Canvas geometry and system fonts. `python3 tools/setup_assets_ui.py` previews
the real setup controls without importing files. `--setup-only` exits after
preparing the cache. `--update` retains the existing update/build workflow;
readiness is checked again before allowing Play.
