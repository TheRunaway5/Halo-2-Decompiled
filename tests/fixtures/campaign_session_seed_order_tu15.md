# Original campaign session seed and new-map initialization

`campaign_session_seed_order_tu15.json` records unchanged English Xbox TU1.5
execution from independent admitted option seeds `0x12345678` and `0x87654321`.
Original executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Whole `13A8B0` admits the session options and copies the seed to the gameplay
RNG. Registered new-map rows 24, 25 and 42 then execute `11E550` (including
whole `219F90`), `1F38D0`, and `219300`. Other lifecycle rows are not claimed
executed by this component reference. All instructions in these four
initializers execute; 25 of 31 instructions in admission execute through its
campaign branch. No instruction patches or service returns are supplied.

Each case retains all 3,340 resulting noise bytes and eight derived scalar
bytes. Original noise generation consumes 24 gameplay RNG draws. Replaying
the actual admission routine afterward resets the RNG to the original seed,
demonstrating the incorrect constructor-before-admission order even when the
initial seeds happen to match. Full hardware floating-point status is not
qualified by this emulator reference.

Production keeps the original initializers, direction table and scalar
algorithms. Their calls now occur in session configuration after admitted
seed and clock/engine/world setup, before cinematic/HS/AI initialization.
The full-world `campaign_session_seed_order_tu15_tests` constructs an actual
MCC campaign runtime for each seed and compares the noise/scalar outputs.
It never writes reference outputs into native memory. These native checks
pass for both seeds in the complete immutable current-source v3 client. Its
Marine encounter advances125 ticks before missing1B7D20. Original activation
and the other three300-tick actor controls pass; the batch remains failed
because Marine is incomplete. Its immutable snapshot is unchanged and its
owned source/builds are removed. Later checkout drift is recorded separately. These results do not establish full mission,
navigation/combat, checkpoint replay or cooperative acceptance.

Repeat the original reference without replacing it:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_session_seed_order_tu15.py \
  --verify --fixture tests/fixtures/campaign_session_seed_order_tu15.json \
  --report analysis/campaign-ai-coop-20261020/contrail-init/seed-reference-repeat-v1.json
```

The native target registers two tests when
`HALO2_CAMPAIGN_SEED_ORDER_MCC_MAP` names the prepared MCC campaign map.
Use `tools/run_tests.py` and filter `^native_campaign_session_seed_order_`.
The full cohort helper also builds the actual native client, runs original
activation, and executes the four existing MCC actor probes.

Required original reference: 14,557 bytes, SHA-256
`fca9bc98757fe59e177e27eed53764c4d25c0931e1552f70b199e5db50600e48`.
No binary, memory bank or render capture is retained by this reference.
[Original lifecycle order](../../analysis/campaign-ai-coop-20261020/contrail-init/session-seed-order-v1.json),
[repeated original execution](../../analysis/campaign-ai-coop-20261020/contrail-init/seed-reference-repeat-v1.json),
[native qualification](../../analysis/campaign-ai-coop-20261020/current-source-v3/current-source-cohort.json).
