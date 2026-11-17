# MapWorld property query 1DB800

TU1.5 actual `1DB800` is a 34-site childless scalar SSE/integer query in
MapWorld. Actual `11E490` has 30 sites and directly calls this child at
`11E4DA`. The complete source body hashes and instruction bytes for both
entries are authenticated in
`native-map-world-query-child-audit-20261110/report.json`.

MapPack11 uses both numeric addresses for different original routines:
`11E490` has 43 sites and calls `329168`; `1DB800` has 17 sites and calls
`1D3B40` and `1EEB20`. The numeric match provides no legacy-image owner
qualification. No correspondence or legacy reachability is inferred.

The authored TU1.5 selector cases are `0A000693` (the named `phantom_on`
comparison), `0D000691` and `0D000692`. The flag path tests CH bit 5 from
record+4 and performs the original constant read even when the flag is clear.
The latter two paths read unsigned bytes from record+1B and record+1A,
convert to scalar float and multiply by the live scale. An unknown key
publishes AL=0 and skips both outputs. A recognized key publishes the scalar
before reading the byte-output pointer and writes the positive-result byte
from the live comparison flags. All paths retain RET4 cleanup.

- [x] Authenticate the complete TU1.5 and distinct same-address MapPack11 CFGs.
- [x] Draft the private typed policy and physical register/provider adapter.
- [x] Pass the first GCC sanitizer physical gate: all34 sites, 144 original returns, 105 provider stops, 498 comparisons and 1,668 complete pre-I/O packets.
- [x] Close both sanitizer compilers and five compiled defects each (498 comparisons and 1,668 complete pre-I/O packets/compiler); registered physical test.
- [ ] Close physical observers and observer exceptions.
- [x] Qualify the actual source symbol resolver and raw/prepared MapWorld transport under both sanitizer compilers.
- [x] Qualify the complete 11E490 physical caller and genuine 1DB800 child under both sanitizer compilers.
- [x] Close both source compilers for the caller and install the exact pair.
- [ ] Close the actual linked public API and coherent runtime.

The first private physical gate includes a scalar output aliased to the late
byte-output pointer, plus ordinary/hostile providers and scalar FP cases. Its
owner is removed. `native-map-world-property-query-first-20261110/report.json`.
No source or production binding exists.

Both compiler receipts are in `native-map-world-property-query-both-20261110/report.json`.
The second gate compares the same original 144 returns and 105 stops and
rejects each changed selector/byte/cleanup/active-flag/read behavior. Its
owner is removed. All10 child and 15 parent selected literals are verified
identities in the exact TU1.5 owner; the current raw/prepared bodies have
34/30 callbacks. `native-map-world-property-query-source-identities-20261110/report.json`.
The private raw/prepared source view preserves the live symbol resolver and
all34 sites. Its final both-sanitizer proof passes 1,728 returns and 875
instruction/provider/observer stops with four compiled defects/compiler. The
first diagnostics remain preserved: an identity-literal count typo (actual10)
and a feedback fixture that made a deliberate stack-output alias null. The
corrected feedback domain preserves its declared mapped pointer. No production
binding exists. `native-map-world-property-query-source-both-20261110/report.json`.

The genuine caller/child physical proof covers all64 sites in 144 returns and
560 original provider prefixes, including 288 child entries. Both compilers
compare 1,408 runs and 17,670 complete packets. The corrected oracle accounts
for the original POP implicit reads and tests aliasing the child's pushed
byte-output pointer. First incomplete trace diagnostic remains separate.
`native-map-world-property-caller-both-r2-20261110/report.json`.
The source composition passes first GCC Release: all64 sites, twelve modes,
1,728 normal calls, 2,895 prefixes and 202,119 complete packets. Both source
sanitizers/defects and genuine fresh binding are underway. All finished
owners removed; no production binding or launcher change.
`native-map-world-property-caller-source-first-20261110/report.json`.

The exact current and genuine fresh raw/prepared pairs now pass four complete
C++ syntax checks, identity-only body projection, exact reverse/idempotence
and retagging, and 44 strict rejection controls. Their owning contexts include
the complete MapWorld resolver, dispatcher, State, public API and owner map.
The disposable owner is removed; default remains unbound while both source
compiler checks finish. `native-map-world-property-query-binding-private-20261110/report.json`.

Both composed source sanitizers pass 1,728 returns, 2,895 prefixes, 202,119
packets and four compiled defects each. The exact MapWorld defaults are
installed with raw/prepared inverse preserving 191 other bodies and the
actual child edge. The installed source SHA is
`637c76aef7db9a6b9790dc30e024f56dd1e5f266c167dee373c7b0feaf1637ba`.
Registered frames, source, binding and selector pass 6/6 (32.94 seconds);
the two focused operand/catalog regressions also pass. All completed owners
removed. Receipts:
`native-map-world-property-caller-source-both-20261110/report.json`,
`native-map-world-property-query-insertion-20261110/report.json`,
`native-map-world-property-query-installed-components-20261110/report.json`.

The actual linked MapWorld API from R29 passed all five route controls.
Each matched 1,152 normal calls, 1,990 failure prefixes and 86,752 events.
The default entered the caller 2,126 times and the leaf 2,056 times. Each
own-reference switch suppressed only its route, the two switches together
suppressed both, and an unrelated switch preserved both. The owned scratch
was removed. Whole-runtime qualification remains open.
`native-map-world-property-linked-public-r29-20261110/report.json`.
