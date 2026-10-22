# Original AI scalar startup callbacks

`campaign_ai_scalar_defaults_tu15.json` is a required independent original
reference input: 43,743 bytes, SHA-256
`6000301bb32b407949f1b567b9eb6a5f32bc6c32d349dcb87080707b659d871b`.
The authority is original TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`tools/oracle_campaign_ai_scalar_defaults_tu15.py` executes whole `3826A0`,
`3826C0` and `3826F0`, all 11 instructions. The original `_cinit` table at
`46DD00` supplies their order before `29BE40` consumes their geometry inputs.
`3826D0` is an interior instruction, not a separate callback. Four authored
readonly operands are copied to loader-zeroed mutable `56E080..56E08F`.
The production data profile never supplies completed scalar outputs.

36 cases combine original operands, negative zero, signaling/quiet NaN
payloads, positive/negative infinity and zero/A5 previous destinations.
Native comparisons include complete GPR/SIMD/flags, every instruction/store
and prior bytes, with a real RET stack word and no supplied dependency return.
Separate controls verify authored mounted producers, checkpoint restoration,
adjacent holes and missing-source/destination/stack failure boundaries.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-ai-coop -- python3 tools/test_campaign_ai_storage_cohort.py --scalar-defaults --output analysis/campaign-ai-coop-20261020/ai-scalar-defaults/current-cohort-repeat
```

Normal and ASan/UBSan checks use the same immutable input closure. All copied
sources, builds, clients and objects are disposable. Only the required original
reference and small reports persist; this is component evidence.
