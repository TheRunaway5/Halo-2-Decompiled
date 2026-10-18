# Local original campaign location fixture

Generate this ignored JSON from the owned English OG Xbox TU1.5 executable and the retained ordinary-native introduction input bank. The generator executes the unchanged original main prefix `2B770..2B816`, complete `2B700`, and complete `14CDB0`; it does not import the candidate implementation or supply query results.

Required input SHA-256 values:

| Input | SHA-256 |
| --- | --- |
| English OG Xbox TU1.5 `default.xbe` | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Actual ordinary-native introduction pre-call bank | `14457cb4fb18292e5f0c2fd007e9a4f36590ae97d210f3212c1cba32e9d4cb49` |

The bank is retained at `analysis/opening-cinematic-20261015/campaign-particle-consumer-v1/environment-closure-v1/native-control-v2/case/fleet/before.bin`. It is the actual native factory/cache input before the environment producer. It supplies the real authored introduction BSP and source storage, and does not contain location results from the function under comparison. The executable's text must remain byte-for-byte intact after overlaying the bank. This is a bounded original source replay, not an original gameplay checkpoint or framebuffer capture.

```sh
./.tools/python/bin/python tests/fixtures/generate_campaign_environment_location_tu15.py \
  --xbe /path/to/owned/TU1.5/default.xbe \
  --native-input analysis/opening-cinematic-20261015/campaign-particle-consumer-v1/environment-closure-v1/native-control-v2/case/fleet/before.bin \
  --output tests/fixtures/campaign_environment_location_tu15.json
```

The Python environment needs Unicorn and the repository's original-image tooling. The generator refuses existing output or changed input pins. It retains the two actual main camera positions, a real failed point query, valid and mismatched-but-in-range previous pairs, and negative/end-bound invalid pairs. Failed queries never repair the persistent global history. Caller positions and previous location values are explicit fixture inputs; no leaf, cluster, return value or original code is patched.

The native location/environment test additionally requires the local environment fixture documented in `campaign_environment_caller_tu15.md` and owned cache/executable assets. Missing required inputs return 77. The test checks native source query/producer order, complete environment publication, genuine original environment banks and full memory journal replay. Vulkan, full live script chronology and final visual parity remain separate validation gates.
