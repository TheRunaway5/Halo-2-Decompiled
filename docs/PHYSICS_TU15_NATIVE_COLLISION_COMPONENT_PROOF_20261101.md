# Native TU1.5 collision component visibility and surface resolver

`native_tu15_collision_component_enabled` (`1F76A0`,82 sites) and
`native_tu15_collision_channel_enabled` (`186E70`,27 sites) pass all 187
physical cases on GCC and Clang. The [current-source report](../analysis/physics/native-collision-component-visibility-tu15-20261101-final/report.json)
contains 162 complete calls,25 read-fault prefixes,1,193 ordered reads and
6,081 original instructions per compiler. Three semantic mutations are
rejected. The helpers use authored keys, current BSP, visibility banks and
object generation fields; they have no CPU interpreter or floating-point work.

The cases exercise global and instanced channels through255, disabled flags,
signed BSP values, object generations0/1/127/128/255, inactive/sentinel objects,
kind2's unconditional branch and the channelFFFFFFFF sentinel. The source
zero-extends byte channel255 before comparing against-1; its bitmap read is
retained. Signed object generations are compared to the unsigned encoded key
generation exactly. The immutable five-target dispatch table is authenticated;
native code expresses its branches as a C++ switch.

An earlier RED ledger omitted `TEST [memory],register` reads because this
Capstone version reported access zero at `1F774E` and `186EB9`. The observer
metadata now correctly marks TEST/CMP memory operands read-only and refuses
any other unclassified operand. Original instructions and native algorithms
were unchanged by that correction. Earlier failed reports remain preserved.

## Strict surface-resolver boundary

The new native `1F7C10` surface-key paths1/5 include native visibility,
`1F67C0`'s argument staging and native `1F5FE0` construction. Shared/object
paths2/3/4 remain explicit refusals. The [full strict comparison](../analysis/physics/native-collision-component-tu15-20261101-full-strict/report.json)
remains **RED** on both compilers: zero of120 cases pass the unchanged complete
before/read/write ledger. No byte waiver, expected AFTER input or saved GP
serialization is introduced.

The separate [diagnostic aggregate](../analysis/physics/native-collision-component-tu15-20261101-full-strict/strict-boundary-aggregate.json)
records the useful boundary precisely. Every one of120 cases per compiler
matches all persistent output, complete FP state and final argument/XYZ/resource
DATA packets. Every source read value, written value, address and event order
also matches:15,431 events and10,852,036,536 persistent bytes per compiler.
Only a newly written DATA cell's old bytes differ. Three mutations are rejected
for semantic differences beyond that already-failing baseline. These results
do not turn the strict result green or prove full `304350` raw parity.

## DATA initialization and prior control lifetime

For a successful non-null surface constructor, the complete typed argument
record is initialized before any constructor read. The authenticated linear
construction sequences are:

| Path | DATA writes before constructor entry |
| --- | --- |
| Global kind1 | `1F7C4E` key; `1F7C4F` surface index; `1F7C55` resource sentinel; `1F7C57` category; `1F7C58` model |
| Instanced kind5 | `1F7D32` source part/resource cell; `1F67C4` key; `1F67C9` surface index from that cell; `1F67D0` resource identity; `1F67D1` category; `1F67D3` model |

Every listed argument store is a write-only source PUSH of its new semantic
value. No store reads its destination's old bytes. The `1F5FE0` entry occurs
only after all five fields are defined; its later overwrites of argument0/1
and subsequent reads are preserved by the native kernel. The placed resource
cell is defined at `1F7D32` before the wrapper reads it at `1F67C5`. Native
arguments distinguish the encoded surface index from the collision record's
resource identity; the initial placed-path slot swap was found by the first
comparison and corrected from this source sequence.

The optional XYZ packet is written at `1F60D1`,`1F60FA`,`1F6128` before the
output-copy reads all three scalars. The unplaced branch never reads XYZ.
Disabled/null-output branches do not enter a constructor. These are local
initialization/dominance facts for separately owned data and output areas.
They do not establish arbitrary aliases with source control storage.

Before argument construction, genuine helper prologues used those same source
addresses for saved registers. The native visibility helpers have no such
control-storage writes. Consequently the strict observer's old bytes differ
when later write-only argument initialization begins, even though subsequent
reads and results match. This explains the local observation. It does not
authorize materializing a GP file, old EBP identity or return bytes, and does
not resolve the wider `303EE0` constructor's copy of uninitialized bytes into
persistent records. Future allocations and full producer raw storage remain
open separately.

All authority is unchanged English TU1.5 XBE SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The production source/header hashes are frozen in each report. The completed
scratch owners are absent, with allocated bytes recorded in the adjacent
cleanup receipts. Only bounded reports, source and required old reference
inputs remain; no new large artifact is retained.
