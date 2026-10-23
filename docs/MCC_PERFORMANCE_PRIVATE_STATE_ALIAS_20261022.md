# Private execution-state alias hypothesis

Research date: 2026-10-22. No production edits or full/runtime builds.

## Recommendation

**Do not add a restricted state reference as a performance change on the
strength of this experiment.** The inspected shadow runner supports a narrow
non-escape contract for its privately constructed `LiveState`, but adding
`__restrict__` to the unchanged hot classification excerpt produced identical
normalized assembly in both GCC 16.2.1 and Clang 23.1.1. It removed no machine
instructions or loads/stores in that component. Timing differences during a
concurrent full build are noise, not a speedup result.

This result applies to the tested private-wrapper/classification component.
It does not prove that a qualifier cannot help another complete routine, and
does not justify annotating every generated `State` function or public memory
parameter. The private C++ execution object and the externally owned source
memory are separate alias domains. Source memory, callbacks, observers and
cursor targets retain their existing aliasing and mutation semantics.

A single adjacent exception-edge experiment also found **no performance gain**:
an out-of-line `noexcept` wrapper performed each original checked read exactly
once, captured any exception, and rethrew on a caller cold branch. It preserved
the bounded callback/fault results but added overhead. This is not a production
API proposal or proof of a nonthrowing retained-memory admission contract.

## Escape and alias audit

The scope audited is `source_run_shadow_frame_tu15`, its `LiveState` subclass,
the inherited `WorldState`/`State` methods, the module's helper boundaries,
and the scalar/vector source-memory helpers. Other generated modules were
checked for analogous local construction, but were not given a complete
module-wide escape audit.

| Boundary | Observed ownership and implication |
| --- | --- |
| Public entry | The runner constructs `LiveState s{m,observe,services,image}` on its own stack, then calls `s.load(abi)`. The caller supplies memory/services/ABI, not a preexisting `State&`. The private object is not returned. |
| Input/output ABI | `load` copies GP, flags, x87 lanes/status and each XMM value. `save` creates a distinct value object and copies them back. ABI arrays and optionals contain values, not references into `State`. Caller ABI may alias externally owned storage without thereby aliasing the private copied register file. |
| Required owner | `required` does `auto a=save()`, passes this separate ABI object to the callback, then `load(a)` on success **and exception**. It does not give the callback `State&`, GP/XMM spans or a pointer to private flags. `WorldState::required` rejects active MMX before forwarding to this base method. |
| Memory adapters | `r` gives `read` a local four-byte array; `v` gives `read_vector` a local 16-byte array. `w`/`vw` create local written-byte values or a distinct store-record object. The adapters receive source addresses and these temporaries, not private register-array storage. Pointer/span lifetimes still apply. |
| Observers | The ordered observer receives a separate `{instruction,address,before,written}` record. Write-policy observers are reached through the external memory owner. Neither interface receives the private execution object. Their arbitrary valid external mutations and exceptions remain permitted. |
| Instruction services/cursor | The callback receives a PC value. `instruction_cursor` points into the **external** services object. The ordinary world supplies `&last_instruction` and external trace flags; these are not addresses of `s.gp`, `s.flags` or `s.xmm`. A custom valid cursor may alias caller ABI/external source storage; no restriction may be imposed on those pointees. |
| Other services/helpers | Reciprocal-square-root receives an input value; timestamp receives no State argument. Internal helpers receive references derived from `s`, or references to private FP fields for known native arithmetic. The MMX helper copies FP bytes into a local value. No inspected helper stores a private State pointer in external storage or passes it to a user callback. |
| Exception publication | The public runner catches around dispatch and does `abi=s.save()` before rethrowing. Intermediate GP/XMM/flags/FP mutations therefore remain observable on failure. `restrict` is not permission to discard state on unwind or move original source effects past a possible fault. |

Primary source locations:
[private construction and dispatch](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L19679),
[exception save](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L19838),
[State load/save/required](../src/physics/runtime/physics_contact_runtime_internal.hpp#L170),
[WorldState forwarding/MMX](../src/physics/runtime/physics_world_runtime_internal.hpp#L7),
[source-memory temporary buffers](../src/physics/runtime/physics_step_internal.hpp#L12),
[observer and memory interfaces](../include/halo2/physics_contact_record.hpp#L15),
[service/cursor public contract](../include/halo2/physics_contact_runtime.hpp#L39),
[ordinary cursor storage](../src/physics/physics_live_world_tu15.cpp#L1035),
[LiveState local MMX representation](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L12).

The narrow source-supported statement is: **valid callers entering through
this inspected public runner do not receive another access path to the private
register/flag/FP object**. References passed from the restricted state itself
to internal methods remain derived accesses; they do not cease to exist.
This does not promise that called methods cannot modify State, nor does it
restrict aliased objects reached through its borrowed references.

The lower-level State headers have public fields and permit other code to
construct State objects. A private direct fixture can arrange aliases that
the inspected public wrapper never exposes. Its passing result cannot establish
the admission contract for public callers, or authorize a blanket header/method
annotation. Any broader transform needs a complete escape audit for each
actual public constructor/dispatcher and all its helper paths.

## Compiler contract

G++ documents `T &__restrict__` as a restricted reference in its local context;
the extension is not a standard C++ keyword. It also documents a separate
restricted member-function `this` qualifier. Those are distinct choices; this
probe only qualified the private helper's reference parameter.
[GCC restricted pointers/references documentation](https://gcc.gnu.org/onlinedocs/gcc/Restricted-Pointers.html).

Clang's primary parser source explicitly accepts restrict on references as an
extension. The official LLVM reference defines `noalias` in terms of modified
memory reached through based pointer values, with caller/callee responsibility;
violating that promise is undefined behavior. It is a contract to prove, not
an advisory optimization hint. The Clang documentation page fetch was unavailable,
so reference syntax was checked against primary compiler source and local
Clang-emitted IR as well.
[Clang reference parsing, lines 6218–6234](https://github.com/llvm/llvm-project/blob/main/clang/lib/Parse/ParseDecl.cpp#L6218),
[LLVM noalias contract](https://llvm.org/docs/LangRef.html#parameter-attributes).

A tiny positive control `S&` versus `S&__restrict__` around an opaque call
confirms Clang emits `noalias` for the restricted reference and replaces the
post-call reload with constant 1. GCC accepts the syntax but retains the reload
in that control. The hot-code equality below therefore is not explained by
Clang silently rejecting or ignoring the spelling. Both control assemblies and
Clang IR are retained in the evidence JSON. No `__declspec(noalias)` function
attribute, `pure`, `const`, `noexcept` or memory-side-effect annotation was used.

## Scratch-only current-source experiment

The canonical shadow source SHA-256 was
`cf7341f8fd24518b5eb478e136dcc48ea8517c2a7df4906924c5e2ffca177d42`.
The existing build-local operand/cursor specialization was applied equally
to both copies. From that result the probe extracted the unchanged 45 original
instruction statements `2E66A8..2E6748` in `run_2e6290`. Exit branch labels were
represented by distinct return codes at the excerpt boundary. All original
loads, stores, GP/XMM/flag operations, cursor delivery and loop back edges in
the excerpt remained present. The only difference inside the two helper
definitions was:

```cpp
classification_baseline(WorldState &s)
classification_restricted(WorldState &__restrict__ s)
```

Each separate translation unit used a public-style runner that constructs a
fresh private `LiveState`, loads the input ABI, calls the helper, and saves
the ABI on success or exception. This avoids giving the callback a direct
fixture State pointer. The helper was `noinline`; the normal compiler's other
intra-unit analysis remained enabled. Consequently this is not a measurement
of the entire recursive `2E6290` compilation context or its full dispatch graph.
[Canonical excerpt](../src/rendering/shadows/source_shadow_frame_tu15_generated.cpp#L17630),
[existing specialization](../tools/specialize_native_operands.py#L34).

Build flags were `-std=c++20 -O3 -fno-fast-math -ffp-contract=off
-frounding-math -ftrapping-math`, with current `include/` and `src/` headers,
and no LTO. Scratch executables linked only this small component/driver, not
a runtime build. Both compilers ran these baseline-versus-qualified comparisons:

- 840 controlled cases: 120 deterministic plane inputs, one to four batches,
  accepted/rejected classifications and seven callback modes.
- 5,158 injected fault comparisons, failing every observed read/write position
  for the first eight inputs in each callback mode. Full failure-prefix state
  was compared, including partially modified ABI and source banks.
- 14 separate required-owner bridge comparisons: the callback changes the
  copied ABI, returns or throws after changing it, and can encounter a source
  read fault. This bridge exercises `required/save/load` separately; it is not
  part of the extracted classification algorithm.

Comparisons include GP, all XMM words, flags, FS base, x87 lanes/depth/status,
x87 control word/MXCSR, all source-memory bytes, ordered source read/write
events with PC cursor, ordered store observations, instruction callbacks,
exit code and failure text. Callback modes cover full tracing, ordered store
observation, trace flags changing during reads, source plane/mask mutation,
generic instruction dispatch, cursor aliasing caller ABI storage, and an
external callback replacement. Active MMX and allocator/diagnostic calls are
absent from this excerpt and are not claimed as covered. Expectations come
from the unchanged current component, not an independent Xbox fixture.

| Compiler | Baseline classification size | Qualified size | Normalized disassembly |
| --- | ---: | ---: | --- |
| GCC 16.2.1, 20260810 | `0x2E05` hot + `0x95` cold bytes | same | Identical |
| Clang 23.1.1 | `0x19CA` bytes | same | Identical |

Normalization replaces only the baseline/restricted symbol-name spelling.
Addresses, machine bytes, operands and relocations otherwise agree in the
retained classification listings. Counts of `mov` instructions in this listing
are not presented as dynamic spill counts.

Seven alternating runs of 100,000 public-style calls each produced noisy median
times of 1473.82/1502.99 ns (GCC baseline/qualified) and 1667.14/1856.73 ns
(Clang baseline/qualified). The checksums agree. These are whole component calls
including load/save, memory-provider and cursor work, during concurrent builds;
there is **no credible timing gain** to report. Identical generated code is the
stronger result. This is neither genuine gameplay coverage nor ordinary
1p/4p FPS acceptance.

## What this rules out and leaves open

The qualifier alone did not improve this measured seam. It does not remove
the memory-owner or cursor calls and does not establish that they are read-only.
The source also intentionally sends `this` to the out-of-line
`State::instruction` fallback; a restricted reference does not promise that a
callee explicitly given a derived State reference cannot modify it. Generic
callback paths and exception publication remain real optimizer constraints.
[Instruction fallback](../src/physics/runtime/physics_contact_runtime_internal.hpp#L129).

## Bounded exception-edge follow-up

At the parent's explicit request, one final private experiment compared four
variants of the same 45-statement classification excerpt:

| ID | Read path | State reference |
| --- | --- | --- |
| `b0` | Current `r`/`v` | Unqualified |
| `b1` | Current `r`/`v` | Restricted |
| `w0` | Out-of-line `noexcept` result wrapper | Unqualified |
| `w1` | Out-of-line `noexcept` result wrapper | Restricted |

For each wrapped scalar access, the wrapper called `m.read` exactly once with
the original address, output buffer and extent. For each wrapped vector access,
it called `m.read_vector` exactly once; the generic adapter still performed its
four ordered scalar reads. It returned an `exception_ptr` in a result object;
the caller tested that result and called a separate cold `rethrow_exception`
function on error. No read, check, store, observer or cursor update was skipped.
The observer's existing `w` preimage-read path was left unchanged. The wrapper
had no production integration or new memory-provider admission.

Both compilers matched the current `b0` reference in **960 controlled callback/
input cases and 5,952 per-access injected faults for each of `b1`, `w0`, `w1`**.
The earlier seven callback modes were retained, with an eighth that sets a
masked SSE exception-status flag inside a memory read. In addition to the full
output/failure-prefix comparison described above, every source read/write event
recorded hardware x87 status and MXCSR before the adapter operation. All four
timing checksums agree. No semantic counterexample appeared in this bounded
controlled component; this does not cover arbitrary exception implementations,
allocation failure while capturing an exception, all FP values/control modes,
an original Xbox fixture or the real recursive caller.

The current qualified/unqualified pair again compiled identically, as did the
wrapped qualified/unqualified pair. The wrapper changed code layout:

| Compiler | Current classification bytes | Wrapped classification bytes |
| --- | ---: | ---: |
| GCC | `0x2E05` hot + `0x95` cold | `0x28EF` hot + `0x5EC` cold |
| Clang | `0x19CA` | `0x1C85` |

These sizes exclude the wrapper/rethrow functions themselves. Smaller GCC hot
text therefore does not establish a cheaper full call or fewer private-state
spills; its cold text and wrapper work also changed. This result does not
establish the C++ read exception edge as the dominant cause of eager State
stores.

The finite timing run pinned each executable to one available CPU, rotated
variant order across seven rounds and performed 100,000 public-style calls per
variant per round. Other work still ran concurrently:

| Compiler | `b0` median ns | `b1` | `w0` | `w1` |
| --- | ---: | ---: | ---: | ---: |
| GCC | 3077.39 | 3488.80 | 3365.62 | 4005.49 |
| Clang | 3107.89 | 3036.20 | 3316.24 | 3243.57 |

The identical qualified pairs' timing spread demonstrates the noise level.
There is no measured wrapper gain; neither its extra calls nor exception-pointer
handling is justified by this experiment. A future real nonthrowing retained-read
admission API would require its own exact ownership, access/fault-prefix,
observer, exception and FP proof. It must not inherit approval from this
catch-and-rethrow control. No further experiments were run after this bounded
follow-up.

If revisited, first inspect the complete matching-build hot function and prove
that the remaining reload is specifically an alias limitation. Then compare
the same complete function with an exact, private-only annotation and repeat
the full caller/observer/fault checks. Keep canonical generated source unchanged
and use a receipt-bearing build-local transform only after a measurable benefit
and complete constructor/escape audit. Do not widen the promise to source memory,
public ABI references or callback-owned targets to force code generation changes.

## Evidence and storage

[`build/test-results/mcc-private-state-alias-20261022/evidence.json`](../build/test-results/mcc-private-state-alias-20261022/evidence.json)
retains source hashes, exact compile/link commands, comparison counts, raw timing
runs, normalized classification-code hashes, one listing per identical pair,
symbol sizes and the small qualifier-recognition control. It is about 350 KiB.
[`exception-evidence.json`](../build/test-results/mcc-private-state-alias-20261022/exception-evidence.json)
retains the final four-variant control, including its exact wrapper/driver/
classification source, commands, callback/fault results, timings and code hashes,
at about 343 KiB.
All scripts/compiler outputs/executables lived under the enclosing
`tools/run_test.py` scratch owner and were removed. No production file, original
input, installed runtime or active parent build was changed. Cleanup is recorded
in `storage-receipt.json`; no large artifacts are retained.
