# Required native actor/order-script ownership

`native_campaign_ai_order_script_assignment_tu15.json` is a 150,126-byte
reference input captured from unchanged English Xbox TU1.5 executable SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It supplies 48 whole original `26C600` calls and 42 source read/store fault
prefixes. The original `140E00` erase, `140DB0` lookup, `140D50` insertion,
`267A30` hash and `267A40` equality bodies execute without code patches or
supplied dependency results. Physical i386 execution verifies every byte in
the 73,728-byte declared bank after each whole call and prefix; Unicorn
provides ordered read/write provenance. These are offline tools only.

This is the actor-plan/script-key association owner called first by the
original order-reset `1EC820`. The native C++ implementation preserves:

- Removal of the old key, collision-chain links and free-list publication.
- Full key equality, original cached-hash filtering and low-byte hash policy.
- Retirement of the previously associated actor and replacement of its value.
- Fresh allocation, free-list refusal, duplicate-key behavior when an existing
  record's actor value is NONE, and final actor-side key publication.
- Every original source access/store and the state left before a failed access.

The original constructor installs the hash/equality identities used by this
specific table and a four-byte actor-datum payload. Native code checks those
metadata identities and executes ordinary typed hash/equality algorithms; it
never calls an executable selector or falls back to lifted code. Foreign
callback profiles and payload layouts are explicit unsupported owners.

The normal cases execute 214 of the closure's 232 original instruction sites.
Generic variable-size payload byte-copy paths and impossible private-stack
guards do not establish broader table-layout support. No complete order,
combat, actor behavior, HS scheduler or live campaign parity is inferred.

Clang normal and ASan/UBSan checks use the production CMake module. A privately
compiled mutation removes the free-list head store and must fail the original
ordered-state test. The root GCC target is independently checked. All private
ELF images, original data-bank copies, compiler output and mutated sources are
removed by `tools/run_test.py`/`tools/run_tests.py`.

```sh
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-native-ai-orders -- \
  uv run --no-project --with capstone --with unicorn python \
  tools/oracle_native_campaign_ai_orders_tu15.py --verify \
  --fixture tests/fixtures/native_campaign_ai_order_script_assignment_tu15.json \
  --report analysis/campaign-native-kernels-20261028/ai-orders/script-assignment/original-repeat.json
python3 tools/run_test.py --scratch-root .local/test-scratch/campaign-native-ai-orders -- \
  python3 tools/test_native_campaign_ai_orders_tu15.py \
  --output analysis/campaign-native-kernels-20261028/ai-orders/script-assignment/clang
```

`ai_set_orders` remains unsupported until the actual actor combat/unit-action
reset, squad spatial transition, order-script scheduling and behavior-retire
callback dependencies are native. No host order assignment or synthetic
callback outcome is installed to conceal those remaining owners.
