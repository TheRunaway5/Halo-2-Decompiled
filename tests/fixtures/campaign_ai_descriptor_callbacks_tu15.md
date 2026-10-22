# Original kind2 callback selector evidence

`campaign_ai_descriptor_callbacks_tu15.json` is a required independent original
reference: 329,798 bytes, SHA-256
`47b0743a40f1b52516a9d6b4f34871d929ce406628aeacd02a566838538e2870`.
Authority is TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

The original `1B0EC0`/`1B0F10` selectors first check descriptor kind2, then
consume actual callback identities at offsets44/48. Their table has 115
nonnull descriptors; 55 have kind2 and need these additional 440 bytes.
The production owner publishes only those fields, preserving the following
record or alignment. Some records follow immediately at+4C, so demanding that
all adjacent bytes be globally unmapped would incorrectly reject those owners.

`tools/oracle_campaign_ai_descriptor_callbacks_tu15.py` produces 230 cases:
155 selectors complete; 75 stop before executing their nonnull indirect CALL.
42 non-call instructions execute, and two call sites are observed at prefix
boundaries. No callback behavior/return is supplied. Compare GPR/SIMD, modeled
flags exceptAF, every instruction and ordered stack store/prior byte. Native
controls also verify exact readonly extents and compatible/conflicting aliases.

```sh
python3 tools/run_test.py -- .tools/python/bin/python tools/oracle_campaign_ai_descriptor_callbacks_tu15.py --fixture tests/fixtures/campaign_ai_descriptor_callbacks_tu15.json --verify --report analysis/campaign-ai-coop/callback-original.json
python3 tools/run_test.py -- python3 tools/test_campaign_ai_storage_cohort.py --descriptor-callbacks --compiler clang++ --output analysis/campaign-ai-coop/callback-tracking-cohort
```

The latter also checks the object-tracking references described in
[`campaign_ai_object_tracking_tu15.md`](campaign_ai_object_tracking_tu15.md).
Normal and sanitizer builds use one immutable source/include/fixture closure.
All copied sources/builds/objects/clients are disposable; these required
original fixtures and bounded reports persist.
