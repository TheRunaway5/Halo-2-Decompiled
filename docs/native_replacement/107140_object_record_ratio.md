# Primary object record ratio 107140

The unchanged TU1.5 routine has 36 sites and no child calls. It looks up
the object and its definition, sign-extends the caller's index, and uses
the definition's signed upper bound at `+0x2c8`. The original has no lower
bound. Record kind 2 returns the live constant; kind 1 subtracts a scaled
ratio made from the signed record word, definition row and live scale.
Both exits restore ESI and use `RET 4`. The authentic 1046C0 caller enters
at `104815` and `104B6C`; both local edges are pinned in current/fresh raw
and prepared source.

- [x] Match unchanged i386 under GCC and Clang sanitizers: 192 returns,
  399 stopped provider prefixes, 192 retired PUSH stops and 15,318
  pre-I/O packets per compiler. Four compiled defects each. Owner removed.
  `native-107140-object-record-ratio-physical-both-20261110/report.json`.
- [x] Match the existing modeled source in both formats and both owners:
  5,376 returns, 14,428 stops and 473,640 packets per compiler. The exact
  original private binding preserves 2,025 neighboring bodies in each
  context. Owners removed. `native-107140-object-record-ratio-source-both-r3-20261110/report.json`,
  `native-107140-object-record-ratio-binding-private-20261110/report.json`.
- [x] Identify both modeled shift differences using unchanged i386. At
  the first post-shift provider read, original EFLAGS is `0x212` versus
  modeled `0x202`. At a shift-six return, it is `0x297` versus `0x287`.
  All other fields match in these witnesses. Both modeled-shift controls
  are rejected under both compilers. `native-107140-object-record-ratio-shift4-witness-20261110/report.json`.
- [x] Close the ordered native and retained source proof under both
  sanitizers and six defects each. Both modeled shifts are rejected. The
  ordered proposal is scoped to TU1.5 x86 and preserves the MapPack shift
  policy. All four whole contexts pass both compilers and 12 structural
  refusals each. Owners removed.
  `native-107140-object-record-ratio-ordered-source-r2-20261110/report.json`,
  `native-107140-object-record-ratio-ordered-binding-private-r2-20261110/report.json`.
- [x] Install the exact current 107140 default. Whole raw/prepared inverse
  preserves all 2,025 other bodies and previous native receipts. Genuine
  fresh Primary generation and specialization include this owner.
  `native-107140-object-record-ratio-insertion-20261110/report.json`.
- [x] Pass all three frozen installed sanitizer checks and the exact
  raw/prepared catalog. Owner removed. The earlier configure failure is
  preserved. `native-107140-object-record-ratio-installed-r2-20261110/report.json`.
- [x] Trace both real 1046C0 paths on unchanged original: 96 complete
  returns, 48 calls and returns per call site, and 4,536 packets. Owner
  removed. `native-1046c0-107140-original-routes-20261110/report.json`.
- [x] Pass actual linked direct and inherited calls in three modes: 192
  returns, 20 provider stops and 22,316 events each, including 48 returns
  at each 1046C0 call site. The one-shard R31 relink excludes the old
  archive member and proves no coherent accounting claim. Owner removed.
  `native-107140-object-record-ratio-linked-r31-overlay-r2-20261110/report.json`.
- [ ] Pass a coherent runtime with the 107140 default and its registered
  linked test.
