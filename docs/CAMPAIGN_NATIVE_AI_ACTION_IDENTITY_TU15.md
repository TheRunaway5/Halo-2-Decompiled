# Native campaign action identities

The native data components now implement whole TU1.5 `26C510` action history,
`26C850` static/dynamic goal lookup, `277D40` dynamic goal lookup, and generic
`16E940` salted data-array lookup. The pinned original SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

History scans all four entries, preserves raw nonzero flag bytes, advances
the signed remainder ring, uses the computed index for its flag publication,
then freshly reads the retained index for its goal publication. Its NONE
value is the actual retained `47D6C0` scalar. Negative ring indices keep the
original wrapped addresses rather than being clamped. The first native
read-order regression caught and removed an extra read before the flag write;
the failed diagnostic remains in `identity-bringup-validation`.

Goal lookup keeps the original signed low-word bound and the distinct static
zone/dynamic owner masks. Dynamic membership uses the actual bitset, including
indices above 31; no extra owner bound or fixed capacity is supplied. Salted
lookup keeps the signed high-water check, lazy stride/row reads, nonzero salt
and exact full salt equality.

[Proof](../analysis/campaign-native-ai-actions-20261030/identity-proof.json)
records 354 physical unchanged-original cases: 293 completed calls, 56
rejected read/write prefixes and five genuine unmapped memory faults. All 124
original instruction sites are exercised. Native comparisons cover 1,570
ordered reads, 148 stores, four input timelines and 289 store-observer calls.
GCC, Clang, instrumentation of both actual source components with ASan/UBSan,
12 semantic mutations and an independent byte-identical original repeat pass.
Original live x87, SIMD, MMX and control/status preservation is checked.

The main project registers these typed retained-data APIs; this increment
qualifies their standalone component builds. There is no
numeric World binding, full action-planner acceptance, mission/co-op result,
or runtime promotion. The history function has a void semantic result; its
private transient GP state is not a return contract. No action outcome or
executable-backed gameplay dependency is supplied.

The required canonical input is
`tests/fixtures/native_campaign_ai_action_identity_tu15.json.gz` (723,841 bytes).
The initial original reference is preserved at
`analysis/campaign-native-ai-actions-20261030/identity-original-v1.json.gz`
(721,925 bytes). All completed owned source/build/original-execution scratch
directories were verified removed. No executable or raw bank is retained.
