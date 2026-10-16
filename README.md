# Halo 2 native port workbench

A native C++20 Halo 2 port in development, built with SDL2 and Vulkan, with
asset import, source-recovery and validation tools. The target is original
Xbox **TU1.5 gameplay and visuals**, using installed **MCC Halo 2 Classic**
content as the asset donor for normal setup.

**The port is incomplete. MCC content can be imported, but Play remains disabled
until native startup and full TU1.5 data conversion are ready.** Existing
reference-backed subsystem builds demonstrate rendering, frontend, campaign
components and local/network Slayer; they do not establish a finished MCC game.

Original game files, imported artwork/fonts, reference captures, Ghidra exports
and build outputs stay local and are excluded from Git. Generated Ghidra C is
analysis pseudocode, not application source.

## Progress at a glance

### Completed foundations

- **Content and recovery tools:** MCC installation discovery, verified private
  imports, native cache readers and geometry/model/bitmap/animation/audio
  adapters, checked 64-bit tag ownership, Xbox disc extraction and Ghidra export.
  See [MCC conversion](docs/MCC_GAMEPLAY_CONVERSION.md) and
  [the tag system](docs/TAG_SYSTEM.md) for the supported boundaries.
- **Native rendering and runtime components:** SDL2 input/gamepads, Vulkan
  presentation, authored BSPs, textures, lightmaps, skinned models, animation
  and decoded audio. Campaign components include checkpoint/revert, BSP
  streaming and cinematic camera playback; full missions remain unfinished.
  See [native systems](docs/NATIVE_SYSTEMS.md) and
  [campaign progress](docs/CAMPAIGN_PROGRESS.md).
- **Frontend and settings:** native title/menu presentation, local profiles,
  saved variant copies, keyboard/mouse settings, display controls and movie
  playback. The original menu definitions and option dialogs are loaded;
  service and visual coverage varies. See [frontend progress](docs/FRONTEND.md).
- **Multiplayer foundations:** local split screen, native LAN Slayer/Team
  Slayer, weapon/HUD/respawn integration, and online discovery, identity,
  matchmaking, party, clan and voice components. These retain documented
  gameplay and service limits. See
  [multiplayer progress](docs/MULTIPLAYER_PROGRESS.md) and
  [online services](multiplayer/master_server/README.md).

### Still to finish

- **MCC-only gameplay:** complete native world startup, physics/collision and
  gameplay-tag conversion, and remaining runtime consumers so the normal
  launcher can start a TU1.5-equivalent game without an Xbox executable.
- **The complete sandbox:** finish and integrate original movement/Havok,
  vehicles, all weapons, damage/shields, inventory, AI and animation behavior.
  Passing individual source comparisons does not complete these systems.
- **Campaign and co-op:** complete mission scripting, AI, cinematics,
  audio/presentation and save-state integration, then verify complete missions
  and cooperative play. No complete campaign mission has been verified.
- **All multiplayer modes:** finish the seven original mode families, all
  39 built-in variants and editable rules, objective/HUD consumers, replication
  and complete local/network match flows. Menu availability is broader than
  playable mode support.
- **Final parity and delivery:** verify Xbox TU1.5 rendering, effects, lighting,
  audio and timing against original references; complete performance,
  physical-controller and cross-platform validation. Linux is the currently
  tested desktop platform.

Detailed evidence and acceptance gates live in the subsystem guides below.
Asset decoding, exact CPU comparisons and Vulkan smoke tests each prove their
own scope; they do not establish full gameplay or original framebuffer parity.

## Build and run

### Prepare MCC content

With the local launcher installed at the project root, run:

```bash
./Halo2-Launcher
```

First launch searches installed game libraries and opens MCC setup. Select the
MCC installation if detection misses it. Setup verifies and copies Halo 2
Classic content into a private cache, preserving the original installation.
Later launches show cache and runtime status. Preparing content or updating the
build does not bypass the unfinished gameplay requirements.

Terminal setup works without Tk; quote paths containing spaces:

```bash
./Halo2-Launcher --prepare-mcc-gameplay --mcc-source "/path/to/Halo The Master Chief Collection"
./Halo2-Launcher --launcher-info
```

Use `--setup-only` to prepare files and exit. A source-only checkout can prepare
content before building a launcher:

```bash
python3 tools/setup_assets.py --prepare-mcc-gameplay --mcc-root "/path/to/Halo The Master Chief Collection"
```

Normal setup uses MCC only. The original Xbox ISO and TU1.5 update are
reference inputs for development; old Xbox/Vista source selections are ignored
by the launchers. See [game-file setup](docs/ASSET_SETUP.md) for cache ownership,
discovery options and readiness checks.

### Build from source

Required: CMake 3.24+, Ninja for the commands below, a C++20 compiler, Python
3.11+, SDL2, Vulkan headers/loader, GLM, `glslangValidator`, zlib, pkg-config,
libcurl 7.86+ with WebSocket support, OpenSSL Crypto, nlohmann-json 3.10+,
libopus, and FFmpeg development libraries (`libavcodec`, `libavformat`,
`libavutil`, `libswresample`, `libswscale`) with Bink and dedicated Xbox ADPCM
decoding. Runtime rendering requires a Vulkan-capable GPU. The launcher setup
and home windows also require Python Tk; `python3 -m tkinter` checks it.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --target halo2_launcher halo2_title_launcher -j
python3 tools/install_launcher.py --build build --desktop
python3 tools/install_launcher.py --build build --title-first --desktop
./Halo2-Launcher --launcher-info
./Halo2-Title-Launcher --launcher-info
```

Both launchers use the runtime directory selected at installation.
`--launcher-info` reports that path, the selected MCC cache and gameplay
readiness. The title-first launcher is configured to skip the boot intro once
startup is available. Use an optimized build for runtime work; Debug asset
preparation is substantially slower.

Release builds bundle the locally supplied upscaled intro and credits. Stage
them with `python3 tools/bundle_movies.py --source "/path/to/Halo 2 Movies"`
before making a movie-enabled release. A source-only Release build can select
`-DHALO2_BUNDLE_MOVIES=OFF`. See [release movies](docs/RELEASE_MOVIES.md) for
checksums, codecs and packaging.

The launcher provides `--update` to download and build the current branch while
preserving imported content and local changes. Logs remain in
`launcher-update.log` and the selected build's `launcher-build.log`. A moved
checkout preserves an incompatible build directory as `build.previous` and
creates a fresh one. Runtime flags and direct reference-backed development
commands are documented in [native systems and controls](docs/NATIVE_SYSTEMS.md)
and [frontend inspection](docs/FRONTEND.md#controls-and-inspection).

## Multiplayer and online services

Existing subsystem builds support local split screen and native LAN
Slayer/Team Slayer. Lobby setup, launcher arguments, controls and remaining
sandbox/network limitations are documented in
[multiplayer progress](docs/MULTIPLAYER_PROGRESS.md) and
[keyboard/mouse controls](docs/MP_KBM_CONTROLS.md).
[Elite integration](docs/MP_ELITE_MULTIPLAYER_INTEGRATION.md) covers authored
Spartan/Elite bodies, arms, cameras and HUDs. Optional
[Fiesta and Infection Slayer](docs/CUSTOM_SLAYER_VARIANTS.md) are native custom
variants with their own rules and verification limits.

ONLINE uses [Discord sign-in](docs/DISCORD_IDENTITY.md); players retain their
chosen local profile names. Campaign, Split Screen and System Link use local
profiles. The [server browser](multiplayer/docs/native-discovery.md) uses the
federated directory at `https://master.halotoforever.com` by default, with
trusted-master overrides through `H2_MASTER_URLS`.
[Matchmaking](multiplayer/docs/matchmaking.md),
[parties](multiplayer/docs/parties.md), [clans](multiplayer/docs/clans.md),
[Discord Friends](multiplayer/docs/friends.md) and opt-in
[voice](multiplayer/docs/voice.md) have separate setup and support boundaries.
These service components do not remove the MCC gameplay blocker.

## Validation and source recovery

Build the test targets before running CTest:

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The full suite needs local reference data: some asset-dependent checks skip
when inputs are absent, while others fail. See the
[fixture requirements](docs/REPOSITORY_LAYOUT.md#tests-and-reference-data) and
each subsystem's evidence before interpreting results. GPU validation requires
the Khronos Vulkan validation layer; use `VK_LAYER_VALIDATE_SYNC=1` for
synchronization checks. Render sweeps, source differential commands and measured
results are linked from [port status](docs/PORT_STATUS.md).

For sanitizer builds:

```bash
cmake -S . -B build-asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DHALO2_SANITIZERS=ON
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
```

Developer reference extraction and decompilation remain separate from MCC
onboarding. With a user-supplied Xbox disc:

```bash
python3 tools/disc.py extract "/path/to/Halo 2.iso" --output assets/disc
python3 tools/disc.py xbe assets/disc/default.xbe
python3 tools/decompile.py --bootstrap --matching-inventory
```

Extraction refuses to overwrite existing files. Decompilation downloads pinned
Ghidra/XboxDev loader releases into `.tools/`, requires JDK 21+, and checks the
matching executable identity. Saved projects and pseudocode exports remain
under `analysis/`. See [pinned research and provenance](docs/RESEARCH.md) and
[Map Pack reference research](docs/MULTIPLAYER_MAP_PACK_RESEARCH.md).

## Find your way around

| Path | Contents |
| --- | --- |
| `src/`, `include/halo2/` | Native implementation and public interfaces |
| `shaders/` | GLSL renderer shaders |
| `tools/` | Setup, extraction, decompilation, audits and launcher tools |
| `tests/`, `cmake/` | Test sources, fixture guides and build configuration |
| `multiplayer/` | Master server, online services and their documentation |
| `data/tags/` | Authored native extensions, including custom Slayer variants |
| `external/`, `resources/` | Source dependencies, notices and supporting resources |
| `docs/` | Developer guides, implementation status and source evidence |

The [repository guide](docs/REPOSITORY_LAYOUT.md) explains local-only directories
and fixture ownership. For detailed status, start with
[MCC conversion](docs/MCC_GAMEPLAY_CONVERSION.md),
[campaign](docs/CAMPAIGN_PROGRESS.md),
[opening cinematics](docs/OPENING_CINEMATIC_STATUS.md),
[multiplayer](docs/MULTIPLAYER_PROGRESS.md),
[game modes](docs/GAME_MODE_PROGRESS.md),
[frontend](docs/FRONTEND.md),
[physics](docs/PHYSICS_PARITY.md),
[weapons](docs/MP_WEAPON_GAMEPLAY.md),
[damage/shields](docs/DAMAGE_SHIELD_COVERAGE_GOAL.md),
[rendering](docs/APPEARANCE_PARITY.md) and
[first-person presentation](docs/FIRST_PERSON_PARITY.md).
