# Source layout

Implementation files are grouped by the subsystem that owns their behavior.
Public headers remain in `include/halo2/`, so their include paths stay stable.

| Directory | Contents |
| --- | --- |
| `app/` | Game and launcher entry points, desktop session and loading |
| `assets/` | Cache readers, content, residency, tag ownership and MCC imports |
| `audio/` | Sound playback, decoding, spatial audio, DSP and native sound runtime |
| `campaign/` | Mission state, cinematics, AI, navigation and scripting |
| `core/` | Shared math, datum storage and presentation primitives |
| `gameplay/` | Simulation, animation, camera, game modes, objects, units, vehicles and weapons |
| `input/` | Controller and keyboard settings, rumble and training controls |
| `network/` | Sessions, replication, authority, peer transport and online services |
| `physics/` | Collision, world construction, contacts, solver and vehicle physics |
| `platform/` | Startup, memory allocation, threads, timers and movie playback |
| `rendering/` | Renderer, materials, textures, models, world geometry, effects, shadows and weather |
| `runtime/` | Native memory, execution, continuation, publication and scene bindings |
| `ui/` | Frontend, menus, HUD, widgets and Vista UI reference components |
| `tags/` | Existing binary tag directory and type system |
| `macos/` | Existing macOS application and platform adapters |
| `pstencil/` | Existing original-renderer reference and diagnostic implementation |

Keep generated `.cpp` bodies and `.inc` data with their owning subsystem.
Their existing names identify original owners and provenance; moving them does
not change their implementation or establish new runtime parity.

Private headers use paths relative to `src/`, for example
`#include "physics/runtime/physics_world_runtime_internal.hpp"`.
CMake provides this private include root. Standalone compiler tools must also
pass `-I<checkout>/src` alongside the public include root.

When adding a file, register its explicit subsystem path in the existing CMake
target. Generators that discover compiled owners search recursively; tools that
receive module basenames use `tools/source_layout.py` and reject duplicate owners.
Tests remain under `tests/`, shaders under `shaders/`, and offline reference
inputs and captured evidence retain their original paths.
