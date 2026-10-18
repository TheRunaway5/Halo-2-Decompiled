# Original TU1.5 completion caller-state fixtures

Authority: English Xbox TU1.5 `default.xbe`, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The executable stays unchanged. `tools/oracle_campaign_completion_tu15.py`
executes the recovered completion code with explicit caller registers, clock,
game/options, player-pool and network banks. It stops at external owners before
they execute and supplies no external results. These banks are declared CPU
inputs; they are not a saved or authenticated live Xbox session.

Generate recovery evidence in a fresh directory using
`tools/generate_campaign_completion_tu15.py --output NEW_CPP --directory NEW_RECOVERY`.
Then generate this fixture directory using
`tools/oracle_campaign_completion_tu15.py --recovery NEW_RECOVERY --output tests/fixtures/campaign_completion_tu15`.
Run both Python tools through `tools/tag_match_memory_budget.py` with the
unchanged 60 MiB hard limit, independently measured 64 MiB ceiling and zero swap.
The optional CTest returns 77 when the locally generated fixtures are absent.

The 47 cases include null/inactive game, campaign profile and stack-allocation
boundaries, multiplayer first/repeated finish, five clock rates, missing-map
progress, player-option transfer with empty/sparse/dense/over-capacity rosters,
statistics once-only behavior, network state/epoch branches, queued options and
11 original path-search cases (empty/absent/repeated/first/last/null/high-byte).
Each runs with and without the native write observer. All retained bytes,
complete GP/XMM state, defined arithmetic/direction flags and observed store site/order/
before/written bytes must match. `--omit-options` is a failing control: skipping
the original player-option transfer must fail the retained-bank comparison.

The fixture covers 258 executed instruction addresses within 773 recovered
instructions. It does not validate paths after an external stop, profile saves,
the entire active-campaign sequence, loaded next-level gameplay, final audio,
GPU presentation, or an installed whole-current-source runtime.
