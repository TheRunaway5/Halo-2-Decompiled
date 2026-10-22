# Original child-command link construction

`campaign_ai_child_links_tu15.json` is a required original Xbox TU1.5 execution
reference:722 complete calls to1AFDE0,2,596,841 bytes, SHA-256
`64c3a3b372cdd86120705d88dc5eebda3cf53817bf1e0731156dd1302ede6387`.
The authenticated executable SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The declared caller supplies child kinds, command opcodes/targets, count bytes
and active masks. Original instructions clear each child head and construct
the output lists. No resulting links, successful dependency returns, code
patches or command results are supplied. Cases cover all five opcodes and
ordered triples, shared/distinct/missing target kinds, active/inactive masks,
empty lists, mask-word boundaries and two caller/storage poison patterns.

The whole production Frame owner is unchanged. Native comparison checks all
general/SIMD registers, modeled flags, instruction order, every ordered store
including prior bytes, resulting lists, preserved caller fields and checkpoint
restoration.137 of142 static instruction sites execute; the five unobserved
sites are explicitly listed in the reports, without qualifying those paths.

The production data include mounts only120 authored row+0xC DWORDs (480
bytes).105 rows come from authored behavior children and15 from embedded
property commands. The original heads are zero. Authored headers/timers stay
immutable; unread row padding/tails gain no ownership. Publication preserves
existing mutable backing/values and preflights all heads before adding missing
ones. Native checks also exercise a conflicting read-only owner, partial
existing backing, native address preservation and checkpoint restoration.

Regenerate through disposable storage (output include remains in scratch):

```sh
PYTHONDONTWRITEBYTECODE=1 python3 tools/run_test.py \
  --scratch-root .local/test-scratch/campaign-ai-coop -- \
  .tools/python/bin/python tools/oracle_campaign_ai_child_links_tu15.py \
  --verify --fixture tests/fixtures/campaign_ai_child_links_tu15.json \
  --data-output '@HALO2_TEST_SCRATCH@/child-links.inc' \
  --report analysis/campaign-ai-coop-20261020/behavior-links/original-repeat.json
```

Run the production component target, normally and with ASan/UBSan, using
`tools/test_campaign_ai_storage_cohort.py --object-decisions` through
`tools/run_test.py`. Original reports are in
`analysis/campaign-ai-coop-20261020/behavior-links/`. This reference proves
bounded component behavior; complete mission/navigation/combat/co-op acceptance
requires the separate current-client and live gameplay gates.
