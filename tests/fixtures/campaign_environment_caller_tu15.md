# Local OG Xbox TU1.5 campaign environment fixture

The JSON fixture is intentionally ignored. Generate it locally from owned inputs; do not force-track original byte payloads. The trackable generator executes unchanged original caller instructions and never reads candidate C++ output.

Required inputs and identities:

| Input | SHA-256 | Captured identity |
| --- | --- | --- |
| English OG Xbox TU1.5 `default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` | Original code authority |
| `00a_introduction.map` | `5093e2f8ef7f0df41c11c6621bac5785a36f78a1cff3b8b2bc6c14b561c02a28` | Original introduction cache |
| Fleet environment bank | `e80fc2d489e0fcc5794376b58457d060712035af03f16c4af4bbc7d4c258c20d` | 288 bytes at `4C6710`, game tick 1558 / frame 32800 |
| Destruction environment bank | `fbef1d9f6671be6ab102051b4797f3816ef42893b5e253acb4d7c11dcd2c7091` | 288 bytes at `4C6710`, game tick 2800 / frame 34081 |

Both banks came from completed actual original main calls under xemu 0.8.136 / Complex 4627 with the pinned TU1.5 executable. They are source-data captures, not physical Xbox or final framebuffer evidence. The generator authenticates them as inputs and copies their bytes; it cannot recreate their capture identity by running the candidate implementation.

From the repository root, with the owned executable and retained captures available:

```sh
./.tools/python/bin/python tests/fixtures/generate_campaign_environment_caller_tu15.py \
  --xbe /path/to/owned/TU1.5/default.xbe \
  --map assets/disc/maps/00a_introduction.map \
  --fleet-bank analysis/campaign/character-shaders-20261013/campaign-original-session-v1/fleet-main-timeline-v1/environment-extension-v1/published-environment-0x4c6710.bin \
  --destruction-bank analysis/campaign/character-shaders-20261013/campaign-original-session-v1/tick2800-main-timeline-v2/published-environment.bin \
  --output tests/fixtures/campaign_environment_caller_tu15.json
```

The Python environment needs Unicorn and the repository's original-image tooling dependencies. The generator refuses changed pins and an existing output. Its 252 deterministic caller controls run actual `2B868..2B8AB` and `2BA96`: RGB flag 0/1/255, enabled byte 0/1/2/255, strict near/far endpoints, and unordered produced clamp limits. All 288 published bytes and the camera far word are retained. Synthetic NaN controls establish caller output behavior; they do not qualify floating-point exception flags.

`original_campaign_environment_caller_tu15` skips with 77 when this local fixture is absent. Native cache tests also skip when required assets or the configured `HALO2_TU15_GAMEPLAY_XBE` are absent. A present corrupt fixture is an error. Separate native tests cover the real resolver/producer, full publication, six required writable-owner schemas, journal replay, active mixer and zero/positive accepted-frame scheduling. These controls do not prove live Vulkan cutscene or final visual parity.
