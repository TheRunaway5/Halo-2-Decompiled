# Halo 2 native port workbench

This repository contains a native C++20 Halo 2 workbench using SDL2 and Vulkan,
plus tools to extract a user-supplied Xbox disc and export a local Ghidra
decompilation. Original game files, imported artwork/fonts, generated reference
captures, analysis exports, and builds stay local and are excluded from Git.

**This is a native subsystem workbench, not a complete Halo 2 port.** The
renderer loads real BSP meshes, authored geometry instances, player starts,
and diffuse textures. The optional simulation adds collision/movement,
authored squad models, skeletal animation, provisional AI/combat with authored
navigation, a compiled-script subset, scenario object rendering/lifetime,
authored camera tracks, fades/letterbox, and decoded impulse/looping audio. Full
gameplay fidelity, Havok, animation state machines, mission scripts,
saves and complete networking parity remain unfinished. Native LAN and local
split-screen Slayer are available in the current multiplayer build; see
[MULTIPLAYER_PROGRESS.md](docs/MULTIPLAYER_PROGRESS.md). The retail title/menu frontend is now
implemented as a partial native UI; its remaining fidelity gaps are recorded
in [FRONTEND.md](docs/FRONTEND.md). The generated Ghidra C is
analysis pseudocode; it is not compiled into the application.

See [native systems and controls](docs/NATIVE_SYSTEMS.md) for implementation
boundaries, commands and validation.

## Find your way around

| Directory | Contents |
| --- | --- |
| `src/`, `include/halo2/` | Native implementation and public interfaces |
| `shaders/` | GLSL renderer shaders |
| `tools/` | Asset setup, extraction, decompilation, audits and launchers |
| `tests/`, `cmake/` | Test sources and build configuration |
| `multiplayer/` | Master server, online services and their documentation |
| `external/`, `resources/launcher.svg` | Third-party source dependencies and launcher icon |
| `docs/` | Source evidence, implementation status and developer guides |

Start with the [repository guide](docs/REPOSITORY_LAYOUT.md),
[game-file setup](docs/ASSET_SETUP.md), and [port status](docs/PORT_STATUS.md).
The guide explains local-only directories and the reference data required by
some tests. Generated C++ used by the native runtime remains in `src/`.

Engine and tool developers can use the shared [tag system](docs/TAG_SYSTEM.md)
for salted handles, authored group inheritance, nested blocks, references,
BSP residency and cache resources. Its public interface is `halo2/tags.hpp`.

## Build and run

The requested [Vista gameplay import](docs/VISTA_GAMEPLAY_CONVERSION.md) now has
a checked preparation step for Vista maps/tags and recovered Xbox TU1.5
postprocessing, with Xbox texture donors and Vista HUD/menu artwork. The native
adapter reads Vista tags, geometry and supported animations and binds original
Xbox bitmap resources. Vulkan material passes, remaining runtime consumers and
the full parity audit remain unfinished; the active launcher still uses the
Xbox gameplay source described below.

The repository includes an x86-64 Linux `Halo2-Launcher` executable in the
project root for first-run game-file setup. Rebuilding creates
`build/Halo2-Launcher` (or uses your chosen build directory).
Installing the rebuilt launcher also creates `Halo2-Launcher` in the project root and
the **Halo 2 Native** desktop/app-menu shortcut. First launch requires a
Halo 2 Vista installer for HUD/menu artwork and an original Halo 2 Xbox ISO
or extracted disc folder for gameplay, models and in-game textures. Select
both once; setup prepares the needed files automatically. It keeps the current
layouts and samples compatible Vista artwork/fonts. See
[game-file setup and source policy](docs/ASSET_SETUP.md).
Optional MCC Classic HUD/menu artwork, fonts, Vista SVG button redraws and
bitmap atlases can be prepared locally under the ignored `resources/ui`.
They are not included in a Git checkout. Normal game-file setup selects a
local bundle when present and no existing UI selection is saved; otherwise
it uses the supplied Xbox/Vista sources. See [local UI assets](docs/UI_ASSET_BUNDLE.md)
for verification, activation and regeneration.
After setup it plays the startup intro before the title screen and
menu frontend. This launches the current native workbench; full campaign gameplay is
still unfinished. Keep the executable with this project and its `assets` and
`build` folders.

The refreshed `Halo2-Title-Launcher` and **Halo 2 Native (Title)** shortcut start
at the title screen. The launchers use the runtime from the build directory
selected when they are rebuilt and installed, including the F1 CRT filter and
the current multiplayer HUD with compatible Vista artwork. Vulkan shaders are
independent GLSL implementations using Xbox material behavior as their reference.
Vista keyboard/mouse controls are available from **Player Profile**, and
**Settings > Video** opens the Vista video menu using the supplied reference
cache. See [Vista settings support and source evidence](docs/FRONTEND_VISTA_SETTINGS.md)
for working options and remaining engine effects.

**ONLINE** requires Discord sign-in using application `1558522073272426519`.
The ONLINE sign-in asks only for basic identity; the Discord user ID stays internal.
Optional [Discord Friends](multiplayer/docs/friends.md) uses separate Social SDK
consent for friends/presence, the Vista Friends tab and native party invitations.
Players keep their chosen local profile name as their screen name, editable from
**Discord Account → Change screen name**. Campaign, Split Screen and System Link keep local profiles.
See [Discord sign-in setup and verification](docs/DISCORD_IDENTITY.md).

The independent [federated master-server foundation](multiplayer/master_server/README.md)
provides signed server registration, discovery, expiration and peer replication,
with a simulator and Docker examples. **ONLINE → Server Browser** now reads
that directory at `https://master.halotoforever.com` by default; override trusted
masters using `H2_MASTER_URLS` as described
in [native discovery setup](multiplayer/docs/native-discovery.md). Native
Quickmatch/Optimatch/Playlists use a separate Discord-authenticated queue API;
see [matchmaking setup and limits](multiplayer/docs/matchmaking.md). Gameplay
admission launches native Slayer/Team Slayer through the authenticated relay.
Accepted [parties](multiplayer/docs/parties.md) have invitations, leader transfer,
member playlist consent and group searches; historical catalog restoration is
still incomplete.
Discord-backed [clans](multiplayer/docs/clans.md) support invitations, membership,
roles and messages. The opt-in [clan and party voice channels](multiplayer/docs/voice.md)
uses hold-V push-to-talk and local muting. ONLINE and clan panels use the imported
Vista fonts and artwork; Discord IDs stay hidden and screen names remain editable.

Use Split Screen or host through System Link; LAN joining and 3/4 local players
are available through the launcher arguments in
[MULTIPLAYER_PROGRESS.md](docs/MULTIPLAYER_PROGRESS.md). Existing campaign
work and remaining mission blockers are recorded in
[CAMPAIGN_PROGRESS.md](docs/CAMPAIGN_PROGRESS.md).

Elite profiles and mixed Spartan/Elite multiplayer rosters use their authored
bodies, arms, cameras and HUDs; see [Elite integration](docs/MP_ELITE_MULTIPLAYER_INTEGRATION.md).

To rebuild and reinstall the launcher and shortcuts:

```bash
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target halo2_launcher halo2_title_launcher -j
python3 tools/install_launcher.py --build build-release --desktop
python3 tools/install_launcher.py --build build-release --title-first --desktop
./Halo2-Launcher
```

Use an optimized runtime for ordinary play: Debug builds spend substantially
longer decoding assets before the title screen and matches. New single-config
builds default to `RelWithDebInfo`; an explicitly selected build type is retained.
For repeatable first-frame loading measurements, see
[loading performance](docs/LOADING_PERFORMANCE.md). For steady gameplay/menu measurements and the current material-bank optimization, see
[frame-rate performance](docs/FRAMERATE_PERFORMANCE.md).

Runtime flags can be passed to the launcher. `--launcher-info` prints the paths
it found; `--map` selects a level directly. `--menu-screen ID` previews an
authored menu screen, and `--menu-sweep DIRECTORY` captures all menu panes and
variant option dialogs. See [frontend menus and controls](docs/FRONTEND.md).
`--skip-intro` goes directly to the prompt. `--menu-movie intro|attract|credits`
and `--movie-frame N` preview disc movies; `--menu-tick N` selects a reproducible
flythrough tick for screenshots. The separate `Halo2-Title-Launcher` also starts
at the prompt and uses its own desktop shortcut.

Select a user-owned supported Xbox update once with
`python3 tools/select_gameplay_source.py /path/to/update/default.xbe`.
The tool checks its SHA-256 and records an external path in the ignored
`assets/gameplay-source.tsv`. Both launchers then carry that executable through
the title/menu route into multiplayer. `--launcher-info` shows the selection;
`--gameplay-xbe PATH` explicitly overrides it. A missing selected update stops
launch instead of silently reverting to base-disc gameplay data. Without a
selection, the original disc executable remains the default. Selecting an
update does not establish complete native gameplay parity.

Required: CMake 3.24+, a C++20 compiler, SDL2 and zlib development files, Vulkan headers
and loader, GLM, `glslangValidator`, Python 3.11+ with Tk, 7-Zip, pkg-config, FFmpeg development
libraries (`libavcodec`, `libavformat`, `libavutil`, `libswresample`, `libswscale`) with Bink and dedicated Xbox
ADPCM decoder, libcurl 7.86+ with WebSocket support, libopus, OpenSSL Crypto,
nlohmann-json 3.10+, and a Vulkan-capable GPU.
The current runtime was built and tested on Linux. Other desktop systems have
not been validated.

For a fresh checkout, build and run the launcher from the project root:

```bash
./Halo2-Launcher --setup-only
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target halo2_launcher -j
./build/Halo2-Launcher
```

The first launch opens the game-file setup window. Select your Xbox ISO or
extracted disc folder and Vista installer, then choose **Prepare game files**.
The launcher saves that selection and starts the game after setup succeeds.
Configuring CMake after setup uses the selected Xbox source for the menu and
particle shaders, including sources imported outside `assets/disc`.

The bundled executable uses `python3` from your PATH and requires SDL2 and Tk.
If the setup window cannot open, run `python3 -m tkinter` to check Tk in that
interpreter. A missing Tk module requires your distribution's Python Tk package.
Terminal setup works without Tk when both source flags have a file or folder
path after them (quote paths containing spaces):

```bash
./Halo2-Launcher --setup-only --vista-source "/path/to/Vista installer.zip" --xbox-source "/path/to/Halo 2.iso"
```

To regenerate it after changing launcher source:

```bash
cmake --build build --target halo2_onboarding
install -m 755 build/onboarding/Halo2-Launcher Halo2-Launcher
```

For direct runtime development, extract an Xbox disc into the conventional
local path before using the examples below:

```bash
python3 tools/disc.py extract 'Halo 2 (USA, Europe) (En,Ja,Fr,De,Es,It,Zh,Ko).iso' --output assets/disc
```

Extraction refuses to overwrite existing files; skip it if the disc has already
been extracted locally. Direct `halo2_native` commands bypass launcher setup.
Explicit level/simulation commands use Lockout unless `--map` selects another level:

```bash
./build/halo2_native --map assets/disc/maps/ascension.map --overview
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --bsp 0
./build/halo2_native --play --weapon 'objects\weapons\rifle\battle_rifle\battle_rifle'
./build/halo2_native --map assets/disc/maps/01b_spacestation.map --play \
  --weapon 'objects\weapons\rifle\battle_rifle\battle_rifle' --place-squad 1st_hum_floor_mar
./build/halo2_native --map assets/disc/maps/01b_spacestation.map \
  --camera-animation 'objects\characters\cinematic_camera\x02\x02' x02_01 17
```

- Click the window to capture the mouse; move with WASD and look with the mouse.
- By default Q/E move down/up and Shift increases free-flight speed.
- With `--play`: collision and gravity apply; Space jumps, Shift crouches, left mouse fires, R reloads, F2 toggles free flight. Supported actors use original skinned models and idle clips; unavailable models retain diagnostic boxes.
- Tab cycles authored player starts; R moves to an overview camera.
- Escape releases the mouse, then exits. SDL gamepads provide movement/look.
- `--diagnostic` uses material colors; `--bsp N` selects a structure BSP.

The current multiplayer input uses WASD movement, mouse look, left mouse fire,
R reload, Space jump and Shift crouch. The remaining requested bindings and
their gameplay integration status are documented in
[multiplayer keyboard and mouse controls](docs/MP_KBM_CONTROLS.md).

F1 opens or closes the optional debug panel; Escape closes it before returning
to normal game input. Hold F3 for the multiplayer scoreboard. The full scoreboard
also opens automatically at the original respawn-countdown gate and fades away
after respawn; death animations, physical corpses and the death camera remain
unfinished. See [death-cycle implementation and source gaps](docs/MP_DEATH_CYCLE_IMPLEMENTATION.md).
Gameplay debug
controls are available in local sessions and are disabled in LAN sessions.
The Display tab includes the optional CRT Filter ported from Phase-Distorter:
flat scanlines, an RGB aperture grille, and OLED brightness compensation.
It starts disabled, lasts for the session, and filters the completed game image
before the debug panel. See [CRT filter provenance](docs/crt-filter.md).

`--camera-animation` previews the specified graph, animation name and scenario
anchor flag index. Cairo's original first camera track lasts 515 ticks. This
preview uses a 60 degree horizontal field of view, matching Cairo's first
scene. It does not run the cutscene's actor animations or mission startup.
`--campaign` now executes native checkpoints/save/revert, cross-BSP streaming,
asset prediction and sound-class fades. Cairo next stops at
`object_set_permutation`; relative actor animations and later mission services
remain pending. No complete campaign mission has been verified.

Rendering uses authored mip chains, template bitmap indices and constant remaps,
P8 bump/lightmap palettes, and per-cluster/per-instance lightmap bindings. The
Supported lightmap, texture, emissive and transparent paths run the original
assembled pixel-combiner programs and admitted authored vertex programs.
Model tangent bases, fixed object change colors, shader lighting selectors,
specular brightness, cubemap faces/mips and single-visible-sky models are bound.
Active material stages retain authored sampler states and execute supported
projected/dependent texture operations. Pixel viewport constants scale with the
active render target, including viewport offsets and the original pixel bias;
native capture checks cover 480p through 4K.
Other materials retain an explicit fallback. The complete retail pass scheduler,
remaining externs, object light probes, dynamic lighting/shadows, complete
render-state coverage, transparent sorting, fog/postprocess and remaining special
texture/sampler modes remain pending.
Full visual parity remains unverified; captured weapons still have flat lighting.
See [APPEARANCE_PARITY.md](docs/APPEARANCE_PARITY.md) for current corrections,
evidence and remaining gates, and [PORT_STATUS.md](docs/PORT_STATUS.md#shader-and-lighting-parity)
for the broader implementation checklist.
First-person arms and weapons share the authored animation graph and original
camera-control adjustment, with player FOV read from the original biped tag.
Placement evidence and remaining motion/visual gates are recorded in
[FIRST_PERSON_PARITY.md](docs/FIRST_PERSON_PARITY.md).

## Multiplayer Map Pack source data

The second ISO is extracted separately under `assets/multiplayer-map-pack`.
Its 148 files include nine DLC maps, their authored map-menu catalogues and
previews, and the original **1.1** title-update executables. The base game
extraction is preserved. Each file and the ISO have SHA-256 provenance in
`assets/multiplayer-map-pack/disc-manifest.json`.

```bash
python3 tools/disc.py extract 'Halo 2 Multiplayer Map Pack (World) (En,Ja,Fr,De,Es,It,Zh,Ko).iso' --output assets/multiplayer-map-pack
python3 tools/audit_map_pack.py
.tools/python/bin/python tools/oracle_map_pack_levels.py > analysis/multiplayer/map-pack/level-instruction-oracle.json
```

Skip extraction when the manifest already exists. The audit verifies all 148
files, decodes the original catalogue records and checks their CRCs; the oracle
executes the original Xbox validators with valid and corrupted inputs.
The native cache reader recognizes all nine map headers/tag directories/BSP
records. This does not establish playable DLC-map support or apply the game's
balance changes. Those changes still need recovery from the updated executable;
the `.lvl` files are map catalogues. The disc's 1.1 update also does not establish
the final-ever Xbox update. See
[Map Pack source research](docs/MULTIPLAYER_MAP_PACK_RESEARCH.md).

The armor/shield/body kernels match 20,000 untouched 1.1 instruction cases.
The active [damage and shield coverage goal](docs/DAMAGE_SHIELD_COVERAGE_GOAL.md)
tracks every damage producer, health/shield behavior and shield presentation.
The shared adapter now retains authored shield shader/effect references and
ordered damage/depletion effect attempts; live effect attachment and shield hit
shimmer remain open.
Original firing-error/recovery, error-angle selection and random direction
rotation are now ported into separate C++ kernels: 70 source fixtures and 13,098
direction/seed cases match exactly. The bounded single-SMG trigger, reload and
pending-shot path is now integrated in local and native LAN matches: 2,220
instruction-trace ticks and 960 actual-map ticks pass. Its owner-motion launch
projection also matches 32 original cases. A native adapter now activates authored action clips for other weapons and both
hands, and connects map pickup, carry selection, independent offhand fire/reload,
HUD and UDP delivery. [Current implementation and source limits](docs/MP_WEAPON_GAMEPLAY.md).
Complete original non-SMG managers, dual blending and world/event order remain
unresolved. See
[SMG source evidence](docs/SMG_FIRING_1_1_RESEARCH.md).

The allocated/equipped stock SMG path additionally matches 599 original ticks
and 165 requests, including its own animation state and reload admission; those
ticks also pass through production local and UDP matches. Ordinary SMG damage
now uses full captured owner identities, previous-history lethal credit/assists,
then history recording and synchronous player detach. Respawn retains the old
corpse's full identity and source garbage deadline. Native protocol11 replicates
assists/suicides and completed single-SMG first-person clock pairs, with retained
short jump/fire/reload requests across coalesced or lost input packets. The incoming-owner counter matches 81 original cases. Local
single-SMG first-person bindings match the recovered registration/removal and
authored node maps, with four independently bound native views. Their primary
animation clocks now match 541 original ticks, 2,705 phase states and four event
callbacks; 516 ticks also match in actual local matches and their dedicated
presentation replicas. Rendering samples fractional authored frames through a
separate native presentation policy. Actual one/four-view local and UDP
fire/reload probes pass. Complete death animation/deletion, secondary first-person
channels/camera/audio, inventory and the
broader sandbox remain unfinished. See
[multiplayer integration](docs/MULTIPLAYER_INTEGRATION.md),
[LAN animation clocks](docs/MP_FIRST_PERSON_LAN_CLOCK_INTEGRATION.md), and
[measured frame preparation](docs/MP_RENDER_FRAME_TIMING_INTEGRATION.md).

The bounded native pickup kernels now compose source item points with 207 original
consumer calls, 187 exact normalizations and 1513 terrain-backed casts, normally
and with focused sanitizers. The separate item-point producer matches 208 calls,
including three captured ground-marker/contact projections. Live current
object/node/contact producers and pickup admission/transfer remain integration gates. See
[the item-point connection](docs/MP_ITEM_POINT_NATIVE_INTEGRATION.md) and
[the verified scope](docs/MP_PICKUP_VISIBILITY_NATIVE_INTEGRATION.md).

Native sound variation imports17 authored sound definitions and23 permutations.
The source gain/pitch/selection kernels match32 original selections, three
creation slices and83 ordered random-state writes, including sanitizer checks.
The full sound scheduler and audible playback remain integration work. See
[the measured sound scope](docs/MP_SOUND_VARIATION_NATIVE_INTEGRATION.md).

The native unit-input consumer matches 10 original marshalled updates against
complete unit prefixes, and its empty-slot scan matches 9 original calls.
Both pass with focused sanitizers. These consume literal source state; live
controls, pickup admission and item transfer remain unfinished. See
[unit-input integration](docs/MP_UNIT_INPUT_NATIVE_INTEGRATION.md).

Inventory-slot/sequence orchestration also matches 10 original calls each and
59 ordered writes, normally and with sanitizers. Its 19 ownership/removal/drop
leaves are explicit recorded-output boundaries; the complete live pickup chain
remains unfinished. See [inventory orchestration](docs/MP_UNIT_INVENTORY_NATIVE_INTEGRATION.md).

Native neutral-command initialization and cached delivery match both original
owning cases normally and with focused sanitizers. Full command/delivery/record
buffers agree; submission codec, device/control producers and live frame
integration remain unfinished. See
[command kernels](docs/MP_PLAYER_COMMAND_NATIVE_INTEGRATION.md).

Native first-item motion matches six original resting/miss/contact calls and58
ordered writes, normally and with focused sanitizers. Transform/world/effect
producers, continuous settling and the live frame route remain unfinished.
See [motion scope](docs/MP_ITEM_MOTION_NATIVE_INTEGRATION.md).
Native matrix/current-node/bounds producers also match their original buffers
and ordered writes. Reciprocal hardware, registry/location owners and live
integration remain open. See [producer scope](docs/MP_ITEM_TRANSFORM_NATIVE_INTEGRATION.md).

The three native object dispatchers match original complete header/bitmap/queue
and body buffers, ten explicitly supplied producer outputs and 15 owning writes.
Normal and focused sanitizer checks pass. Live motion/contact producers and
complete frame integration remain open. See
[object scheduling](docs/MP_OBJECT_SCHEDULE_NATIVE_INTEGRATION.md).

```bash
python3 tools/compare_map_pack_combat.py
.tools/python/bin/python tools/differential_damage.py --version map-pack-1.1 --library build-multiplayer/libhalo2_kernels.so
python3 tools/extract_weapon_direction_table.py
.tools/python/bin/python tools/oracle_weapon_error_1_1.py --output analysis/multiplayer/map-pack/weapon-error-1.1-oracle.json
cmake -S . -B build-multiplayer -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-multiplayer --target weapon_error_tests original_data_tests -j 2
.tools/python/bin/python tools/differential_weapon_error.py
ctest --test-dir build-multiplayer -R '^(retail_weapon_error_kernels|local_original_data)$' --output-on-failure
```

The offline C++ test uses original output bits in
`tests/fixtures/retail_weapon_error.tsv`; regenerate that snapshot only after
passing the instruction differential with `--export-fixtures` and the same path.
The native kernels take their direction and fade tables from the user's local,
validated `default.xbe`; those original tables are not embedded in source.
`local_original_data` checks the two supported images and rejection of corrupt
data. These asset-dependent tests skip explicitly when the local originals are
absent. Code and synthetic tests still build without a disc.

DLC multiplayer maps can use the base disc's shared resources directly:

```bash
./build-multiplayer/halo2_native --multiplayer \
  --map assets/multiplayer-map-pack/demos/dlc/00000001/warlock.map \
  --shared-maps assets/disc/maps --local-players 4
```

The future MCC adapter is not implemented. It will require actual supplied
Steam files as format evidence.

## Decompile the local executable

The local `default.xbe` is title `4D530064`, XDK 5849, retail build matching
SHA-256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

```bash
python3 tools/disc.py xbe assets/disc/default.xbe
python3 tools/decompile.py --bootstrap --matching-inventory
```

The bootstrap downloads pinned official Ghidra 12.0.3 and XboxDev XBE loader
releases into `.tools/` and verifies their upstream SHA-256 checksums. Ghidra
requires JDK 21+. `JAVA_HOME` can select the installed JDK. The optional
`--matching-inventory` obtains function boundaries from the pinned CC0
[matching-decompilation project](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27);
the script only applies that inventory to the exact executable hash above.

The local analysis run produced the following ignored outputs; they are not
included in a Git checkout:

- `analysis/projects/halo2-retail.gpr`: saved Ghidra project.
- `analysis/retail/functions/`: one pseudocode file per successfully exported function.
- `analysis/retail/all.c` and `functions.tsv`: combined export and address/status index.
- `analysis/retail/summary.json`, `provenance.json`, and `ghidra.log`: results and evidence.

The local run identified 19,135 functions and exported 19,130; five failed.
Warnings and unresolved types/calling conventions still require manual review.
Repeat analysis with a new output and project name to preserve this saved work:

```bash
python3 tools/decompile.py --matching-inventory --project halo2-next --output analysis/next
```

## Validate

```bash
ctest --test-dir build --output-on-failure
./build/halo2_native --validate-maps assets/disc/maps
./build/halo2_native --validate-systems assets/disc/maps
./build/halo2_native --frames 120 --resize-test --validation --screenshot analysis/frame.ppm
python3 tools/smoke_render.py --validation --screenshots
python3 tools/audit_rendering.py --maps assets/disc/maps
.tools/python/bin/python tools/differential_rendering.py
.tools/python/bin/python tools/differential_combiners.py
```

`--validation` requires the Khronos Vulkan validation layer. For synchronization
checks, set `VK_LAYER_VALIDATE_SYNC=1`. `--screenshot` records pixels from the
actual rendered swapchain; it exits after three frames unless `--frames` is set.
The smoke runner creates hidden SDL windows and tests every available BSP.
The rendering audit records each map's original shader templates, pass
implementations, bitmap/constant remappings, vertex-program hashes, lightmap
palettes and bindings. It reports missing native capabilities explicitly;
successful asset parsing or a Vulkan smoke test does not establish retail
frame parity. The differential test uses the local retail XBE and Unicorn
to check mip selection, pixel-program assembly and vertex/pixel constant remapping;
it does not establish GPU or frame parity.

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHALO2_SANITIZERS=ON
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
```

See [docs/PORT_STATUS.md](docs/PORT_STATUS.md) for measured results and remaining
milestones, and [docs/RESEARCH.md](docs/RESEARCH.md) for pinned primary format
sources. ISO files, extracted assets, analysis exports, tool downloads, and build
outputs stay local and are excluded from Git. `assets/disc/disc-manifest.json`
records the source disc hash, file offsets, and individual file hashes.
