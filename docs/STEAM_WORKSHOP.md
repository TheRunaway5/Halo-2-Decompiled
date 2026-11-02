# MCC Steam Workshop mods

The source frontend has a **Mods** entry before **Exit Game**. It scans Steam's
configured libraries for `steamapps/workshop/content/976730`, reads local
`ModInfo.json`, and lists only `Engine: Halo2` subscriptions. Campaign mission
order comes from `CampaignInfo.json`; multiplayer maps use their authored JSON
descriptors. Refresh rescans without changing Steam subscriptions.

Choose a mod, then a mission, then **Play Campaign**, **Play Local Multiplayer**,
or **Explore Map**. Campaign play uses the existing difficulty picker. Explore
Map opens the existing free-camera MCC content preview; it does not start the
campaign or its AI. First preparation runs in a worker while the menu displays
progress. Preparation errors and missing script support return a menu dialog.

## Discovery and preparation

`tools/steam_workshop.py` reuses onboarding's `steam_client.libraries`, including
external libraries, Linux/Flatpak/macOS Steam paths, Windows registry paths, and
Proton's host path translation. `HALO2_WORKSHOP_ROOT` can name an explicit MCC
Workshop content directory (the directory containing numeric item folders).
No Steam account, API key, download, or Workshop manifest mutation is required.

Each selected mission gets a private, content-addressed preparation under
`SDL_GetPrefPath("Halo2Native", "Halo2")/workshop`. Tests use
`HALO2_TEST_SCRATCH/workshop`. Updating a subscription or the base cache creates
a different preparation. Existing prepared owners are verified before reuse;
unknown or changed files are retained and reported, never silently repaired.

The producer copies the selected map and MCC resource maps, retains authenticated stock fonts, and
builds a separate bank of the existing native template/pass/program definitions.
The mod's material instances are retained. Embedded texture/sound containers
use hardlinks to that private map copy. Compressed maps materialize their decoded
container so embedded resource offsets retain their original meaning. The
source and final hashes, encoded-to-decoded change, native-bank ownership, and
source dependencies are recorded in the normal prepared-cache provenance.
The metadata-authorized cache filename may differ from its embedded scenario
name. E3's low-spec cache intentionally shares `earthcity_e3`'s internal path;
preparation retains that path and never renames the authored scenario.

The authenticated Workshop producer opts out of stock gameplay authoring
corrections, stock weapon/vehicle balance projections, and stock script subtree
replacements. Pointer/layout normalization and checked material conversion still
run. Native engine behavior itself is not a complete MCC engine implementation.

Current preparation supports the96 declared missions in all10 installed Halo2
packages, including their package-owned shared maps/DAT banks and inherited tag
metadata. Each resource retains its checked source owner; unknown layouts still
fail explicitly. All201 BSP readers pass the bounded native resource sweep.
Alpha Moon's custom Conduit font table/atlas remains unsupported. Multiplayer launch is local;
there is no Workshop download distribution, network hash agreement, or custom
variant import. These are compatibility limits, not subscription problems.

## E3 2003 / Earth City

Local item `3360515088` declares `earthcity_e3`, `earthcity_playground`, and
`earthcity_e3_potato`. All three appear in the catalog. The main mission's data
loads with 62,902 collision triangles, 14,688 navigation sectors, 111 AI squads,
596 scripts, and no content-reader errors in the initial bounded smoke.

**Full campaign compatibility is not implemented.** The first native smoke
rejected `rasterizer_bloom_override_silver_bullet`. The three authored bloom
controls and `object_cinematic_collision` now have native owners. Bloom uses
the mod's actual pass count, box-factor spacing and screen/additive blend choice;
the E3 value7 requests seven blur passes. The object action commits the original
collision flag and invokes the existing spatial-update owner. Independently
captured TU1.5 outputs cover36 HS setter cases,64 complete bloom schedules and
programs, and32 collision flag/callback cases. Collision-query admission is
also checked against the engine's actual flag consumer. These are CPU/contract
checks, not a claim of complete scene collision or Xbox framebuffer parity.

The MCC and Xbox `ssc_light_diffuse` and `global_playtest_mode` declarations both
have null backing. They now retain the original typed-zero read and converted
assignment-result semantics. Writes do not invent a retained lighting/playtest
state. Their descriptors are authenticated in the compatibility audit.

Other requirements remain, including camera/scenery animation overload admission,
vehicle controls, command-script actions and sound timing. Some already have
host implementations but lack complete native evaluator qualification. The
production preparation path checks the complete script layout and reports
remaining requirements before gameplay; it never erases those calls or
substitutes successful results. The playground's sole original layout blocker
was cinematic object collision. Actual startup then exposed the already
registered `unit_set_enterable_by_player` command's missing campaign-host
handler. The source now has both the actual native effect and the campaign
host route.32 flag cases,8 HS evaluator cases and4 complete original
seat-selection cases pass. The player-entry bit12 gate is checked with the
actual recursive seat enumerator, including its exemption for AI entrants;
authored seat flags bound these cases before geometry/animation admission.
Actual Playground startup now passes this handler and the subsequent
`object_cannot_die` call. The latter preserves the original vitality byte and
is checked against64 original evaluator cases and the real body-damage clamp.
The next observed missing host, `ai_actors`, now traverses retained native
squad/group/actor and starting-location banks, expands swarm members and
returns the existing salted HS list bank's original prepend order.144 original
captures and96 native effect comparisons cover its bounded behavior; existing
AI count/fraction tests still pass. A longer run reaches a separate cinematic
LOD presentation-owner bug, now corrected by keeping native actors with their
actual object owner. The next90-tick check reaches an
`object_create_anew_containing` placement-without-definition failure and remains
red. A longer run also exposed an unavailable native-world backing read. These
are open runtime gates, beyond declaration conversion.
The cache test also verifies67 authored
weapon/projectile/biped/vehicle definitions after required physics normalization.

Private preparation now retains the rasterizer's complete global native vertex
closure, including the unchanged212-byte decal program. Verified unanimated
SMG-grip and plasma-decal material conversions retain authored inputs; the
transparent decal applies only a paired-source pixel-slot permutation. Changed
authored pixels or native template payloads are rejected. Campaign rendering
gets past those blockers and now reaches unresolved `window_skylight`; other
model/material gaps remain. No stock material instances replace mod authoring.
After script admission, campaign preparation checks actual placed models'
material tables for missing native passes. Playground now reports those
rendering gaps before launch, using a separate material message. Explore Map
remains available. This preflight does not qualify every rendering feature or
the later script/physics paths.

The existing engine can render this mission through content preview. The initial
four-frame Original/Vulkan preview loaded 92,677 BSP triangles and 200/288 diffuse
textures, with remaining material-conversion gaps. This is not full visual,
campaign, audio, E3 custom-shader, or online compatibility.

Earlier reports: `build/test-results/workshop-20261031/`. The first feedback build
`1f04e6a955854e238689fbd26d53dfa8` passed all59 required CTests without skips,
all13 map/player submitted-frame smokes and all four frontend startup captures.
That publication selected both local launchers. The normal installed launcher
also renders the Mods catalog and all three E3 mission entries after removal of
the private build/source tree; both pages were visually reviewed. No required
test or smoke failure was waived. Failed direct E3 play probes remain explicit
failures, and neither E3 mission is qualified as playable.

Open **Mods**, choose the E3 item and a mission, then **Explore Map** for the
bounded preview. Main E3 still reports25 script requirements; Playground's
material preflight now keeps its known renderer gaps in the menu. Later
object-creation/native-world, full scene rendering, audio and progression gates
remain open. The requested new runtime/resources retain3,195,039,802 logical
bytes (2,079,563,776 allocated) under
`bin/engine-builds/1f04e6a955854e238689fbd26d53dfa8`. The prior feedback runtime
is preserved; private source/build/map/capture copies are removed.

The later Workshop build `0daad7b0c12346cc8ab3ce28ef52feab` passed62 required
checks and the complete installed resource/runtime sweeps. Its runtime sweep
records35 simulation passes,19 preview passes and zero gameplay-frame passes;
no installed campaign is qualified as complete. Both live selector records
currently name the separately qualified Zanzibar build
`29f4db427b1d44ea94a4449388ef6ff1`. The new sky candidate remains unpublished
because one of67 required checks fails. Alpha Moon and all three E3 variants
render four preview frames on that candidate, while campaign scripts/materials
remain explicit failures. The native delayed-title setter and lifetime library
passes240 original CPU comparisons, four mutation controls and Clang ASan/UBSan;
World/presentation integration is still open and it is not enabled in gameplay.
See the [current sweep and acceptance limits](WORKSHOP_COMPATIBILITY_SWEEP.md)
and [active checklist](MCC_GAMEPLAY_GOAL_CHECKLIST.md).

## Validation

Discovery fixtures and native menu/service integration:

```bash
python3 tools/run_tests.py --target halo2_native \
  --target steam_workshop_menu_tests --target steam_workshop_cache_tests \
  --target frontend_mcc_submenus_tests \
  --filter '^(steam_workshop_content|steam_workshop_menu|steam_workshop_cache|mcc_frontend_submenus)$' \
  --cmake-arg=-DHALO2_WORKSHOP_TEST_CACHE=/absolute/prepared/mcc/cache \
  --output build/test-results/workshop-validation
```

The menu test checks refresh, empty catalogs, mission/difficulty routing,
progress ownership, and error recovery. With the actual E3 item installed, the
cache test checks the real subprocess bridge, private-owner reuse, authored
gameplay preservation (allowing the required unit-physics layout normalization),
and native script rejection. The optional
`tests/steam_workshop_runtime_tests.py` compares source hashes, renders a preview,
and records actual campaign-start failure under `tools/run_test.py`. Its failing
campaign result must remain visible; a successful preview does not turn it green.

All test maps, binaries, libraries, and captures belong to the runners' scratch
directories and are removed afterward. Keep the bounded JSON/log evidence.

The authored-control/reference regressions are:

```bash
python3 tools/run_tests.py --target workshop_bloom_tests \
  --target workshop_script_contract_tests --target cinematic_bloom_tests \
  --target workshop_ai_activation_tests --target workshop_ai_activation_contract_tests \
  --target workshop_unit_entry_tests \
  --target workshop_ai_actors_tests \
  --target mcc_script_layout_tests \
  --filter '^(workshop_bloom_controls|workshop_script_contracts|workshop_ai_activation|workshop_ai_activation_contracts|workshop_unit_entry|workshop_ai_actors|original_cinematic_bloom_tu15|mcc_script_signature_layout|mcc_script_signature_profile)$'
```

`tools/oracle_workshop_bloom_tu15.py` captures unchanged offline TU1.5 outputs;
`tools/generate_workshop_script_builtins_tu15.py` lowers complete authenticated
evaluator bodies into ordinary native operations. Runtime code uses neither
an executable decoder nor original code execution. Requests beyond31 blur
passes fail explicitly at the renderer's descriptor budget.

## Other installed mod gaps

The read-only audit covers101 maps across all10 installed Halo2 packages.
`build/test-results/workshop-20261030/all-mods-compatibility.json` records each
map's declarations, globals, resource selectors and material templates. A
matching declaration is only a conversion candidate; it does not prove its
engine behavior or make a mod playable. The existing native clock getters are
recognized as owned rather than reported as missing.

| Area | Observed examples / remaining work |
| --- | --- |
| Shared resources | Private preparation now retains each package's matching shared maps and DAT banks, with distinct local/inherited resource ownership. Digtacular03_ascension, Reformation lockout and Rampancy abyss pass complete bitmap-frame, first-audio-permutation and facial-block inspections; all-map/runtime validation remains open. |
| AI and command scripts | Force-active and actor-list queries have bounded native owners; movement/formation/death/sound actions, full scene AI and vehicle behavior remain open. |
| Cinematic/gameplay APIs | Camera/animation overloads, scenery animations, input tests and sound timing need full native qualification. |
| MCC-only APIs | Breadcrumb navigation points, distance-to-position, oversteer control, movie/loading/light-suppression commands need adapters. |
| Custom materials | E3 preview still lacks some authored shader conversions; stock shader substitution would discard mod authoring. |
| Multiplayer | Local maps are routed; authored variants, Workshop content agreement/downloads and online acceptance remain open. |
| Mission metadata | Default primary weapons, category-specific team limits, mission images, scoring and film/insertion policy are not yet routed by the Mods bridge. All installed insertion-point lists are empty; Grifball explicitly requests a gravity hammer. |
| Packaging | Windows-style directory case and mixed campaign/multiplayer declarations are handled; loose tags, custom controller content and >1GiB maps remain outside preparation. |

The installed MCC `rasterizer_blur(short)` evaluator calls `180030CC0`, whose
entire body is `RET0`. Its pending adapter must still preserve argument
evaluation and HS completion; adding an invented blur effect would be incorrect.
The audit treats Reformation only as installed user content/reference; no
Reformation or Xbox asset was imported into the base MCC runtime.

The current whole-installation baseline and follow-up findings are retained in
[WORKSHOP_COMPATIBILITY_SWEEP.md](WORKSHOP_COMPATIBILITY_SWEEP.md). Preparation
uses only the selected mod's banks and the user's prepared MCC fallback. It
never borrows another Workshop item's assets to resolve a tag-version collision.
