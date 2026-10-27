# Native TU1.5 constrained motion commit proof

`native_tu15_motion_commit.hpp/.cpp` implements the three complete actual
constrained-motion commit owners as typed native math:

| Actual owner | Sites | Authentic motion table | Behavior |
| --- | ---: | --- | --- |
| `3235F0` | 88 | `41F1E0+10` | Prescribed velocity pose integration |
| `3249E0` | 129 | `41F2E0+10` | Integrate scaled solver velocities |
| `3255D0` | 156 | `41F360+10` | Integrate, then restore saved velocities |

Each receives motion, step packet, binary32 commit parameter and accumulator.
The actual semantic return is accumulator plus `80` bytes. The kinematic owner
consumes neither the parameter nor accumulator payload; physical cases include
an unbacked accumulator identity, wrapping return cursor and an unused signaling
NaN parameter. The original calls use `RET12`; the native API returns a typed
cursor without fabricating a CPU postimage.

The dynamic owner transforms working local angular velocity through the old
matrix, scales working linear/angular velocities, and masks both before pose
integration. Its half-angular construction reads the mask before angular
velocity. The saved-velocity owner integrates unmasked working velocities,
retains the original linear read before its angular store, and updates pose.
It then restores saved linear velocity without scaling and transforms saved
angular velocity through the updated matrix. Masking occurs after restoration,
with source-specific linear and angular read order. These algorithms cannot be
replaced by free-step pose `325170`.

Closed quaternion product `323430` (58 sites), normalization `1E1010` (31),
matrix construction `37C5D0` (69), and the appropriate quaternion center rotation
`2E5810` (62) run as real native x87/SSE math. The code locks ordered SIMD
operands and grouping, uses actual native RSQRTSS and original Newton operations,
and preserves source memory reads, stores and numerical FP state. There is no
guest register/flags file, emulated instruction dispatcher or residual math.

`build/test-results/native-motion-commit-tu15/report.json` pins unchanged
physical i386 TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` and backend
`unchanged-tu15-motion-commits-physical-operands-rsqrt-full-fp-v1`:

- 664 original cases: 522 complete calls and 142 read-fault prefixes.
- 92 paired NaN cases, exceptional inputs, nonzero W lanes, rounding controls,
  motion/step/accumulator aliases and quaternion edge controls.
- 13,846 ordered stores, 28,406 ordered reads and 191,178 original instructions.
- Exact actual returned cursor, store PCs/order/bytes, retained bank bytes,
  x87 numerical control/status, MXCSR and 594 original/native RSQRT observations.
- GCC 16.2.1 and Clang 23.1.4 strict-FP Release passes.
- Three private negative controls rejected by unchanged original: precise
  reciprocal-root substitution, scaling the unscaled saved-velocity restore,
  and changing the returned accumulator stride from `80` to `70`.

The required parser rejects absent or mismatched physical evidence. A binding
must derive one authenticated commit kind and use it for both the memory view
and native call. The component tests begin with an empty true x87 stack and vary
TOP/status; actual incoming live-value transport remains the caller's contract.
Unsupported depth, unmasked traps or unavailable native math diagnose instead
of falling back. Xbox/PentiumIII RSQRT estimate-bit qualification and device or
unaligned memory trap semantics remain separate gates.

```sh
.tools/python/bin/python tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_motion_commit_tu15.py \
  --output build/test-results/native-motion-commit-tu15
```

The 30,569,807-byte generated fixture, original ELF/observer, memory banks,
objects and all compiler builds were removed with scratch `halo2-test-jv33blsl`.
The initial proof scratch `halo2-test-a0sxl4_s` was also removed. Both paths were
checked absent and all five recorded source hashes match. No large artifact was
retained. Native core solver closure, complete constrained-world composition,
live comparison and visible multiplayer remain separate gates.
