# Original descriptor27 behavior callback

`campaign_ai_behavior_27_tu15.json` is a required original TU1.5 execution
reference:372 complete calls,1,510,016 bytes, SHA-256
`b8f1e1c3ed08b8c5591c8ac6cb04a5113b66ebe8afdd3ec600b91c219d74b5db`.
Original executable SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The actual Marine encounter reaches1B0195 at tick288. Its indirect call reads
descriptor48ABE0+0x10, which is original callback1BD1E0. The descriptor is
kind1 at directory index27. The engine adds whole1BD1E0 and1ED100 (148 original
sites), preserving all91 prior actor bodies and all212 immutable data bytes.
The actor module now contains93 bodies /7,112 original instruction sites.

142 callback,120 character-data and110 standalone iterator cases cover the
actor guards, absent property/query/character data, query status/kind, variant
selection and inherited character blocks, active/inactive script settings,
matching/mismatched/missing linked property records, deadlines below/equal/
above the current tick, and iterator exhaustion. Two caller poison patterns
retain unwritten bytes. All92 callback sites, all56 character-data sites,
all11 property-lookup sites and all50 iterator sites execute. The existing
27C710 script-style lookup executes34 of50 sites; its squad fallback remains
unqualified by these cases.

The original five owners run without code patches or supplied return values.
Inputs are declared caller actor/property/query/script/tag/frame records.
Original instructions produce every callback return and iterator write.
Native tests reuse complete production Frame lookup owners and an exact,
test-only extraction of the existing Scene iterator278DD0. No iterator body,
record ordering or result is rewritten. Comparison checks all general/SIMD
registers, modeled flags, exact instruction order, every ordered store and its
prior bytes, preserved inputs and checkpoint restoration.

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_behavior_27_tu15.py \
  --verify --fixture tests/fixtures/campaign_ai_behavior_27_tu15.json \
  --report analysis/campaign-ai-coop-20261020/behavior-27/original-repeat.json
```

Run `tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py` for normal and ASan/UBSan component checks. Original,
generation/preservation and composed-client reports are tracked under
`analysis/campaign-ai-coop-20261020/behavior-27/` and the campaign checklist.
Complete mission/navigation/combat/co-op and live callback/checkpoint acceptance
are separate gates. Retain this required reference and bounded reports;
generated binaries, source snapshots and raw banks use disposable storage.
