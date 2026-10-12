# Repository guide

The Git checkout contains the native implementation, shaders, developer tools,
test sources, and documentation. Game installations and generated research
data are local inputs. `.gitignore` keeps them out of ordinary `git add` and
GitHub checkouts without deleting or relocating the working copies.

## Source map

| Path | Purpose |
| --- | --- |
| `CMakeLists.txt`, `cmake/` | Build targets, dependencies, compiler settings and test registration |
| `include/halo2/` | Public C++ interfaces; `tags.hpp` is the shared tag-system entry point |
| `src/` | Native cache readers, runtime systems, frontend, renderer and launchers |
| `src/tags/` | Tag-system implementation |
| `shaders/` | GLSL shader sources; compiled SPIR-V stays in the build directory |
| `tools/` | Python/C++ extraction, setup, decompilation, source oracles, audits and capture utilities |
| `tools/ghidra/` | Ghidra scripts used by the decompilation workflow |
| `tests/` | C++/Python tests and GPU test shader sources |
| `tests/fixtures/` | Tracked fixture documentation/generators; generated payloads stay local |
| `multiplayer/master_server/` | Python master server, online services, tests and deployment examples |
| `multiplayer/docs/` | Discovery, matchmaking, identity, party, clan and voice documentation |
| `external/` | Vendored source dependencies and their provenance/license notices |
| `resources/launcher.svg` | Project launcher icon |
| `docs/` | Implementation guides, recovered source evidence, validation and outstanding work |

Generated C++ such as `src/*_generated.cpp` remains publishable source because
the native build consumes it. The Ghidra exports under `analysis/` are separate
local pseudocode outputs. `external/imgui` and other source dependencies remain
available to build from a checkout; retain their accompanying notices.

## Local-only directories

| Path | Local contents |
| --- | --- |
| `assets/` | Extracted Xbox discs/maps, imported Vista/MCC packs and source-selection registries |
| `Halo 2 Vista installer/` | User-supplied installer archives, executables and extracted files |
| `resources/ui/` | Optional local artwork, font, SVG and atlas bundle |
| `analysis/` | Ghidra projects/exports, disassembly, emulator captures, benchmarks and reports |
| `tests/fixtures/*` except `*.md` and `*.py` | Generated reference tables, original-byte dumps, framebuffer captures and oracle results |
| `tools/reference/` | Downloaded reference repositories |
| `.tools/` | Downloaded tools, Python environment and tool caches |
| `.local/` | Local service state, credentials, SDK downloads and working backups |
| `build*/`, `cmake-build*/`, `out/`, `dist/` | Build and packaging outputs |
| `Halo2-*`, `Halo 2 Native*.desktop` at the root | Installed launchers, previews and desktop shortcuts |
| `.ark-*/` | Temporary archive extraction directories |

Disc images, archives, game executables/maps, compiled binaries, runtime caches,
Python caches, environment secrets and editor backups are also ignored. Example
environment files remain available. Preserve the established local paths: the
launchers, setup tools and reference checks use them.

## Start here

- [README build and run instructions](../README.md#build-and-run)
- [Game-file setup and source selection](ASSET_SETUP.md)
- [Native systems, controls and boundaries](NATIVE_SYSTEMS.md)
- [Port status and remaining engine work](PORT_STATUS.md)
- [Rendering and appearance parity](APPEARANCE_PARITY.md)
- [Frontend implementation](FRONTEND.md)
- [Campaign progress](CAMPAIGN_PROGRESS.md)
- [Multiplayer progress](MULTIPLAYER_PROGRESS.md) and [gamemode coverage](GAME_MODE_PROGRESS.md)
- [Master-server setup](../multiplayer/master_server/README.md)
- [Shared tag system](TAG_SYSTEM.md)
- [Pinned research sources](RESEARCH.md)
- [Optional local UI bundle](UI_ASSET_BUNDLE.md)

## Tests and reference data

A fresh Git checkout does not include native reference fixture payloads or
the game files used to create them. The test sources and generators remain
available in `tests/`, `tools/`, and the Python files in `tests/fixtures/`.
Use the associated fixture Markdown and subsystem documentation to identify
the original source and capture/export procedure. Some fixtures require an
emulator capture or additional local analysis inputs; they cannot all be
regenerated from an ISO alone.

The full CTest suite is not self-contained: some asset-dependent tests skip
when inputs are missing, while others fail. Run those checks after preparing
their documented data. Compact numerical service-test fixtures under
`multiplayer/master_server/tests/fixtures/` remain with the service tests.
They are distinct from the bulk native capture directory.

To review what an ordinary add would include without staging anything:

```sh
git status --short
git add --dry-run .
```

Ignore rules do not remove files already tracked by Git or erase committed
history. At this cleanup, only `README.md` and `LICENSE` were tracked, so no
asset removal from the index or history was needed. Avoid forcing ignored
game data or capture payloads into a source commit.
