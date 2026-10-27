# TU1.5 native pose and island-controller component proof

The native pose owner and island-deactivation controller passed independent
comparisons against unchanged English TU1.5 instructions executed as i386 code
on this workstation. The goal still includes live MCC multiplayer qualification,
the remaining custom body policies, and original Xbox reciprocal-square-root
estimate-bit qualification.

The original executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
TU1.5 is the sole source authority for these modules.

| Native owner | Complete original body | Native execution |
| --- | --- | --- |
| Motion pose `325170` | 99 instruction sites | Position/history, quaternion advancement, angular travel and origin publication |
| Quaternion product `323430` | 58 sites | Original cross/scale/dot grouping and extended W product |
| Quaternion normalization `1E1010` | 31 sites | Actual native `RSQRTSS` and the original Newton sequence |
| Quaternion matrix `37C5D0` | 69 sites | Original extended X/Y intermediates, binary32 Z spill and scalar output order |
| Island deactivation `320C80` | 119 sites | Strict timer thresholds, wrapping, live body-policy iteration and signed 8-bit check counter |
| Default body policy `3233C0` | 3 sites | The original false output byte, consumed as a semantic bool |

The source files are `native_tu15_motion_pose.hpp/.cpp` and
`native_tu15_island_deactivation.hpp/.cpp`. Their algorithms use typed storage
views, native SIMD operations and the host floating-point backend. Each SIMD
arithmetic operation in the pose kernel locks destination/source order to
preserve distinct NaN payloads across compilers. Original addresses identify
data layouts, function ownership and optional store provenance.

## Independent original comparisons

`tools/observe_native_motion_pose_tu15.cpp` single-steps the unchanged original
code. Authenticated instruction descriptors identify retained memory operands;
the observer records their actual read order and before/after store bytes.
It also records physical `RSQRTSS` inputs/outputs and the complete x87 control
word, status word and MXCSR. Source instructions and arithmetic results remain
unchanged throughout reference execution.

Capstone operand metadata requires two architectural corrections: `LEA`
computes an address, and x87 `FST`/`FSTP` write their memory destinations.
The validators apply those corrections before recording any observation.

| Retained result | Native pose | Native controller |
| --- | ---: | ---: |
| Physical original cases | 210 | 290 |
| Ordered retained reads | 8,229 | 3,254 |
| Ordered retained stores | 4,301 | 1,038 |
| Original instructions executed | 52,942 | 13,154 |
| Read-fault prefixes | 7 | 19 |
| Distinct paired-NaN controls | 24 | 16 |
| Actual native/original `RSQRTSS` observations | 206 | — |

Both GCC 16 and Clang 23 passed with `-fno-fast-math`, `-ffp-contract=off` and
`-frounding-math`. Each case compares every retained byte and complete FP
context. A separate pass compares the ordered data reads without diagnostic
preimage reads. Controlled read-fault cases stop the original immediately before
the failing operand, then compare the native prefix, preceding stores and FP
state.

The pose mutation replaces `RSQRTSS` with precise `1/sqrt`; the unchanged
physical original rejects its estimate. The controller mutation treats the
check counter as unsigned; the original rejects its ordered state stores.

Controller cases include 263 complete original calls, of which 12 execute the
real default policy through authenticated vtable `41F1B0`. Eight cases stop at
the actual custom policy entries `31E5C0` and `31E690` selected by vtable
`41EB6C`. Their outcomes remain owned by those unconverted policies.

Additional controller model regressions cover callback changes to body counts,
body lists, counters and FP context; count caching before a wrapping store;
continued iteration after a failed/null policy; output/counter aliasing; and
timestep/elapsed aliasing. These models qualify the dependency contract
separately from original execution.

## Reproduction and retained evidence

Run from the repository root:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_motion_pose_tu15.py \
  --output build/test-results/native-motion-pose-tu15

python3 tools/run_test.py --scratch-root .local/test-scratch -- \
  .tools/python/bin/python tools/validate_native_island_deactivation_tu15.py \
  --output build/test-results/native-island-deactivation-tu15
```

The bounded reports are
`build/test-results/native-motion-pose-tu15/report.json` and
`build/test-results/native-island-deactivation-tu15/report.json`. They retain
commands, compiler results, source/listing hashes, fixture hashes and explicit
scope limits. CMake logs remain beside each report.

The final raw pose fixture was 8,701,945 bytes; the controller fixture was
14,645,902 bytes. Both fixtures, original reference ELFs and private build trees
were removed by their owned test lifecycles. Removal was verified for the final
pose owner `halo2-test-t5rpy8kn` and controller owners `halo2-test-q6o94fih` and
`halo2-test-rlasp2de`. Retained large artifacts: none.

## Acceptance still open

- Original Xbox/Pentium III `RSQRTSS` estimate-bit parity remains open. The
  current comparison executes original instructions on this host.
- Custom deactivation policies `31E5C0` and `31E690` still require native ports.
- Unmasked platform traps and device/unbacked vector semantics remain outside
  this component qualification.
- Live world/controller composition, visible MCC multiplayer, vehicle-camera
  acceptance and full native-only physics closure remain tracked by the main
  physics goal and checklist.
