# Native first-item motion

Native `F9B10/F9EA0` matches all six original core calls: the first resting BR,
falling BR with a terrain miss, and SMG with an actual terrain contact. Both
normal and address/undefined/leak sanitizer comparisons pass. The comparison
uses 29 literal states and checks complete occupied bodies, including embedded
current nodes, all captured other raw buffers, exact producer packets and
aliasing, defined F9B10 AL and **58 ordered native body stores**. Authority:
[independently repeated original motion](MP_ITEM_MOTION_BOUNDARIES_RESEARCH.md).

Gravity, endpoint formation and subtraction retain the original separate
float operations. The terrain-contact path preserves original normal/speed
gates, effect and transform order, actual point/normal inputs, angular
projection, resting/contact fields, velocity stores and the angular Y signed
zero. `B8630`, the measured `BAA80` resting branch and `FBAC0`'s actual
zero-angular branch are native owning operations. Physical component NONE
retains the original early notification exit; it is not a completed solve.

Twelve invocations of `BC910`, `16B240`, `B8490`, `FAF20` and `FBBE0` remain
explicit producers. The test checks their full entry states and packets and
then copies their actual original output buffers in place. This does not
implement their moving-state registration, terrain/world query, transforms,
node/bounds/location/registry updates, marker math or audio services.
Original FBBE0 takes the contact point at cast+8 and normal at cast+28h,
then writes a separate adjusted point. The older frozen field name
`eax_matrix34_output` is a literal over-wide input window, not the routine ABI.
The interface correction retains that immutable window, proves it unchanged,
and compares the actual adjusted point. Its normal and sanitizer comparisons
are recorded separately in `item-motion-abi-correction*-differential.json`.

The native functions fail when an unrecovered impulse, timer/effect, queue,
physical motion, alternate impact, lifespan or rotating pose producer is
needed. F9EA0's raw EAX is not promoted to an output contract. Alternative
listed branches have no measured coverage here. This is one first-item
schedule; continuous settling, complete live frame/world/physics, additional
contact contestants and Xbox numerical parity remain open.

## Reproduction

```sh
cmake -S . -B build-multiplayer
cmake --build build-multiplayer --target item_motion_tests -j4
python3 tools/differential_item_motion.py
ctest --test-dir build-multiplayer --output-on-failure -R '^retail_item_motion$'

ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=halt_on_error=1 \
python3 tools/differential_item_motion.py \
  --binary build-multiplayer-asan/item_motion_tests \
  --output analysis/multiplayer/integration/item-motion-asan-differential.json
```

The exporter checks six pinned authority files, all173 earlier dependencies,
all49 instruction files and byte-identical report/listing/audit repeats. Source
definitions, clock/gravity/constants, dormant scratch and expected body/store
buffers come from literal original observations. No expected result is
calculated by the native implementation. The fixture remains in ignored local
analysis storage; the CTest check skips77 when absent.

Both new command/motion CTest checks pass normally and with focused sanitizers.
The initial exporter used a duplicated instruction-directory prefix; its
failure is retained in `item-motion-first-export.log`. Correcting the exporter
path required no original fixture or native numerical changes. A fresh
source-only snapshot builds and runs71 checks:18 pass and53 original-data
comparisons skip, including both new command/motion checks. Its836 copied
files are pinned in `code-only-item-motion/report.json`; later documentation
and shared source edits remain outside that snapshot. Captures57/58 use the
rebuilt runtime containing both command/motion modules: title startup, four-view
Zanzibar firing/reload, and a loopback dedicated host with three participants
across one/two-view clients. Runtime/launcher/shaders remained stable during
each run and Vulkan validation reported no errors. Captures55/56 remain
historical. These are rendered smoke checks; the source frame route and full
physics/multiplayer integration remain open.

Native matrix/current-node/bounds producers are now verified separately;
[producer scope](MP_ITEM_TRANSFORM_NATIVE_INTEGRATION.md). They still require
their source world/location/registry owners before replacing the complete
motion service chain. Captures57/58 predate the ABI correction and these new
producer modules; their exact original build remains pinned separately.
