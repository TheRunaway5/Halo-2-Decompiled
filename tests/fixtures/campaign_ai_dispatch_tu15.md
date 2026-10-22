# Original AI bounded switch data

`campaign_ai_dispatch_tu15.json` is a required independent original reference
input: 9,044 bytes, SHA-256
`dafc967f06069902aac5ff14a3a2d8352fcf722f453f283c36e26d0d435e2a39`.
The authority is TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/export_native_tu15_ai_dispatch_data.py` audits 686 original function
bodies against the pinned compiled-frame snapshot. It exports 49 guarded
immutable byte/DWORD tables, 1,836 bytes; each DWORD branch stays in its own
original function. It rejects unknown instruction sites and excludes no SDK
entries. Writable state, tags, time imports and computed results are absent.
Ten independent original signed-selector executions cover the reached
`1FBCC0` guard and six-lane `1FC0E8` table. These are guard prefixes only.

The production directory test checks every byte and readonly permission,
retains a compatible existing lane/owner, rejects conflicting or writable
aliases and keeps the byte after the reached table unmapped. The same source
cohort reruns all 17 original property/directory cases. Run through the owned
test workflow:

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-ai-coop -- python3 tools/test_campaign_ai_storage_cohort.py --child-directory --output analysis/campaign-ai-coop-20261020/ai-dispatch/current-cohort-repeat
```

Only the original reference JSON and small reports persist. This does not
certify whole behavior callback, navigation/combat, mission or co-op parity.
