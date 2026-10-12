# Original retained player identity and machine remapping

The shared native result module now implements the original retained-context
remapper and machine binder. It preserves the original record sizes, identities,
both PvP axes, event participant IDs and version-specific machine retention.
It is compiled into the systems library. Native source identity/configuration
production and its live participant callbacks are still required before binding
this owner to a match; the Carnage Report remains unavailable.

## Original owner evidence

| Owner | Retail | TU1.1 | U5 |
| --- | --- | --- | --- |
| Remap retained records | `196470` | `197F70` | `19AE70` |
| Bind retained machines | `199310` | `19AE10` | `19DD10` |
| Find machine by identity | `199250` | `19AD50` | `19DC50` |
| Allocate retained machine | `1991F0` | `19ACF0` | `19DBF0` |
| Actual stack probe | `320560` | `326AF0` | `32A660` |
| Retained-context base | `510CB0` | `5184A0` | `51DFB0` |

All original instructions and reached game routines execute independently in
the oracle. The large source stack frame uses its actual probe, without a
supplied return. Recording flags, the complete retained context, occupied live
pool records, configurations, identities, machine roster and high-water bound
are explicit API inputs. No platform identity producer is replaced by a success.

The remap owners have the same complete223-instruction shape across all three
images, apart from five declared context-address CMP immediates. Actual context
offsets and known remap/binder/probe callees are checked. Machine lookup and
allocation also match completely. Retail/TU1.1 machine binders match each other
but have115 instructions, versus U5's87; that behavioral difference is preserved
instead of declaring an unsupported cross-version equivalence.

## Full data layout and mutation

The original initializer clears exactly`DE78` bytes at the retained-context
base. The shared native structure keeps all bytes of this layout:

| Offset | Records |
| --- | --- |
| `0000` | `384` raw header/settings bytes; header+1 is completion |
| `0384` | 16 player-property records, each`A4` bytes |
| `0DC4` | 16 raw team-property records, each24 bytes |
| `0F44` | Original`4040`-byte player/PvP/team statistic bank |
| `4F84` | 1,000 raw event records, each36 bytes |
| `DC24` | 16 machine records, each11 bytes |
| `DCD4` | `1A4` remaining raw bytes |

A player-property record contains presence at+0, signed machine selector at+1,
the exact12-byte identity at+2, two preserved bytes at+E, and the exact144-byte
live player+44 configuration at+10. Rank's signed byte at+A0 is independently written;
the following byte and score's16-bit word at+A2 remain unchanged during new-row
creation. Unclassified configuration fields are copied verbatim. New rows are
not reconstructed from a name or native player datum.
The actual signed readers`19B5F0` and`19B570` use MOVSX for the rank byte and
score word respectively; their complete static listings are retained under
`result-retention/property-readers`. They are field-type evidence, not a claim
that the source valid/complete report getters are bound to a native match.

The owner requires recording-enabled and completion zero. Its recording-frozen
byte is **not** the completion flag and does not stop this remapper. In ascending
live-slot order, it finds the first occupied retained record with the same full
12-byte identity. Each retained source row can be claimed only once. A later
duplicate live identity stops at that first match even if already claimed,
then follows the unmatched path; it does not search another duplicate row.

Matched occupied destinations copy the whole old`A4` property record.
Unmatched occupied destinations set presence1, identity/configuration, machine
NONE and rank byte`FF`, preserving all other bytes. Nonlive property rows remain
unchanged. The separate437-word player-stat records are copied by the same
mapping or completely zeroed when unmatched, including nonlive destinations.
PvP pairs are mapped on both row and column axes; either unmatched endpoint
zeros the complete two-word pair. Team statistics stay unchanged.

Every event's signed participant bytes at+1/+2 use the inverse retained-to-live
mapping. Negative IDs and IDs16..127 become NONE. Every other event byte remains
raw and unchanged. Original sources process all1,000 records, not just a guessed
event count.

The machine binder reads the original16×6-byte live machine roster and each
occupied live player's signed16 machine selector at+1A. Its first pass resolves
existing retained machines by all six identity bytes. Its second pass retries
lookup for unresolved players and allocates the first empty11-byte record if
necessary, zeroing that record, copying the identity and setting presence1.
A full retained table returns NONE. Existing records preserve their remaining
four raw bytes.

Between those passes, retail and TU1.1 clear machine records not referenced by
occupied retained properties. This mask includes retained nonlive property
rows; it is not simply the live roster. The original x86 shift masks selector
bits with31. U5 omits pruning and preserves its unused retained machines.
The shared policy selects the verified owner behavior. Live machine selectors
outside NONE/0..15 are unavailable in this bounded native API: the source would
read outside the declared roster, so no fallback identity is invented.

## Verification and remaining runtime work

The fixture contains1,224 whole original calls across three unchanged images:
936 complete remaps and288 complete machine binders. It covers slot permutations,
duplicate player/machine identities, occupied holes, new/departed players,
signed high-water boundaries, enabled/frozen/completion bytes, full machine
tables and repeated six-step transitions. Retail/TU1.1 outputs agree; U5 differs
where its actual machine policy differs.

Native goldens compare SHA256 over every byte of the complete`DE78` context,
before and after each call. Nonzero poison covers all unclassified fields.
The oracle separately verifies unchanged header/settings, team properties/team
stats, other event bytes and trailing raw fields. There are1,111 mutating calls
across the three images. Nine complete source pairs cover745 instruction rows;
all20 instruction listings and the fixture/report are independently reproduced.
Five U5 initialization/update/participant callers are static evidence only;
their external services are not declared successful executions.

Fixture SHA256:
`46e0d0a6714902825139bc4564e27864ba681159a8aed73cafd797b2ed637156`.

The complete remap/binder implementation passes ASan, UBSan and leak detection.
Nineteen rebuilt focused regressions pass. A freshly rebuilt production Team
Slayer command advances300 ticks with four local players on actual Lockout
content. The strict audit still runs78 normal menu starts, admits2/39 authored
variants and blocks37 (expected exit1). These checks do not prove a rendered
PCR, complete authored match, physical LAN or original Xbox wire/hardware parity.
Current pins and logs are in
`analysis/multiplayer/game-variants/result-retention-verification.json`.

Native matches still publish their counters into the existing live-slot bank.
They do not yet provide the original12-byte identities, complete144-byte player
configurations and6-byte machine roster to this remapper. Capture/ranking,
valid/complete publication, additional stat/event producers, peer retained-bank
transport, PCR construction/navigation and source session/frontend callbacks
remain required. No game mode, weapon or edited rule gained admission, and the
operational goal remains active and incomplete.

```sh
.tools/python/bin/python tools/oracle_result_retention.py
cmake --build build --target result_retention_tests -j2
ctest --test-dir build -R '^original_result_retention$' --output-on-failure
```
