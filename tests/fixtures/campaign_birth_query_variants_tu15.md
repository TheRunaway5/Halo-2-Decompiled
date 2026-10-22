# Original campaign birth and update query evidence

All executions use unchanged English Xbox TU1.5 code, XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The native client never imports these references or runs the oracle.

| Required original reference | Bytes | SHA-256 |
| --- | ---: | --- |
| `campaign_birth_query_variants_tu15.json` | 10,470,383 | `91ddb58dabf78dbccfa922be289f94a50506350d5669c770fd5cc0e3aeb249f6` |
| `campaign_update_query_variants_tu15.json` | 5,438,362 | `973276541ad5f327a7b4b7576789b8bd6715b480e514becb521105ae32391904` |
| `campaign_remaining_query_variants_tu15.json` | 8,538,331 | `61c24423702677a183587c0a35865f3d930dacb39a5419d205a662724e069828` |

The actual authored Marine path requires `284300` during birth. Its four
variants are `285CF0`, `2861A0`, `286460` and `2868A0`. Once three Marines are
born, object update requires `2912A0` through existing selector `284390`.
That second complete family also includes `290B50`, `291740` and `291E30`.
These names describe the reached boundaries, not inferred original symbols.

`tools/generate_campaign_birth_query_variants_tu15.py` lowers all nine entries,
preserving 2,385 distinct original sites /2,583 per-entry sites. `286460`
contains shared `2868A0` code, so summing CFG lengths double-counts 198 sites.
The original `28619C` breakpoint remains an explicit failure; it is not skipped
or converted into a normal return. No case currently executes that breakpoint.
The existing renderer owns the writable sixteen-byte `.data` sign mask.
The existing scalar owner supplies readonly `46A0D0`. This module exports no
new mask, mutable query context, callback result or successful actor record.

The later complete-selector audit covers eleven existing interfaces and 65
targets. Its twelve remaining absent bodies add 3,716 sites. The same generator
now lowers 21 entries /6,101 distinct sites /6,299 per-entry sites, validates the
existing selectors against original instructions, and rejects duplicate owners.
All 65 static targets have bodies; actual runtime dispatch and behavioral
acceptance remain separate gates. No extra data is exported.

`tools/oracle_campaign_birth_query_variants_tu15.py` executes 1,308 whole
calls and 420 prefixes stopped before actual indirect callbacks. Explicit
component controls cover both selectors, negative/zero/positive counts,
byte/DWORD mask boundaries, signed indices and remapping, and zero/A5 priors.
No live actor state is captured or injected. The native comparison checks
GPR/SIMD, modeled flags except AF, exact instruction paths, all ordered writes,
complete query context/stack outputs and checkpoint restoration.

`tools/oracle_campaign_update_query_variants_tu15.py` reuses only those declared
input controls and executes all four original update variants independently.
It never reads the earlier expected outputs or supplies a dependency return.
Its 760 whole calls and 80 callback prefixes retain the same observations.
Write capture reads completed original bytes at instruction boundaries;
contiguous fragments of one 16-byte vector store retain one instruction's
complete before/after bytes. The new 840 cases and first 1,728 cases are added
to the existing 2,366-case production target.

Its `--additional-families` mode executes the twelve remaining target bodies
using only the same declared input controls: 1,100 whole calls and 160 prefixes.
That 1,260-case reference brings the combined production target to 6,194 cases.
Every unexecuted instruction remains listed in the original reports; the static
target inventory does not qualify those paths. The actual authored Marine run
now passes birth and reaches 34 ticks before requiring a different AI owner.

| Original entry | Observed sites | Complete CFG sites |
| --- | ---: | ---: |
| `284300` | 12 | 12 |
| `285CF0` | 219 | 309 |
| `2861A0` | 211 | 214 |
| `286460` | 382 | 477 |
| `2868A0` | 192 | 198 |
| `290B50` | 210 | 417 |
| `2912A0` | 200 | 295 |
| `291740` | 194 | 390 |
| `291E30` | 188 | 271 |

These figures deliberately retain the missing callback/SIMD/estimate paths.
Compiling the entire CFG does not qualify its unexecuted sites or physical
Xbox CPU estimate bits. Successful birth is separate from continued original
AI, navigation/combat, mission/checkpoint and cooperative-session behavior.

Use disposable storage for original regeneration and the production target:

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_birth_query_variants_tu15.py --fixture tests/fixtures/campaign_birth_query_variants_tu15.json --report analysis/campaign-ai-coop/birth-original.json --verify
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_update_query_variants_tu15.py --controls tests/fixtures/campaign_birth_query_variants_tu15.json --fixture tests/fixtures/campaign_update_query_variants_tu15.json --report analysis/campaign-ai-coop/update-original.json --verify
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_update_query_variants_tu15.py --controls tests/fixtures/campaign_birth_query_variants_tu15.json --fixture tests/fixtures/campaign_remaining_query_variants_tu15.json --report analysis/campaign-ai-coop/remaining-original.json --additional-families --verify
python3 tools/run_test.py -- python3 tools/test_campaign_ai_storage_cohort.py --object-decisions --compiler clang++ --output analysis/campaign-ai-coop/query-cohort
```

Source regeneration requires scratch `--output` and a bounded `--directory`.
Preserve the three original references (24,447,076 bytes total). All generated
sources, objects, clients, build trees and raw instruction/open-file traces
remain disposable; no capture or installed runtime is retained by this work.
