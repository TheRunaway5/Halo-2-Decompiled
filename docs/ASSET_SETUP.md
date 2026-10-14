# Game-file setup

The latest requested migration uses **installed MCC data with Xbox TU1.5
conversion and the native renderer**. Its implemented preparation and remaining
runtime/graphics work are documented in [MCC gameplay conversion](MCC_GAMEPLAY_CONVERSION.md).
Run `./Halo2-Launcher --prepare-mcc-gameplay` to discover and privately copy MCC
data without an ISO. This prepares an inactive cache; ordinary launch still
uses the existing Xbox gameplay path below.

The MCC reader now uses compiled immutable TU1.5 engine operands and loads
content from all 43 MCC maps without an Xbox executable argument. Mutable
world startup, complete gameplay-tag conversion and normal MCC runtime
activation remain unfinished. The Xbox ISO/TU1.5 patch are reference-only
inputs for the requested replacement; the legacy launch path below remains
documented because it is still the currently selected runtime.

The earlier Vista migration's preparation step and remaining runtime
work are documented in [Vista gameplay conversion](VISTA_GAMEPLAY_CONVERSION.md).
The active launcher still uses the working Xbox gameplay path described below;
the prepared Vista conversion is not yet activated.

First launch requires two user-supplied sources:

* **Halo 2 Vista installer**: the inspected installer ZIP, `h2pc_data.bin`,
  its setup EXE alongside that data archive, or an already extracted folder.
  Supplies HUD/menu artwork, compiled fonts, original match announcer audio,
  and the loading textures/ambience from `loading.bin` and `halo2.exe`.
* **Halo 2 for Xbox**: the original retail ISO or a complete extracted disc
  folder. Supplies gameplay caches, models, world/weapon textures, material
  definitions, audio, movies and original executable reference data.

Run `./Halo2-Launcher`, select both in the first-run window, and choose
**Prepare game files**. The launcher then configures and builds the runtime
with CMake and starts the game automatically. Build progress and compiler
output appear in a window and are saved to `build/launcher-build.log`.
An incompatible build cache left by moving or copying the checkout is
preserved in `build.previous` before a clean build is configured.
The installer executable is never run. Subsequent launches use the saved
selection and built runtime. Selecting new sources rebuilds the runtime.
The setup window uses hand-drawn vector lettering, orbital geometry, and blue
menu panels. It loads no game artwork, image files, or bundled fonts. Both
launchers use the same window; `python3 tools/setup_assets_ui.py` opens a visual
preview that cannot import files.
The window needs Python 3.11+ with Tk; the supplied installer data format needs
7-Zip (`7z` or `7zz`). Terminal setup works without Tk:

```sh
./Halo2-Launcher \
  --vista-source '/path/to/h2pc_installer_2.1.zip' \
  --xbox-source '/path/to/Halo 2.iso'
```

`--setup` reopens selection and rebuilds before launch. `--setup-only` prepares
files and exits. `--launcher-info` reports saved roots and readiness.
Help works before import. Selecting an extracted Xbox folder avoids another
disc copy; it remains a read-only reference and must remain available. An ISO
is extracted into a new private import directory.

A supported user-owned Xbox update can be selected independently:

```sh
python3 tools/select_gameplay_source.py '/path/to/update/default.xbe'
./Halo2-Title-Launcher --launcher-info
```

This validates the update SHA-256, leaves the original disc executable untouched,
and atomically records its external path in `assets/gameplay-source.tsv`. The
launchers pass it into multiplayer after the normal title/menu flow. Explicit
`--gameplay-xbe PATH` takes precedence. Missing or malformed saved selections
stop launch rather than silently changing gameplay versions. Without a saved
selection, the disc executable is the default. Unsupported updates are rejected
without changing the previous selection. Complete native update parity remains
unfinished.

The revised user-owned Xbox Tombstone cache can be added separately:

```sh
python3 tools/install_tombstone.py '/path/to/owned/highplains2.map'
```

This verifies the exact revised cache and installs a private copy in the selected
Xbox maps directory. Restart the game to refresh installed-map capabilities. The
map is required by the [expanded Tombstone rotations](../multiplayer/docs/tombstone-playlist-coverage.md);
the older `highplains.map` revision cannot substitute for it.

## Artwork and layout

The policy is **Vista artwork with the current layouts**. Complete bitmap
banks must match exact tag paths, frame counts, effective sprite UVs and
registration. Page proportions must match at a consistent 1x or 2x density.
The runtime changes uploaded pixels while retaining Xbox widget geometry,
logical texture extents, sprite selection, font metrics, spacing and animation.
HUD fonts use the existing menu font sampling adapter. Incompatible sprites
retain Xbox artwork; their repacking is not guessed. Frame seams retain the
existing native extension path. Each import report lists coverage and exclusions.

The supplied Vista map catalog and fonts are retained. Compatible UI pages keep
their source DDS format and mip chain. The selected scenario cache owns its
original multiplayer response bank; `shared.map` owns its announcer sounds.
The main-menu cache also supplies source-qualified Vista settings. Existing
menu routes and settings support remain; setup does not adopt different Vista
layouts. No Vista campaign, multiplayer level or weapon/model cache becomes a
gameplay source.

Imports made before 2026-10-11 discarded the match audio owners. Repeating setup
with the same original sources creates a complete new import and preserves the
previous one. The launcher checks the default map and shared bank before launch.

Vulkan shaders are independently implemented in this project's `shaders/`
GLSL and compiled by the build. Original Xbox material definitions and observed
hardware behavior are their reference. Setup does not select Vista shader
programs or import shader binaries into Vulkan. This policy does not establish
complete shader appearance or gameplay parity: the native renderer and game
simulation still have documented implementation gaps.

## Storage and validation

Imports are staged under `assets/imported/.staging-<id>`, validated, then
published under `assets/imported/<id>`. `provenance.json` records source paths,
source/archive and asset hashes, admitted banks, exclusions and policy.
`assets/asset-sources.tsv` is replaced atomically only after success. Failed or
cancelled setup cannot select incomplete files or overwrite the prior registry.
Original inputs and older imports are preserved.

The Vista import includes `loading/`: twelve DDS textures, their manifest,
and the original loading ambience. Preparation verifies every texture payload
against the captured Microsoft D3DX9_31 output and validates the source bank and
executable fingerprints. It requires no DirectX or Wine. NumPy can accelerate
this step when installed; a matching standard-library decoder is included.
The launcher checks the complete loading bank before starting the game.
Older imports missing it are repaired automatically from the installer path
saved in their provenance, without importing the Xbox disc or Vista maps again.
If that installer was moved or removed, source selection opens again. A complete
legacy export under `assets/menu-ui/vista/loading` also remains supported.

MCC-only onboarding supersedes the older Vista loading-bank selection.
`tools/prepare_mcc_gameplay.py` exports the seven MCC Classic loading bitmap
banks (13 frames) and preserves independently authored loading textures. It
never carries the original Vista executable loading images or ambience into a
new MCC import. The OG Xbox ISO and TU1.5 patch are reference-only inputs.
Full native MCC gameplay/conversion remains unfinished; see
[MCC conversion status](MCC_GAMEPLAY_CONVERSION.md).

The supported Xbox cache build is `02.09.27.09809`, with the inspected original
retail executable fingerprint. The supported Vista donor build is
`11081.07.04.30.0934.main`. Unsupported or truncated inputs are rejected.
The supplied Vista ZIP includes Cartographer components; its provenance as an
unmodified stock release is unverified. Selecting it does not assert that its
assets are Xbox-identical.

Direct `halo2_native` commands remain developer entry points. Pass
`--menu-maps <xbox>/maps`, `--menu-ui-assets <vista>` and
`--hud-ui-assets <vista>` as appropriate. Explicit empty UI paths select the
original Xbox artwork for diagnostics.

## Verified delivery, 2026-10-10

The supplied nested Vista ZIP and original Xbox ISO completed a real import.
A separate import checked the solid-archive allowlist in one pass and an
extracted Xbox folder as a read-only donor. Coverage was 213 menu banks / 757
pages and 49 HUD banks / 175 pages. Of 70 original shared HUD banks, 20 retain
Xbox artwork for different page dimensions and one for differing sprite/bank
correspondence.

The optimized `build-asset-setup` launchers were installed with both desktop
shortcuts. Verification passed 52 launcher routing/setup cases, ten focused
CTest checks (including five importer boundary cases), prepared-pack decoding
and menu painting, and 1,518 unchanged HUD vertex positions across 1–4 players
at 640x480 and 1280x720. Native title, one-player and four-player captures ran
with Vulkan validation enabled. The Vista shield framebuffer test preserved
165,144 transparent samples and rendered 102,888 visible-fill samples.
Evidence and captures are under `analysis/asset-setup/`. These checks cover
setup, sampling and bounded native rendering; they do not prove complete
campaign, multiplayer behavior or original Xbox visual parity.
