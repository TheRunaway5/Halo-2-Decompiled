# MCC game-file setup

Both launchers select the private MCC import; **MCC supplies the stock runtime
assets, supplemented by the port's authored custom content**. Normal startup no longer reads Xbox or Vista source
registries, supplies an Xbox executable, or repairs an old Vista loading bank.
The original Xbox ISO and TU1.5 patch remain reference material
for development and validation. The current custom UI, controller artwork,
maps, game modes, and 4K60 intro and credits are included with the port.
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

Builds and Linux/macOS onboarding packages stage the authored sources through
`tools/custom_assets.py`. Its receipt lists the verified files for both
Xbox/PlayStation button banks, the custom UI masters, the bundled map catalog,
the native game-mode definitions, and both 4K60 movies. Missing or changed required inputs stop
staging instead of silently dropping content. Source builds refresh this stage
on asset-only changes as well as engine changes. Disposable component-test
builds omit the large source copies by default; production engine qualification
explicitly includes them with `HALO2_STAGE_CUSTOM_ASSETS=ON`.

Setup installs the custom button maps and celestial rank textures into the
private cache. Ordinary rank pages retain their MCC owner; the moon phases,
eclipse, sun, comet and Halo ring use the existing custom DDS artwork in both
rank banks. These bindings follow the original bitmap slots and geometry.
All authored UI masters are included as sources; incomplete menu redraws do
not automatically replace the rest of the imported menu.

Fresh preparation, cache reuse, cache selection and Repair all install both
port-authored button banks. Xbox, PlayStation, shared prompt symbols and keyboard
keycap frames sample these banks. MCC fonts supply ordinary text and logical
metrics; their embedded button pixels are not a rendering fallback. Missing
owned prompt artwork requires Repair. Newly qualified frontend receipts pin
both banks as required resources.

The launcher home exposes **Rebuild Cache** beside **Play**, even when the
current cache needs repair. Rebuild always imports a new private cache from
MCC and installs the full current custom bundle. It keeps the previous cache
and runtime. When a frontend runtime is installed, real title/menu captures
must pass before selecting the replacement. A cache-scoped capability binds
that executable to the rebuilt resources without replacing its existing
startup proof. Failure or cancellation keeps the previous selection and removes
the incomplete new cache and temporary captures. Small startup diagnostics
remain under `diagnostics/cache-rebuild-<id>/`.

Rebuild validates the saved MCC source before using it. If MCC moved or that
folder no longer contains Halo 2, it scans the current Steam libraries instead.
**Choose MCC folder** always takes precedence over a saved location. Repair
uses this same fresh rebuild and startup check when the existing cache cannot
be repaired. Import and startup validation use the same selected executable.
The title and menu checks always wait for the native map intro to clear.
Rebuild also checks playback of the bundled intro before selecting the cache.

Packages ship `resources/movies/intro_60.mp4` and
`resources/movies/credits_60.mp4` at their original 3840×2160 and 59.94 fps.
Setup and Rebuild Cache verify their SHA-256 hashes and copy them byte for byte
into `presentation/movies/` in the private cache. Budget 2.54 GB for the two
files in both the package and the private cache.

`resources/custom-asset-bundle.json` is the authoritative source contract for
required maps, game modes, prompt families, celestial slots and artwork policy.
Dropping a required map from the delivery catalog stops the build/import.
`custom-asset-cache.json` is the required installed bundle receipt. It binds
both prompt families, all fourteen celestial pages, the three authored game
modes and Bomb Defusal layouts, map bindings, the map delivery receipt and
their source identities. The home checks this receipt before enabling Play.
Missing, changed or outdated custom assets require Rebuild Cache. The source
delivery receipt (`custom-assets.json`) carries the same explicit artwork
policy: authored button pixels and celestial ranks, with no MCC button fallback.

`resources/custom-maps.json` lists compiled maps for automatic delivery.
This includes Dire Valley (the Danger Canyon remake) and Deserted v9, with
their menu cards, previews, native resource owners and textures. Deserted v9 is
the user-approved temporary compiled preview; its final interior-lighting/art
acceptance remains open. Its exact source hash and component review are kept in
`assets/custom-maps/deserted/menu/source.json`. Replace that complete source
cohort after final polish; packages automatically consume the registered version.
Uncompiled Dust2 remains authoring work. Setup preserves an older import when a new map catalog
requires a fresh cache. Fiesta Slayer, Infection Slayer and Bomb Defusal,
including its map layouts, are installed through the standard native tag path.

The loading-image inventory retains all five MCC banks and all eleven pages,
including their actual datums, flags, resident mip counts and byte hashes. The
native loading adapter consumes these copied MCC owners. When the installed
Halo 2 Editing Kit is available, setup also stages the original loading screen
under `presentation/original-loading-v2`: the six used `loading_dx11.bin` art
slots and the four-voice loading ambience. The importer checks the pinned
`loading_dx11.bin` and its DDS extents, a pinned tool executable and its
`.rdata` extent, the encoded ADPCM hash and the decoded PCM identity, without
executing the tool. `HALO2_H2EK_ROOT` selects a nonstandard kit location.
Packages include the source policy, while setup reads the user's installed kit.
The campaign logo is the authentic 1024×128 DXT3 Vista artwork in slot1, pinned
by dimensions and SHA-256 in both the importer and native reader. It replaces
the lower-resolution 512×64 MCC loading-bank logo and retains its aspect ratio.
When present, this bank is the selected logo/sweep loading presentation. Missing
kit data is reported as unavailable, and MCC V3 loading remains selected. An
earlier ambience-only `original-loading-v1` bank is kept as a reference. No
Xbox installation is an onboarding input.

Windows resource reads and donor discovery also recognize Wine's synthetic
host-mount metadata. The exception requires Wine's native runtime export, a
mount-point tag and `FSCTL_GET_REPARSE_POINT` reporting no actual reparse data.
Real directory junctions and symbolic links remain rejected. This prevents
Proton's `Z:\home` mount from stopping valid onboarding or game startup.

### Install missing Steam content

The graphical setup checks the actual MCC Halo 2 campaign/multiplayer files
and Halo 2 Mod Tools (H2EK). Installing the MCC shell or retaining an app manifest
alone does not count as installed Halo 2 content. Missing content is named in
the setup window, which offers **Download from Steam** on Linux and Windows.
Preparation stays disabled after a check finds missing or unfinished content.

Download hands only the missing installation requests to the user's installed
Steam client: MCC (`976730`), Halo 2: Anniversary campaign DLC (`1064270`),
MCC - H2: Multiplayer DLC (`1097223`), and Halo 2 Mod Tools (`1613450`). Steam
handles ownership, its installation prompts, library location and download
queue. Confirm the prompts and enable the named Halo 2 DLC entries if Steam
asks. The launcher does not sign into Steam, change its manifests or claim
that a request means a completed download. Setup checks again every two seconds
while waiting and enables preparation when the content is found and Steam has
finished installing it. Closing setup leaves Steam's downloads running.

Both applications share bounded Steam library discovery, including configured
external libraries, nonstandard Windows registry locations, Flatpak Steam and
the native host libraries when running the Windows launcher through Proton.
Game Pass discovery remains available through normal source discovery/manual
selection; Steam installation checks do not scan unrelated Xbox drive roots.

A newly downloaded `H2EK.7z` counts as a downloaded tool installation. When its
tool and loading files have not been extracted, setup explains the kit's
Extract step instead of requesting another download. The launcher does not run
the kit's batch file or modify existing kit files. CLI import and its verified
MCC fallback remain available independently of the graphical prerequisite check.

The application identities and DLC/tool distinction follow the
[Halo 2 Steam page](https://store.steampowered.com/app/1064270/),
[multiplayer DLC support entry](https://help.steampowered.com/en/wizard/HelpWithGame/?appid=1097223)
and [official mod-tools instructions](https://learn.microsoft.com/en-us/halo-master-chief-collection/general/overview-tools).
Steam documents its [DLC installation controls](https://help.steampowered.com/en/faqs/view/6948-51E6-E624-8A84).

October29 validation covers15 Steam discovery/dispatch controls,36 shared setup
controls (one platform skip),24 preparation controls,41 onboarding controls,
12 H2EK source controls and three Windows resource-collection controls. All
three focused CTest groups pass. Native Windows Python/Tk under GE-Proton11-7
passes51 cases with eight platform skips, correctly detects the installed host
MCC/H2EK sources and renders the missing-content prompt. Steam dispatch is
intercepted in tests; no real downloads were queued. The Windows package's four
updated Python modules match source; its native engine and shader bank remain
in place. [Validation and cleanup receipt](../build/test-results/steam-onboarding-20261029/validation.json)
and [delivery inventory](../build/test-results/steam-onboarding-20261029/delivery.json)
retain the evidence. The120,173-byte `windows-download.png` preview is retained
for layout review; private tests/builds and the temporary helper were removed.
No large validation artifacts are retained.

Source preparation retains the authenticated MCC V3 loading bank, including
decoded MCC audio and its MCC-derived bitmap/mask owners. Installed H2EK loading
art takes priority when available. Button and keycap rasters come from our
authored banks; the copied MCC fonts retain ordinary text and metrics. Supplied
Xbox/Vista reference assets are not onboarding inputs. These checks do not qualify
a fresh complete import, delivered loading interval or physical playback; see
[loading presentation](MCC_LOADING_PRESENTATION.md).

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
material/shader ownership and fresh package/launcher acceptance remain
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
