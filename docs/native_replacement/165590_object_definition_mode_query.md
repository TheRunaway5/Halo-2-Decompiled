# CombatAction object definition mode query 165590

The unchanged TU1.5 routine is childless. It checks the live word at
`[game_state+0x180]` against 9, rejects the full `0xffffffff` datum, then
reads the actual definition through the object and tag tables. It sets AL
only when the definition word at `+0x290` equals 1, preserving EAX's
upper bytes. No mode or field name is inferred. The nearby 1655F0 routine
has a genuine 24B430 child and stays separate.

- [x] Match all 23 original sites under both sanitizer compilers: 96
  returns, 248 provider stops, 96 retired-store stops, 4,680 complete
  pre-I/O packets and three compiled defects each. Owners removed.
  `native-165590-definition-mode-physical-both-20261110/report.json`.
- [x] Match actual current and freshly generated CombatAction in both
  raw and prepared forms. Both sanitizer compilers pass 2,688 returns,
  4,884 stops and 128,064 packets, including live flags, provider and
  observer feedback, with four defects each. Owners removed.
  `native-165590-definition-mode-source-both-20261110/report.json`.
- [x] Authenticate four whole CombatAction contexts, with exact raw and
  prepared inverse and ten strict refusals per context. Owner removed.
  `native-165590-definition-mode-binding-private-20261110/report.json`.
- [x] Install the exact CombatAction default with its separate
  `HALO2_REFERENCE_OBJECT_DEFINITION_MODE_QUERY` switch, generator,
  late retag and source catalog hooks. All 452 other complete bodies,
  including Projectile 100E70, reverse exactly.
  `native-165590-definition-mode-insertion-20261110/report.json`.
- [x] Pass the frozen installed source and frame suite: seven registered
  tests, four focused operand/catalog checks, and linked driver syntax.
  The `2875f86b…` owner was removed.
  `native-165590-definition-mode-installed-20261110/report.json`.
- [x] Pass the actual linked public API for 165590 and Projectile. The
  replay rebuilt the exact 165590 CombatAction and public-driver objects,
  copied R31's linked inputs independently and verified that the old owner
  object was not extracted. For 165590 the three full streams match 384
  returns, 588 stops and 14,688 events each. Default and unrelated control
  each enter the native bridge 972 times; own-reference enters zero times.
  Owner removed. `native-165590-definition-mode-linked-r31-overlay-20261110/report.json`.
- [ ] Qualify inherited calls and a coherent runtime with this default.
  The production ownership predicate already covers 165590; the insertion
  added no inherited dispatch. R31 itself predates this default.

Other images, actual inherited routes and whole runtime remain open.
