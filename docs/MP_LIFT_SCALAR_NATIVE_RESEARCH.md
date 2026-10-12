# Native lift scalar consumer, 2026-10-10

`source_lift_scalar` now reproduces the delta and ratio outputs of original
TU1.1 `1D5C70` and supplied u5 `1DA530`. This is a scalar consumer; it does
not yet make the map's gravity lifts carry a player. Actual phantom contact
admission, body mass/velocity, marker frames, vector force delivery, activation
and world stepping remain required inputs.

The new header and implementation are
`include/halo2/source_lift_scalar.hpp` and `src/source_lift_scalar.cpp`.
The selected executable's epsilon, one, half and reverse sentinels are explicit
operands. No source constant packet or authored lift force is embedded in this
module. The independent oracle imports their bytes from the owned executables.
The current runtime does not call this leaf or provide those operands.

Each image independently executes 31,232 unchanged original calls: the existing
512 authored-rule scalar fixtures and 30,720 explicitly declared helper controls.
The latter vary reverse permission, mass scaling, mass, forced acceleration,
acceleration, current velocity, target, velocity limit and supplied time step.
They are not measured player contacts. Both images produce identical output
pairs, including signed zero and branch-boundary results. The native normal and
ASan/UBSan comparisons match all 62,464 delta/ratio pairs exactly.

Separate binary32 operations retain the source braking and clamp order. The
square-root branch retains the original x87 53-bit product, doubling and root
before one binary32 store. Ratio uses the incoming velocity/limit independently
of the later delta clamp. Positive-exponent rules require a separate owner of
the original pow/multiply/store sequence; missing service throws when that
branch is requested. It is not replaced by host `pow`. These fixtures do not
execute that owner, FP exception/status equivalence, NaN/denormal behavior or
an original clock producer.

The CMake target is `source_lift_scalar_tests`; its original fixture is ignored
analysis data. Both independent oracle runs and the current-source normal and
sanitizer commands/results are pinned under
`analysis/multiplayer/integration/lift-scalar-native-20261010`. Earlier source
and delivery freezes are unchanged. The fixed revision4 launcher predates this
new leaf, which has no live effect until the actual contact owner is connected.

Reproduce into a fresh output directory:

```sh
PYTHONPATH=tools .tools/python/bin/python tools/probe_source_lift_scalar_native_20261010.py --output /tmp/halo2-lift-scalar-repeat
cmake --build BUILD_DIRECTORY --target source_lift_scalar_tests
BUILD_DIRECTORY/source_lift_scalar_tests /tmp/halo2-lift-scalar-repeat/fixture.txt
```

The original owners and earlier 512-case boundaries remain documented in
[lift source research](MP_SANDBOX_EXPLOSIVES_LIFTS_SOURCE_RESEARCH.md) and
[later-update correspondence](MP_SANDBOX_LATER_UPDATE_SOURCE_RESEARCH.md).
