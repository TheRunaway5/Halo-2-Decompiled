# Vista menu assets available in MCC

Audited the installed MCC Halo 2 Classic files on 2026-10-13 against the original Vista menu reference inventory. This is a source-location report for the separate MCC importer work. No runtime import, activation or further artwork redraw was performed.

**558 of 574 unique in-scope Vista menu pages have byte-identical DDS counterparts in MCC**, including their serialized mip levels. The full decoded RGBA hashes also match. Another three server icons have identical RGB values and alpha differences of at most 1/255. Thirteen pages have no exact counterpart in the scanned caches/texture packs.

Emblem customization banks and HUD/button prompts were excluded. Virtual-keyboard keys are menu graphics and remain included, as do controller illustrations and menu status icons. The excluded Vista inventory accounts for 288 unique emblem pages; button prompts were already outside that inventory.

## Results

| Inventory | Exact matches | Total |
| --- | ---: | ---: |
| All unique menu pages, excluding emblems/prompts | 558 | 574 |
| Identified character portraits/composites and related bone illustrations | 21 | 21 |
| Previously unfinished illustrations | 50 | 50 |
| Previously unfinished map screenshots | 46 | 46 |
| Previously unfinished panels/linework | 20 | 20 |
| Previously unfinished game symbols | 38 | 38 |
| Vista executable-owned loading textures | 0 | 12 |

The last five rows partition the 166 previously unfinished pages outside emblems: **154 are already present exactly in MCC**, leaving the 12 loading textures unmatched in this scan. The 21 portrait studies are a subset of the illustration row, not additional coverage.

The 408 canonical authored menu masters also correspond to 404 exact MCC pages, three server-icon alpha variants and the one missing server-icon state. They were preserved; their recreation status was not changed by this audit.

Confirmed originals include the Halo 2 title logo, rank banks, header/footer Forerunner linework, live/status icons, arrows, shield/controller/speaker/crown icons, download-progress states, dialogs, lists, keyboard keys, map previews, Spartan/profile portraits, skull/flag illustrations and matchmaking scenes. Their matching Classic pages have the same dimensions as Vista; no higher-resolution assumption is needed to use their original artwork.

## Importer inputs

The installed Classic source directory is:

```text
<MCC>/halo2/h2_maps_win64_dx11/
```

Bitmap metadata comes from the named `.map` caches; compressed image payloads come from `textures.dat`. Of the 558 exact unique pages, the first verified source owner is `mainmenu.map` for 545, `00a_introduction.map` for nine, and `ascension.map`, `dune.map`, `lockout.map`, and `midship.map` for one each. The report lists every additional cache that shares each bank.

Machine-readable evidence is under `analysis/frontend/vista-menu-vectors/mcc-menu-match-audit-20261013-final/`:

- `importer-map.tsv`: 661 distinct Vista bank/frame owners, with source cache, MCC bank/frame, physical dimensions, DDS SHA-256 and `textures.dat` address. There are 645 exact DDS rows, three alpha variants, and 13 unmatched rows. Repeated bitmap content is counted once in the 574-page summary.
- `report.json`: all 43 Classic cache hashes, 289 distinct MCC menu banks, full resource identities, page matches, decoded differences, and bank compatibility evidence.
- `portrait-matches.png`: decoded MCC artwork for all 21 identified portrait/related illustration pages.
- `font-comparison.json`: original Vista versus installed MCC font-file identities.
- `embedded-header-probe.json`: bounded BMP/DDS header search in `halo2.dll` and eight Classic GFX files; no valid standalone headers found. This does not prove absence of differently encoded embedded artwork.

Across 287 distinct Vista menu-bank variants, 286 preserve the recorded page counts, sequence metadata and registration points. The remaining bank is the dedicated-server icon bank described below. Matching pixels do not authorize changing logical dimensions or page/state ownership.

The source audit also inventoried all 13 MCC frontend texture packs and decoded 4,171 non-emblem/non-prompt textures without errors. No additional exact Vista page matches came from those packs. This exact-match result does not exclude differently cropped, recolored or higher-resolution design relatives. The previously inspected frontend rank art remains an optional design reference: it adds numerals and changes some styling, so it is not an exact Vista replacement.

## Exceptions

`ui\screens\game_shell\network_squad_browser_screen\ds_icon` has four Vista frames but only three MCC frames:

| Frame | MCC availability | Difference |
| --- | --- | --- |
| 0 | Present, 32 × 32 | RGB exact; alpha differs by 1 in 204 pixels |
| 1 | Present, 32 × 32 | RGB exact; alpha differs by 1 in 220 pixels |
| 2 | Present, 32 × 32 | RGB exact; alpha differs by 1 in 216 pixels |
| 3 | No counterpart found | Preserve as an explicit missing state |

Do not reject the three available pages solely because the bank count differs, and do not guess a mapping for frame 3. The existing all-or-nothing bank importer may require a measured per-page exception for this bank; this audit does not implement it.

The twelve `executable\loading\0` through `executable\loading\11` pages are Vista's dedicated loading bank. None matched the scanned MCC menu caches or frontend texture packs. Their absence from those sources is not proof that no equivalent exists anywhere in the installation. Do not silently substitute an unrelated MCC loading screen or claim an upscale is new source detail.

One related colour layer is subsequently verified: **loading page 1 has
byte-identical RGB pixels to MCC's
`ui\screens\game_shell\start_screen\start_screen[0]`**, at the same 1024×128
extent. Its alpha differs at 46,707 pixels, by up to 198/255, and a single
alpha-channel lookup table cannot reproduce the target. The source relationship
is recorded in `loading-logo-source-relationship.json` beside the audit report.
This is original MCC colour artwork that may help reconstruct the loading
variant; it is not an additional exact DDS/RGBA match or a recovered alpha rule.

## Fonts

All eight primary Vista font filenames have installed MCC counterparts in `<MCC>/halo2/h2_fonts/`, and `font_table.txt` is byte-identical. `fixedsys-9` and `MSLCD-14` are byte-identical to the Vista references; the six Conduit/Handel Gothic files differ and should be imported as MCC's own font banks rather than claimed as exact Vista font bytes. On case-sensitive hosts, Vista's `mslcd-14` corresponds to MCC's `MSLCD-14`.

MCC also supplies Chinese, Japanese and Korean font banks/tables. This audit locates them; it does not claim every glyph metric or rendering path has been compared.

## Reproduce

```sh
python3 tools/audit_mcc_menu_matches.py \
  --output /tmp/halo2-mcc-menu-audit-fresh
```

The output must be a fresh directory. An alternate installed source can be supplied with `--mcc-root`. The tool writes original DDS/PNG files into its reference-only output, never into the authored master bundles or runtime selection registries. The font comparison and embedded-header probe are separately retained supplementary evidence.

The audit completed with zero decoding errors. Original source files were opened read-only. The canonical master manifest SHA-256 remained `e2ea58b2d85d3a8c6dd62c15b7e84263e7e6a6b269a3a97f7c6edad6af0af4a0`; the separate experimental portrait-study manifest remained `8d3c4351527a7c6faf618b5e5d5ff8282ca44450f01091fd72de7c18ff1bdd01`.

These findings support an importer that reads assets from the user's local MCC installation. The audit is not a determination of redistribution rights and does not add retail assets to a distributable source-art pack.
