# Flight property query E0900

The exact TU1.5 Primary leaf has 76 sites and no children. It is a genuine
callback of TypeQuery 10DE40. The older inherited-child proof covered 52
finite sites; the standalone hardware proof now covers all 76.

The original `flight` and `flying_speed` selector sites E093F and E0952
use the actual named compiled-symbol join in source. A third selector
`070000C9` reads the authored object byte and uses its separate constant
path. Its name is not inferred. Unknown or disabled queries leave both
outputs unchanged and return through the original RET16 frame.

The speed path reads its threshold before loading three components and
reads it again before division. It executes the original ordered x87
squared-length reduction, square root, stack compaction, and scalar spill.
Five temporary x87 slots are needed at its peak. The physical view stages
the spill: provider failure leaves ST0 live, and completed-write observers
run only after the write and pop have retired. It then reads the scratch
word again. The final active byte depends on the unclamped value.

- [x] Authenticate the complete original CFG, exact Primary owner and named
  symbol rows. `native-e0900-object-query-audit-20261110/report.json`.
- [x] Compare first complete GCC sanitizer physical domain: all 76 sites,
  144 original returns, 570 provider stops, 1,428 comparisons and 16,948
  full packets. Complete scratch owner removed.
- [x] Close both sanitizer compilers, retired-store observers and five
  semantic controls per compiler. Each passed 2,148 comparisons, 26,980
  pre-I/O packets, 2,232 observer packets and 720 observer-throw comparisons.
  The original ran 144 returns, 570 before-provider stops and 360 retired-store
  stops. `native-flight-property-query-frame-both-20261110/report.json`;
  complete owner removed.
- [x] Compare the current and genuine freshly generated Primary source bodies.
  The fresh profile explicitly clears C1 at six loads and three register
  pops and uses its `native_single_spill` status helper at E099F. In both
  source profiles, the observer runs before the modeled stack pop; the
  unchanged hardware oracle observes the retired pop. Keep those domains
  separate.
- [x] Compare the genuine current/fresh raw and prepared bodies with their
  real state and compiled-symbol service under GCC Release. All 76 sites,
  6,912 normal calls, 17,446 failure prefixes, 988,484 packets, 12 modes and
  48 current/fresh distinguishing witnesses pass. Owner removed.
  `native-flight-property-query-source-first-20261110/report.json`.
- [x] Close both source policies under GCC and Clang sanitizers, frozen
  `d1d8e8d7b011233a90466b51281247c2012b2180e272c5fed7cc8686585fc35b`.
  Per compiler, Primary passes 6,912 returns, 17,446 failure prefixes,
  988,484 packets and six deliberate defects. DeathCycle passes 6,912,
  17,458, 973,324 and seven, respectively. Owners removed. The earlier
  JSON packet driver reached its 1,200-second limit; complete typed packet
  comparison preserves the same matrix with lower trace overhead.
  `native-flight-property-query-source-policies-both-20261110/report.json`.
- [x] Install the Primary default and final owning generator hook. Exact raw
  and prepared reversal preserves all 2,025 other complete functions.
- [x] Authenticate the genuine separate DeathCycle current/fresh source.
  Current keeps fixed selector keys, 22 identity operands in its raw form,
  and the status spill helper without explicit C1 clears. Fresh uses the
  Primary named lookup and nine explicit clears. The public entry and local
  dispatch both retain the actual E0900 body.
  `native-e0900-death-cycle-source-audit-20261110/report.json`.
- [x] Install the separate DeathCycle default and generator hook. Its own
  reference control does not disable Primary. Exact raw and prepared inverse
  preserves 310 neighboring functions.
  `native-flight-property-query-dual-insertion-20261110/report.json`.
- [x] Installed registered 11/11, focused operand/copied helpers and catalog
  pass on `f7bf45c5532686b781cd0016601268c08fefb212326e4ad64bc9997e94c9f783`.
  The later linked build was interrupted before either public test ran;
  owner removed. `native-flight-property-query-installed-component-preservation-20261110/report.json`.
- [x] Build and run the actual linked public tests on independent
  `6b7b7ac658aec0961c1eb458daf863345ce8189351d0df253c591a4abebc6329`.
  Both tests reject the absolute host floating-point assertion. This
  outcome is separate from the earlier build interruption and incomplete
  helper copy. The owner was removed.
  `native-flight-property-query-linked-supplement-r2-20261110/report.json`.
- [x] Diagnose that failure using independently copied actual linked
  owners with eight exact owning inputs and both prepared receipts.
  Both owners match complete guest and host streams across default,
  own-reference and other-owner reference routes. All show the same
  change to caller x87 C1 on the same cases. Primary passes 576 returns,
  1,448 prefixes and 68,722 events per route; DeathCycle passes 576,
  1,183 and 50,895. Owner removed.
  `native-flight-property-query-host-fenv-diagnostic-r2-20261110/report.json`.
- [ ] Pass both registered public tests with exact guest and host
  stream comparison. Gameplay, all-map and other-image qualification
  remain open.

The first physical receipt is
`native-flight-property-query-frame-first-20261110/report.json`.
The hardware comparison uses declared dense masked FP state, including
live x87 prefix values. It does not qualify stack overflow, an unmasked
exception envelope, legacy images, or a production default.
