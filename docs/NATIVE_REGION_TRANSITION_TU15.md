# Native region transition parent, TU1.5

`native_tu15_region_transition` owns the complete `BC720` parent's decisions,
signed region count, and DWORD/byte snapshot loops. Its compatibility boundary
preserves the original guest stack, snapshot DATA lifetime, registers, defined
flags, direction, operand reads, stores, and `RET 16`. It keeps the original
unchecked count conversion and late argument/model-tag reads. It supplies no
object admission, model result, RNG value, attachment result, or effect result.

The TU1.5 route in `death_cycle_runtime_tu15_generated.cpp` selects this kernel.
The model-state owner `1704C0` and the four external attachment/model/object
owners retain their real existing execution. They have not all become semantic
native kernels through this parent change. The older literal body remains
available, and the generator installs the native route through a guarded helper.

## Original-code qualification

Authority is the English Xbox TU1.5 XBE with SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The offline reference mounts authenticated original code gaps alongside actual
native-entry BEFORE banks. These inputs are captured from current MCC/native
gameplay; they are not original Xbox hardware captures.

`qualify_native_region_transition_actual_tu15.py` reads only `capture-entry.json`
and its BEFORE banks. The original parent executes on physical i386. The C++
native parent independently starts from the same BEFORE; each remaining callee
executes unchanged original instructions on that side's current mutable banks
and genuine stack. Neither side receives captured native AFTER bytes, prepared
callee outputs, allocation results, effect results, or instruction patches.

The retained passing qualification consists of:

| Input | Compiler | Scope |
| --- | --- | --- |
| Actual Warthog, nine regions, requested state 4 | GCC and Clang | Complete return and all four actual callees |
| Model absent; region counts 0, 1, 2, 3, 4, 7, 8, 12, 15, 16 | GCC and Clang | Eleven explicit component inputs, complete returns |
| Declared type-6 detached, flag-2, refresh branches | GCC and Clang | Three prefixes stopping before `1704C0`; the refresh callee executes genuinely |
| Actual power module, one region, requested state 3 | Clang | Complete return and all four actual callees |

These are 25 complete returns and six explicit prefixes, 2,197 physical parent
instructions, and all 83 parent instruction sites. Ordered parent accesses and
stores, GP state at every instruction, defined arithmetic flags, FP controls,
empty live x87 state, SIMD, call inputs/returns, and all 2,791,504,733 compared
memory bytes agree. That includes stack and snapshot bytes. The generated
observer's terminal zero-count REP cursor is explicitly excluded when matching
physical single-step instruction events; its memory accesses remain exact.
Linux/debugger IF/RF bits are outside the arithmetic-flags comparison.

Reports: [GCC](../build/test-results/vehicle-destruction-region-transition-20261101/gcc-matrix-v2/report.json),
[Clang](../build/test-results/vehicle-destruction-region-transition-20261101/clang-matrix/report.json),
[actual module](../build/test-results/vehicle-destruction-region-transition-20261101/power-actual/report.json).

Three earlier complete runs that changed a Warthog's type to 6 faulted inside
the genuine `1CE0E0` attachment owner at an indirect target of zero. Those
invalid objects are not complete scenery cases. Their later declared prefixes
qualify the parent's branch/argument work only; they do not replace that
failing callee or claim valid full type-6 gameplay.

Four private wrong implementations are rejected: wrong copy stride, state-base
stride, snapshot pointer, and requested-state formal. The
[negative controls](../build/test-results/vehicle-destruction-region-transition-20261101/negative-controls.json)
fail at original/native instruction ABI comparisons, after computing the
original expected execution independently.

## Gameplay and regeneration

The admission authenticates 22 transitive support headers against the active
frozen world and imports only the named parent/header/generator changes.
Unrelated root edits, borrowed test-data mounts and launcher runtimes are
preserved. The [eight-case frozen gameplay run](../build/test-results/vehicle-destruction-phantom-live-20261101/region-transition-eight-v2/refresh.json)
passes the seven vehicles and power module. It preserves 144 complete-world
restore/replay comparisons and 118 native eligibility decisions. The module's
native countdown due, eligibility, and original blast order remains 8, 9, 10.

[Regeneration](../build/test-results/vehicle-destruction-region-transition-20261101/regeneration-v2.json)
retains the complete parent and native route. Its only normalized difference
is the newer fixed-image lowerer's folding of `selected(identity,TU15)` into
the identical immediate. Both exact complete literal hashes are guarded;
other operand, branch, callee, or return changes are refused.

## Required references and storage

Only the required BEFORE archives are retained. Every archive member was
decompressed and hashed again; captured native AFTER files and build binaries
are excluded.

| Required input | Archive bytes | SHA-256 |
| --- | ---: | --- |
| `analysis/physics/native-region-transition-20261101-required-before/destruction-warthog/actual-native-before.tar.zst` | 12,459,595 | `23d550200ff5a05acdc389f716437f5e59cb74ab0d399ca55f5df6c13106f20c` |
| `analysis/physics/native-region-transition-20261101-required-before/destruction-power-module/actual-native-before.tar.zst` | 12,615,018 | `47d7267e906458b0fc4b6ad564dbcfb5549606cf953acc2c320ebb33d69c0807` |

The total new required archive size is 25,074,613 bytes. Existing list-agent
references remain protected. Reproduce an actual Warthog comparison with:

```bash
python3 tools/run_test.py -- .tools/python/bin/python -B \
  tools/qualify_native_region_transition_actual_tu15.py \
  --archive analysis/physics/native-region-transition-20261101-required-before/destruction-warthog/actual-native-before.tar.zst \
  --output build/test-results/region-transition-recheck
```

Add `--matrix --prefix-type-branches` for the declared component matrix or
`--compiler clang++` for Clang. All expanded banks, source copies and binaries
stay inside the enclosing test owner's lifecycle.

## Remaining scope

This does not close full-definition, visible, audible, all-mode or network
acceptance. The gameplay fixture is muted. Proper source sound requests,
original waveform/device output, both combined loops, and exact physical wet
feedback format/time remain open. No Xbox hardware is available; the user's
instruction is to continue native work without guessing those measurements.
All 37 complete-definition gates, D01-D13 and the complete destruction/audio
goal remain active. The next semantic owner is `1704C0`, including its genuine
candidate/RNG and effect stop/create work.

The [next-owner observation](../build/test-results/vehicle-destruction-region-transition-20261101/next-model-owner-observation.json)
now traces actual original `1704C0` execution for both retained inputs. Warthog
changes state 2 to 4 and the module 0 to 3. Each original collector produces one
candidate and the real selector chooses index zero. Both old effect handles and
chosen effect tags are `NONE`, so these particular calls execute no effect stop
or creation owner. They cover 117 of the child's 154 sites; 37 sites and those
effect branches remain unobserved. These traces guide the next conversion and
do not claim a semantic native `1704C0` implementation or sound acceptance.

The subsequent [native model-state work](NATIVE_MODEL_REGION_STATE_TU15.md)
implements and independently qualifies that child, including all five real
callees and its authored effect-allocation branch. Its fresh gameplay
integration is tracked separately. The original finite BC720 build owner has
closed and removed its source/build tree; required BEFORE archives remain.
