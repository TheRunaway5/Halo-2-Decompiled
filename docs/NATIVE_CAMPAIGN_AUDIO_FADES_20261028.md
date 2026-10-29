# Native campaign playback fades

The closed playback fade capability is semantic native code in
`include/halo2/native_campaign_audio_fades.hpp`. It implements fade requests
(`129900`), retained fade sampling (`12D320`), and their complete numerical
dependencies (`12DB00`, `220000`, `220200`). The typed state service retains
actual sound-table/row and clock owners and reads fields lazily. The algorithm
contains no virtual CPU, register array, emulated x87 stack, address dispatcher,
or fallback to the existing register-level candidates. Real SSE/x87 operations
preserve the original numerical precision and rounding behavior.

This closes the fade capability required by playback start/retirement work.
It does **not** qualify the whole `12AB30`/`12ADE0` loops, cache allocation,
channel selection, spatialization, callbacks, or live campaign playback.

## Observable contracts

`129900` accepts source and target sound datums (either can be `NONE`), a signed
curve word and a float duration. It stores current sampled gain, curve and
timestamps, then activates each requested fade in original order. It retains
the initial table owner but resolves rows again at each original lookup. Source
and target may identify the same sound. Sampling re-resolves its own table.

`12D320` returns dB gain in EAX. Inactive sounds return zero without reading
the clock. A sentinel timestamp starts both ends by publishing each relative
timestamp around the original repeated clock reads. Signed timestamp ordering
and wrapped deltas determine fraction and endpoint direction. Curve zero mixes
amplitudes in extended precision, preserving the first endpoint's single
precision store. Curve one rounds both endpoints and applies the original
square-root curve and SSE mix. Unsupported curve words return zero.

`12DB00` returns its clamped fraction in XMM0 with zero upper lanes, and pops
12 argument bytes. Very small intervals use the original ordered step comparison
with the raw policy byte. The old register-level body and table entry have been
removed. The new thin ABI adapter only transports actual arguments and results.

The original `12D320` → `12DB00` caller and continuation execute inside each
complete physical reference call. `analysis/campaign-native-playback-20261028/caller-contracts.json`
also records the unchanged `12AB30` caller sites: `12AD0F` consumes sampled EAX
at `12AD14`; `12AD5E` supplies source EAX, target `NONE`, curve zero and 0.5
seconds, then consumes no volatile result. These callers require the verified
nonvolatile registers and stack cleanup; arbitrary dead scratch GPR/SIMD/stack
contents are outside the contract.

## Independent original comparison

The reference image is the user's owned TU1.5 XBE, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The offline harness executes its unchanged `.text` on the physical i386 CPU.
It supplies no original dependency return, patches no original instruction,
resumes no unknown owner, and uses no emulator. All required helpers execute
as original instructions within the measured call.

888 original calls execute 88,840 instructions, with 6,504 interleaved mutable
input reads and 1,760 retained stores. Native comparisons cover complete retained
banks, ordered read/write interleaving and before/after bytes, return values,
stack cleanup, nonvolatile registers and physical FP control/status/MXCSR.
Cases include active/inactive sounds, both curves and invalid words, sentinel
and wrapped timestamps, source/target identity, clock/timestamp aliasing,
negative/zero/infinite durations, denormals, distinct NaN payloads and signaling
NaNs, all four rounding modes, three x87 precision controls and DAZ/FTZ settings.
Unmasked FP traps and unsupported host FP backends remain outside this proof.

GCC and Clang with AddressSanitizer/UndefinedBehaviorSanitizer pass. Private
source mutations are rejected for reversing the degenerate interval step,
discarding the source gain endpoint, and moving the end-timestamp read across
the repeated clock read. The last mutation preserves data values in ordinary
cases but violates the measured input ordering. An initial origin-choice
mutation survived because it was algebraically equivalent on that corpus;
it is retained as failed test-design evidence, not counted as a negative control.

All 45 request, 34 fraction and 27 exponential instruction sites execute.
Sampling executes 118/130 sites and logarithmic conversion 36/44. The remaining
sites are defensive ordered clamps dominated by the closed numerical conversion:
positive amplitudes are bounded to at most one before the logarithm; nonpositive
and unordered inputs return -64 dB; results below -64 are rejected before single
precision rounding. Rounding cannot cross either exactly representable bound.
Thus the eight redundant logarithmic-clamp sites and twelve outer sampled-gain
clamp sites are unreachable under the proved masked-FP contract. The validator
checks exact expected coverage and original bytes, and reports these unexecuted
sites explicitly; it does not count them as executed evidence.

## Qualification and retained evidence

The native candidate table qualifies `129900` and `12D320` as two dependency
owners, and replaces the previously unqualified `12DB00` candidate. Four of the
forty required campaign addresses now have explicit semantic/original proof
(`129500`, `12DB00`, `225D70`, `228310`). Fifteen existing required candidates
remain unqualified and twenty-one required entries remain absent. The whole
campaign audio registry remains closed. Presence alone still fails its regression.

Reports are under `analysis/campaign-native-playback-20261028/`:

- `gcc-fades-qualified-v3/report.json`: final qualified-table physical comparison.
- `clang-fades-sanitizers-v1/report.json`: independent sanitizer/compiler pass.
- `semantic-gate-v2/report.json`: six actual typed candidates, including the two
  nonrequired dependency owners, plus presence-only rejection.
- `table-metadata-proof.json`: adapter algorithm prefix unchanged when adding
  qualified table records.
- `required-owner-graph.json`, `fade-closure-original.json`, `caller-contracts.json`:
  finite original closure and caller evidence.
- `cleanup.json`: final source hashes and scratch removal receipt.

The earlier three typed audio algorithms remain unchanged. The previous numeric
proofs remain applicable to those exact source hashes. All new ELF files, raw
banks, JSON vectors, mutation sources and build trees belong to the official
disposable wrappers and are removed. No large artifact is intentionally retained.
