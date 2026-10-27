# Native TU1.5 body deactivation policy proof

Actual short-period `31E5C0` and long-period `31E690` are complete native
algorithms in `native_tu15_body_deactivation_policy.hpp/.cpp`. Each original
root contains 66 sites and has no callees. Original constructor store `31E84B`
installs vtable `41EB6C`; its policy slots `+14/+18` select these exact owners.
The new module contains no virtual CPU, register/flags machine, lifted root,
emulated numerical instruction, external math result, or residual dependency.

Both policies compare current motion position/orientation against their own
previous state. The period selects saved position `10/30`, saved quaternion
`20/40`, linear threshold `50/58`, and angular threshold `54/5C`. The actual
body motion is read through body `+3C`; current position is motion `+B0` and
quaternion is motion `+30`. Policy `+8` scales the angular displacement measure.

The native implementation preserves original binary32 SIMD subtraction,
squaring and sum grouping. Ordered native assembly instructions lock operand
priority even for distinct NaNs. The distance uses three lanes, while the
orientation displacement uses all four. All source vector lanes still execute
their original SIMD arithmetic and can affect exception flags. Threshold
squaring and strict comparisons use real native x87 intermediates. Original
quaternion arithmetic occurs before the first comparison is consumed.

A true result only writes the caller's private output byte. A false result
first publishes the captured current quaternion, then rereads the body's
actual motion and current position, publishes the position, and writes false.
The native function returns the same boolean while preserving the private
output contract; it does not fabricate original volatile register outputs.

`build/test-results/native-body-deactivation-policy-tu15/report.json` records
unchanged physical i386 original execution from English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

- 432 original cases, including 400 complete calls and 32 read-fault prefixes.
- 48 paired NaN cases, strict threshold equality/adjacency controls, exceptional
  values, signed zero, denormals, rounding/control states, and output aliases.
- 924 original ordered stores, 3,560 ordered reads, and 22,702 instructions.
- Actual aliases in both periods where quaternion publication overwrites the
  current position, and where it changes body `+3C` to a different motion.
- Full numerical FP control/status comparison, plus preserved host FP state.
- GCC 16.2.1 and Clang 23.1.4 Release passes with strict FP options.
- An unchanged-original negative control rejecting a private mutation that
  admits non-strict distance comparisons.

The source fixture is generated from original physical instructions. The
required backend name and pinned image are checked by the C++ parser; missing
fixtures fail. No original branch result or policy answer is injected.
Read-fault prefixes compare retained memory and numerical FP status at the
original fault boundary; they do not establish resumable host fault delivery.
Incoming live x87 values remain owned by the caller's real context bridge.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_body_deactivation_policy_tu15.py \
  --output build/test-results/native-body-deactivation-policy-tu15
```

The 7,522,261-byte generated fixture, original ELF/observer, object files and
private compiler builds were removed with owned scratch
`.local/test-scratch/halo2-test-eg8murmc`. Both that path and the preceding
`halo2-test-374ciyvy` path were checked absent. Reports total 11,269 bytes; all
five source hashes match. No large artifact was retained.

Main checker/world integration remains root-owned. This component proof does
not establish full composed live-world parity, visible multiplayer acceptance,
or native-only closure for the complete engine. Original unmasked platform
trap ownership remains explicitly unsupported. These policies use no numerical
estimate instruction and introduce no RSQRT estimate gap.
