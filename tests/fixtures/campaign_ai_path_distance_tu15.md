# Original actor path-distance owner

The required `campaign_ai_path_distance_tu15.json` reference contains100
original TU1.5 executions:96 complete200C00 calls and four prefixes stopping
before the actual218780 coordinate-transform call. Its size is296,037 bytes,
SHA-256 `0e8ba40d6cbc617ee88bb06a3ffac50358a4c06eddbf97cb63f2dfeda8d2c159`.
Original executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Whole cases cover active/state guards, empty/reversed/equal index ranges,
one and multiple segments, repeated points, finite integral/fractional/small/
large coordinates and two caller poison patterns. The actor and path records
are declared caller inputs. Original x87 instructions produce every distance;
no expected distance, completed path or transform return is injected.

Native comparison checks all general/SIMD registers, modeled flags, x87 depth,
instruction order, every ordered store with prior bytes, preserved actor/path
fields and checkpoint restoration.81 of84 sites execute; the three unobserved
sites are the transform call and its return guard. Completion of transformed
paths, x87 exception-status/nonfinite behavior and complete navigation remain
unqualified by this reference. The production owner keeps the real218780
dependency; it supplies no fallback transform.

The generator adds only200C00 to the actor behavior module:91 bodies /6,964
original instruction sites. All90 earlier bodies and all212 immutable data
bytes remain exact. Generation and original execution repeat independently.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_path_distance_tu15.py \
  --verify --fixture tests/fixtures/campaign_ai_path_distance_tu15.json \
  --report analysis/campaign-ai-coop-20261020/path-distance/original-repeat.json
```

Use `tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py` for normal and ASan/UBSan production component checks.
Reports, generation preservation hashes and live integration status are under
`analysis/campaign-ai-coop-20261020/path-distance/` and the campaign checklist.
Only this required original reference and bounded reports are retained.
