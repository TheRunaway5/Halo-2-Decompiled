# Local original campaign fog extern fixture

`generate_campaign_fog_externs_tu15.py` executes the complete unchanged English OG Xbox TU1.5 scalar provider at actual `336E0` (dispatch table `338C4`) and RGB provider at actual `33970` (dispatch table `33F7C`). Canonical retail names `336F0`/`33980` identify corresponding owners only; all executed instructions and expected values come from the TU1.5 executable.

Required owned input SHA-256 values:

| Input | SHA-256 |
| --- | --- |
| English OG Xbox TU1.5 `default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Fleet completed environment, tick1558/frame32800 | `e80fc2d489e0fcc5794376b58457d060712035af03f16c4af4bbc7d4c258c20d` |
| Destruction completed environment, tick2800/frame34081 | `fbef1d9f6671be6ab102051b4797f3816ef42893b5e253acb4d7c11dcd2c7091` |

The two complete 288-byte captures were collected with xemu 0.8.136/Complex4627. They retain their genuine capture identity; they are not outputs manufactured from the candidate. Their retained paths are shown below. The other 85 banks are explicit consumer inputs: 81 combinations of primary density, interpolation, transition and eye fraction, followed by signed-zero, unclamped-high, unclamped-negative and tiny-normal controls. They do not claim to be producer outputs.

```sh
./.tools/python/bin/python tests/fixtures/generate_campaign_fog_externs_tu15.py \
  --xbe /path/to/owned/TU1.5/default.xbe \
  --fleet-bank analysis/campaign/character-shaders-20261013/campaign-original-session-v1/fleet-main-timeline-v1/environment-extension-v1/published-environment-0x4c6710.bin \
  --destruction-bank analysis/campaign/character-shaders-20261013/campaign-original-session-v1/tick2800-main-timeline-v2/published-environment.bin \
  --output tests/fixtures/campaign_fog_externs_tu15.json
```

The generator requires Unicorn and the repository XBE reader, authenticates every owned input, refuses existing output, checks every executed instruction against the executable, and verifies all text remains unchanged. It copies each full bank into the original render consumer owner `491F6C`; actual `1340A..13414` copies the 288-byte published environment there. No service result or original instruction is patched. RGB writes must touch exactly three floats and preserve surrounding canaries; the native fourth vector component is padding and is outside this original comparison.

Every bank covers scalar indices `3,24..36` and RGB indices `25..32`, yielding 3,306 exact word observations. The native regression passes the same complete bank through `source_campaign_environment_sample_tu15` and `bsp_fog_externs`, covering the typed adapter offsets as well as scalar/RGB arithmetic. In particular, original scalar29 returns raw interpolation (`492088`), while scalar31 multiplies interpolation by primary density (`491F94`).

The JSON contains original bytes and is intentionally ignored. Keep this generator and documentation tracked instead. `campaign_fog_externs_tu15_tests` returns77 for an absent local fixture. This is CPU data-consumer evidence. It does not verify full `25A10` constant submission, `25E50` composition, secondary patchy color consumers, original GPU programs, final framebuffer appearance, physical Xbox execution or matched visual parity.
