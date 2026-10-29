# Native campaign clock and ducking

The complete TU1.5 `129500` audio clock/ducking update now has a semantic
C++ algorithm in
[`native_campaign_audio_ducking.hpp`](../include/halo2/native_campaign_audio_ducking.hpp).
It closes both dependencies: `33B590` reads the actual imported tick-count
binding and cell; `12E9A0` reads eight activity gates and two signed counters in
their original order. No virtual register file, instruction dispatcher, lifted
fallback or executable access implements these algorithms.

The typed storage interface preserves lazy reads and ordered writes. This
includes signed conversion of wrapped millisecond differences, accumulated
clock/tick state, the original delay initialization and countdown, paused
attenuation, activity-dependent recovery, non-campaign/default gain, exact
single-precision/NaN operand order and the admitted FP environment. The primary
activity flag does not short-circuit the subsequent original activity reads.
The game owner is resolved before accumulated-time publication and retained
while later fields are read.

The thin
[`adapter`](../src/audio/native_campaign_audio_ducking_adapter.hpp)
transports retained storage and the original calling convention. It preserves
the callee-saved GPRs, SIMD4–7, return stack and FP state. It does not fabricate
arbitrary scratch registers or dead-stack bytes.

## Physical original proof

The owned English TU1.5 image is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The
[`validator`](../tools/validate_native_campaign_ducking_tu15.py)
executes unchanged original i386 instructions on the host CPU under the
[`offline observer`](../tools/observe_native_campaign_ducking_reference_tu15.cpp).
All three complete original owners execute to their actual return. No code
patches, supplied results or resumed unknown dependencies are used. Immutable
literal reads are replaced only by the pinned original values in native code;
mutable input reads retain their actual order and observed bytes.

The 1,024-case corpus covers all 16 SSE rounding/DAZ/FTZ contexts, clock wrap
and negative deltas, active/absent/non-campaign games, pause delay and recovery,
every activity gate/counter, clipping branches, signed zeros, denormals,
infinities and signaling/quiet/distinct-payload NaNs. Native output matches
20,960 ordered mutable input reads, 6,448 ordered persistent stores, complete
retained banks and exact FP control/status/MXCSR over 75,344 original
instructions. Every instruction site is reached: 108 in `129500`, 34 in
`12E9A0` and three in `33B590`.
Host FP state is restored after native calls.

Each mutable input also records the number of persistent stores preceding it,
so its interleaving with publications is checked. Three isolated source
mutations are rejected: zeroing the signed/wrapped clock delta, skipping the
activity query and moving game-owner lookup before tick publication. The last
mutation preserves separate read/write sequences and their values; its changed
interleaving still fails the original comparison.

The final GCC result is
[`gcc-interleaved-v4/report.json`](../analysis/campaign-native-ducking-20261028/gcc-interleaved-v4/report.json).
The independent Clang ASan/UBSan build passes the same native and mutation
checks in
[`clang-interleaved-sanitizers-v2/report.json`](../analysis/campaign-native-ducking-20261028/clang-interleaved-sanitizers-v2/report.json).
The original ELF, raw vectors, mutated headers and private builds belong to the
official `run_test.py`/`run_tests.py` cleanup owners. Only bounded reports and
logs are retained.

## Remaining boundary

`129500` is appended to the campaign candidate map as a separate ducking group.
The complete campaign audio registration gate remains closed: 19 of 40
candidates have implementations and 21 remain absent. The earlier candidates
retain their own proof/semantic-conversion scope; their presence is not a new
complete campaign qualification. No launcher selection or live playback
acceptance is implied. Unmasked FP traps and arbitrary scratch/dead-stack
contents remain outside the admitted native contract.

Candidate presence cannot open the group. `CampaignRoutineEntry` defaults to
`semantic_original_proven=false`; only the three typed/proved leaves explicitly
set it true. The shared completion predicate requires every exact required
address, one unique non-null callable and semantic original proof. All 16 older
instruction-level candidates remain unqualified even if the 21 absent entries
are later added. The
[`gate regression and mutation`](../analysis/campaign-native-ducking-20261028/semantic-gate-v1/report.json)
checks the three actual typed candidate records, forty present but unqualified
owners, every single unqualified/null/missing/wrong owner, duplicate owners and
invalid inventories. Removing semantic eligibility from the real shared
predicate fails the regression. The actual registry translation unit compiles
in the same validation.

The
[`metadata receipt`](../analysis/campaign-native-ducking-20261028/gate-metadata-proof.json)
records the table-only additions to channels and the ducking adapter: their
complete algorithm/adapter prefixes are byte-identical, and removing only the
three new `true` metadata fields reproduces their exact pre-change source
hashes. Earlier original and sanitizer proofs retain their bounded source
identities; subsequent component checks build the new metadata layout.

The standalone check is:

```sh
python3 tools/run_test.py -- python3 tools/validate_native_campaign_ducking_tu15.py \
  --output analysis/campaign-native-ducking-20261028/recheck
```
