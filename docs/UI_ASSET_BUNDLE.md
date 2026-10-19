# Local HUD, menu and prompt artwork

Current MCC-only onboarding imports game artwork and fonts from MCC. The
port-authored prompt exception is the authenticated91-file source bank at
`resources/presentation/prompts`; only its nine runtime files enter the selected
cache at `presentation/prompts`. This bank includes the earlier82 SVG masters
and six exact bitmap atlases, with no reference fonts. Bundled intro/credits
are the other explicit presentation exception. The combined local Vista/MCC
bundle workflow below describes the older optional pack and is not a current
MCC onboarding dependency.

`resources/ui` is a local, optional bundle excluded by `.gitignore`. A Git
checkout contains the import, redraw and verification tools, but no imported
MCC/Vista artwork or fonts. The existing local bundle contains the complete
imported MCC Halo 2 Classic HUD and menu artwork, all imported fonts and
language tables, and the corrected Vista button redraws.

| Pack | Bundled contents |
| --- | --- |
| `mcc-classic-hud` | 627 bitmap pages, HUD/radar/shield/scoreboard bindings, 35 font files and 5 language tables |
| `mcc-classic-menus` | 938 bitmap pages, Xbox/Vista/pause menu bindings, 35 font files and 5 language tables |
| `vista-prompts-svg-4k` | 82 editable SVG masters, 331 selector bindings, 6 DDS atlases, PNG previews, and the exact 8 Vista reference fonts plus their selector table |

The HUD catalog includes every imported ammo, grenade and crosshair bank.
The prompt pack includes the circular A/B/X/Y redraws, dark LB/RB lettering,
revised Start/Back arrows, triggers, sticks, direction arrows and key frames.
The [prompt generator](VISTA_PROMPTS_4K.md) and MCC import/verification tools
remain in `tools/` alongside the source code.

Creating MCC packs requires a user-supplied MCC installation; see the
[HUD importer](MCC_CLASSIC_HUD_IMPORT.md) and
[menu importer](MCC_CLASSIC_MENU_IMPORT.md). Once prepared locally, the bundle
can be used without that installation. Original Xbox/Vista game-file setup is
still required for gameplay, widget definitions, strings, audio and the
original logical layout owners. Normal setup automatically installs the local
bundle when it exists and there is no existing HUD/menu selection. Without a
bundle, setup uses the supplied Xbox/Vista sources. Existing selections are
preserved. To explicitly select a prepared local bundle after setup:

```sh
python3 tools/bundle_ui_assets.py --verify
python3 tools/bundle_ui_assets.py --install
./Halo2-Launcher --launcher-info
```

Installation verifies every file, then copies complete packs into the ignored
`assets/bundled-ui/<bundle-hash>` directory. It creates local Vista/prompt
bindings only after all copies are ready and selects both packs through the
existing launcher registries. A corrupt bundle does not replace a working
selection. Existing imported packs are retained. The local bundle contains no
machine-specific runtime bindings; historical installation paths within MCC
provenance manifests are evidence only.

`bundle.json` records every bundled file's size and SHA-256. Verification also
checks every MCC DDS/font against its original import hash and every prompt
SVG/atlas/reference font against the redraw provenance. To verify SVG raster
replay and the button design checks independently:

```sh
python3 tools/verify_vista_prompts.py resources/ui/vista-prompts-svg-4k
```

Prompt generation can run from locally bundled references without the original
installer or an MCC installation. A fresh checkout needs user-supplied Vista
fonts, selected with `--vista /path/to/extracted/vista` (the folder containing
`fonts/`). It needs Python with NumPy, Pillow and SciPy,
plus `rsvg-convert`; the runtime and bundle installer do not need those drawing
dependencies. Use a separate output while reviewing a regeneration:

```sh
python3 tools/recreate_vista_prompts.py --output /tmp/vista-prompts-redrawn
```

To assemble an updated bundle from the imported working packs, write a fresh
destination and review it before replacing `resources/ui`:

```sh
python3 tools/bundle_ui_assets.py --output /tmp/halo2-ui-bundle
```

MCC payloads, authored mips and fonts remain byte-for-byte copies of the imports.
Bundling does not increase the resolution of the source MCC pages or change
their ownership. The Vista prompts retain the fourfold redraws validated in
the previous delivery.
