# Source Unit construction and linked pose composition

Full live original parity remains unfinished. Normal `MultiplayerMatch::spawn`
still constructs typed native Units rather than invoking this source construction
chain. This module does not publish an incomplete source Unit into live gameplay.

`source_run_unit_object_runtime_1_1` links complete object request, header/body
allocation, type/Unit construction, manager binding and pressure callers to the
existing animation/pose and weapon routing graphs. All calls share retained
GP/stack/MMX/XMM/x87/control state and memory. Lower-owner failures preserve
preceding writes and the deepest retained ABI. Missing owners and resource banks
remain errors; the wrapper supplies no successful lower result, resource
residency, world admission or native typed publication.

The construction graph compiles 86 complete TU1.1 bodies and 6,485 instructions.
These include some already recovered owners; 86 does not mean 86 new algorithms.
Production uses static native C++ paths, with no opcode decoder, original-code
execution or fixture consumption. The generator records both the complete graph
and its generated ownership wrapper. Outputs reproduce byte for byte.

The independent Unicorn exporter executes unchanged original instructions
continuously through the first unavailable owner or resource read. Every
executed instruction is checked against the owned image bytes. Fourteen cases
compare 40,164 ordered stores, including private stack writes, and 406,128 final
bytes. Complete request, world record, pool/header/bitmap and retained logical
body banks are included. There are nine original returns, three actual missing
resource-bank stops and two first external-owner stops. There are zero supplied
lower results and zero replayed lower calls. The cases reach 7,069 unique
instructions across the linked graphs, not every branch in every compiled body.

Storage preparation executes original header-pool and body-heap initialization,
the original world-record clear, counter-constructor prefix and main-heap
constructor. Raw backing addresses, capacities and initial storage bytes are
declared platform inputs. The request, clock cells and game scalar cells come
from pinned earlier original executions. They are not new full startup or live
clock proof. No complete Unit body, component offset or readiness state is
provided during the compared construction calls.

The cases cover ordinary construction, body/component/header exhaustion,
genuine main-heap pressure, an explicit request seed, primary/secondary layer
queries and SMG/dual identity requests. Main-heap pressure comes from 89 actual
original allocations rather than edited counters. Layer and identity queries
use a genuinely source-allocated Unit after the interrupted construction prefix;
that preparation neither equips a weapon nor completes world publication.

The ordinary constructor produces a 5,750-byte logical Unit body, manager and
extension descriptors, binds the authored graph and installs initial activity.
Current-node sampling then stops at instruction `124DE8`, reading missing
animation-cache backing at `00000038`. Secondary firing queries stop at
`1E10C3`, reading an unavailable descriptor pointer at zero. Heap-pressure
construction requires the real `1C7970` world reclaim owner before allocation;
the explicit nonzero seed requires CRT helper `37A15A`. None is replaced by a
default or a forced return.

Tests also check absent-owner rejection and preservation of partial lower-owner
ABI mutations. Floating-point control assertions exclude MXCSR sticky arithmetic
status bits: this Unicorn comparison does not establish Xbox exception status
or CPU estimate behavior.

All 86 construction bodies and five normalization/lookup bodies have unique
complete normalized instruction/internal-branch matches in the owned TU1.5
image. The actual used jump tables and paired direct call targets are checked.
This static correspondence does not execute TU1.5 construction or establish
mutable global bindings, external-owner equivalence or gameplay parity.

Authority:

- TU1.1 XBE SHA-256: `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- TU1.5 XBE SHA-256: `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
- Lockout SHA-256: `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- Input pins and preparation boundaries: `tests/fixtures/unit_object_runtime_original.provenance.json`.
- Generated graph and wrapper pins: `tests/fixtures/unit_object_runtime_manifest.json`.
- TU1.5 correspondence: `build-unit-object-20261011/unit-object-u5.json`.

Reproduction:

```sh
.tools/python/bin/python tools/generate_unit_object_runtime_1_1.py \
  --output src/unit_object_runtime_generated.cpp \
  --manifest tests/fixtures/unit_object_runtime_manifest.json
.tools/python/bin/python tools/export_unit_object_runtime_fixtures_1_1.py \
  --output tests/fixtures/unit_object_runtime_original.tsv \
  --report tests/fixtures/unit_object_runtime_original.provenance.json
build-unit-object-20261011/unit_object_runtime_tests \
  tests/fixtures/unit_object_runtime_original.tsv
.tools/python/bin/python tools/research_unit_object_runtime_u5.py \
  --output build-unit-object-20261011/unit-object-u5.json
```

Remaining live work includes binding this construction path to the authoritative
native lifetime pool, actual animation cache initialization/delivery and
current/saved pose banks, completed world registration, complete inventory and
drop routing on those banks, and source Havok world/platform integration. The
native terrain mover remains in use. Passing these bounded construction/query
comparisons does not complete live blends, third-person layers, weapons,
grenades, admission/drop physics or original rendered sequences.
