# Original campaign player queries

`campaign_player_queries_tu15.json` is a local generated reference input and
follows the repository's ignored oracle-payload policy. It contains25 cooperative
query cases and161 complete player lists executed from the authenticated English
TU1.5 image, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/oracle_campaign_player_queries_tu15.py` authenticates both original HS
definitions, executes complete `2A9190` and its actual list/reference allocators,
and stops `2B3790` only at its real result-publication boundary. It supplies no
callee return and patches no instructions. Raw pool/options inputs are declared
component fixtures; they do not establish a real cooperative session.

Regenerate only when the fixture is absent; preserve prior original evidence.
Use the existing local TU1.5 reference selected by the oracle's authenticated
source module. Do not use it as a gameplay/runtime dependency.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_player_queries_tu15.py --output '@HALO2_TEST_SCRATCH@/original' --fixture tests/fixtures/campaign_player_queries_tu15.json
```

Repeat independently with `--verify` to compare every fixture byte. The initial
fixture SHA256 is
`c3b9c88bd0cda3be956622f71c9f2b3520adbc85777a8b9d10567c6c92fd2ea7`.
Tool hashes are part of the fixture; an edited oracle requires a reviewed new
reference rather than silently overwriting the old one.

```sh
python3 tools/run_tests.py --target campaign_player_queries_tu15_tests --filter '^original_campaign_player_queries_tu15$'
python3 tools/run_test.py -- python3 tools/test_campaign_player_queries_tu15.py --output analysis/campaign-ai-coop/query-validation
python3 tools/run_test.py -- python3 tools/test_campaign_player_queries_tu15.py --sanitizers --output analysis/campaign-ai-coop/query-sanitizers
```

The small cohort helper uses the production CMake module, original fixture and
current source snapshot. Normal validation additionally compiles the actual
Simulation caller. It preserves source hashes and bounded logs while the outer
storage wrapper removes all copied headers, objects, libraries and test programs.
