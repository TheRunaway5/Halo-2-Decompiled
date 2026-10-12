# Original transparent command scheduling

`source_transparent_sort.hpp/.cpp` implements the original command comparator,
ordinal sorter, screen-section group builders and bounded ordinary transparent
part command producer. The authority is the supplied Xbox
`assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
This module has no renderer integration or framebuffer parity claim.

## Commands and source order

`SourceTransparentCommand` retains the literal `0x70` layout at `4B6288`:
priority `+00`, depth `+04`, secondary key `+08`, low-byte plane flag `+0C`,
plane `+10`, callback `+20`, opaque 64-byte payload `+24`, center `+64`.
Compile-time assertions cover every field offset. The callback and payload
remain source tokens; sorting never substitutes native material indices.

`1E370` initializes 16-bit ordinals from `4B6284` through `4B6280`, invokes
`13DF80`, then executes each non-null callback with its command payload. The
native `first` argument retains that suffix boundary. `3FDF0/40870` use the
same bank for the separate first-person sequence. In `2BCD0`, the main
`1E370` call follows `36B50` and precedes first-person `404B0`.

`1E700` first subtracts signed priority keys with 32-bit wrapping. The sorter
places comparator maxima at the end, so ordinary priority keys are ascending.
When the first command has a plane, its plane governs the comparison; otherwise
the second command's plane governs it with the opposite result. Camera and
other-command centers are tested against the authored plane D. Camera tests
retain `(Y+Z)+X`; center tests retain `(Z+Y)+X`. These branches do not guarantee
a strict weak ordering.

With no plane, greater depth compares true; reverse greater depth compares
false; equal or unordered depths fall through to the signed wrapping secondary
key difference. The original ordered `COMISS` conditions are retained for NaN,
infinity and signed zero. Only the low eight bits of the plane flag are consumed.

`13DF80` uses its midpoint pivot, original partition swaps and 30-entry range
stack. Partitions of at most eight elements use `13E140` selection sorting.
Equal keys are deliberately unstable. `std::sort` and stable sorting would both
change observable command order. The module accepts at most 1,024 commands,
the original allocator capacity, and rejects invalid suffix ranges.

## Ordinary authored parts

The ordinary path is `40E90 → 44AC0 → 45CE0 → 41040`. `44AC0` traverses source
draw units and source part ordinals. `45CE0` produces a 44-byte context with the
selected shader, actual center, depth bias, optional plane and shader LOD scale.

The bounded `source_transparent_part_command` admits an ordinary command only
when the authored part type is neither 0 nor 5, its material is present, the
actual selected shader/template LOD exposes layer 15 (`0x8000`), view type is 0,
and the caller is outside a screen-section group. Its callback is `41020`;
payload words retain the actual selected shader and packed source draw token.
The token is `((distanceByte << 16 | drawUnitID) << 8) | partOrdinal` in `41040`.

Transparent priority reads raw selected shader byte `+0x16`, bits 0/1, before
calling `40E10`. This is separate from shader word `+0x3C`, which the same
routine supplies to the general `40E30` command path. They are not alternative
metadata layouts.

The transparent depth calculation retains the literal SSE operations:
`((0 - ((deltaZ*forwardZ + deltaY*forwardY) + deltaX*forwardX)) - depthBias)`.
It uses the actual center and camera, rather than a texture, material or triangle
average. `40F60` resolves the retained source draw token and runs the authored
layer 15 pass count for that part.

The authoritative inputs recovered for the separate metadata producer are:

| Source | Center and additional fields |
| --- | --- |
| BSP cluster | Literal authored `0x48`-byte part center at `+0x10` |
| BSP instance | Instance precomputed world center at `+0x3C/+0x40/+0x44` |
| Model section | Actual posed rigid-node matrix translation; section rigid-node index at `+0x2C`; NONE selects the object matrix translation |

The part also retains type/flags at `+0/+2`, signed material index `+4`, index
first/count `+6/+8`, signed subpart first/count `+0xA/+0xC`, and shader LOD scale
`+0x2C` (positive authored value, otherwise 1). Model depth bias is the draw-unit
byte `+0x1B` low nibble: zero gives 0, otherwise `(nibble-5)*0x3A83126F`.
Models have no optional plane in this producer. The model node remap branch
tests definition byte `+4` mask 4 and uses the current geometry LOD's node entry.

BSP transparent-plane records at root `+0x158/+0x15C` have 20-byte stride:
two unsigned-short key fields followed by the plane. `1E280`'s first key is the
cluster ID or instance definition ID, and its second key is
`partOrdinal*2 | instanceBit`. An instance plane is transformed with the literal
`45CE0` ordering: each normal component is `(Z*oldNz + Y*oldNy) + X*oldNx`,
then D is `((oldD*scale + translationZ*newNz) + translationY*newNy) +
translationX*newNx`. The separate metadata module owns their cache decoder and
source transform proof.

## Screen-section groups

`133AB0 → 133F70 → 2CBF0` produces the populated `4C152C` section bank.
`SourceTransparentSection` retains its 36-byte layout: owner depth, owner
center, rectangle `(x0,x1,y0,y1)` and kind. Screen rectangles come from actual
`30290` source camera/sphere projection or its explicit source toggles. They
must not be reconstructed from triangle bounds.

`420A0` seeds groups from unvisited kind 1 entries, visiting source section
ordinals in order. A candidate is admitted only when its rectangle has no
strict overlap with every current group member. Touching edges do not overlap.
The inner loop does **not** filter the candidate's kind. Kind 0 and kind 3 entries
can therefore join a kind 1 group. `42760` subsequently emits every kind 3 entry
without testing the earlier visited mask, preserving the original duplication.

Group callback `41CC0` retains a payload count byte followed by member IDs.
Its center is the midpoint of the component-wise center bounds and its depth
is the negative minimum owner depth. The component minimum retains the source
unordered comparison behavior. Kind 3 callback `423C0` retains its original
section ID and caller argument, own center and negative owner depth. These
callbacks submit original draw-unit groups and authored layers, rather than
one generic alpha pass.

The module preserves existing commands and appends only while source capacity
remains. Populated section IDs are 0 through 30; 31 is the NONE sentinel. Native
new records initialize unused bytes to 0. The executable allocator only clears
the consumed plane-flag byte; parity is claimed for its written/consumed fields,
not unrelated stale padding or unused payload bytes from a previous frame.

## Authored all-map inventory

`analysis/multiplayer-shading/transparent-authored-inventory.py/.json` records
literal cache/resource offsets, masks, raw part fields, plane records and owner
metadata, with a SHA256 for each supplied cache. The model section superset
comes from 925 placed render models and all six geometry LOD collectors. Its
automatic Slayer owner subset has 278 scenery, 48 rendered machines and 415 props.
The four unresolved Zanzibar pillars use explicit retained selector-fixture
bytes in this metadata census; their complete initialization is not inferred.

| Map cache | BSP planes | Cluster layer 15 parts | Instance-definition layer 15 parts | Distinct model-section layer 15 parts | Automatic owners with layer 15 at any LOD |
| --- | ---: | ---: | ---: | ---: | ---: |
| Ascension | 7 | 7 | 0 | 11 | 3 |
| Beavercreek | 13 | 13 | 0 | 5 | 2 |
| Burial Mounds | 0 | 0 | 0 | 5 | 5 |
| Coagulation | 32 | 32 | 3 | 10 | 4 |
| Colossus | 21 | 21 | 3 | 5 | 5 |
| Cyclotron | 56 | 56 | 1 | 4 | 11 |
| Foundation | 23 | 23 | 0 | 1 | 9 |
| Headlong | 62 | 62 | 1 | 3 | 16 |
| Lockout | 14 | 14 | 4 | 0 | 0 |
| Midship | 156 | 156 | 0 | 16 | 32 |
| Waterworks | 17 | 17 | 0 | 11 | 43 |
| Zanzibar | 34 | 34 | 2 | 8 | 25 |
| Backwash | 220 | 220 | 3 | 3 | 2 |
| Containment | 0 | 0 | 0 | 13 | 12 |
| Deltatap | 89 | 89 | 1 | 1 | 9 |
| Dune | 5 | 5 | 0 | 12 | 7 |
| Elongation | 0 | 0 | 8 | 3 | 13 |
| Gemini | 50 | 50 | 3 | 20 | 24 |
| Triplicate | 6 | 6 | 6 | 5 | 3 |
| Turf | 1 | 1 | 6 | 4 | 3 |
| Warlock | 31 | 31 | 4 | 6 | 12 |
| Total | 837 | 837 | 45 | 146 | 240 |

These are authored availability counts. A template exposing bit 15 at some
shader LOD does not establish actual shader LOD selection, view, visibility,
owner creation or command admission. There are 4,952 cluster parts, 968 instance
definition parts and 664 distinct model-section parts in this census. All 741
automatic rendered owners have HLMT `+0x9C` bit 0 clear and effect shader
`+0xF4` NONE. This does not prove absent runtime unit camouflage, effects or
other screen-section context producers.

## Validation and remaining integration

`tools/differential_source_transparent_sort.py` executes complete untouched
`1E700/13DF80/13E140/420A0/42760/1E2D0/40E10`. Its ordinary part tests execute
the untouched `41040` gate at `41120..4113A` and final ordinary command producer
at `41361..41477`. No called service is stubbed. The harness supplies actual
preceding register/stack inputs and explicitly represents FPCW027F/MXCSR1F80.
The omitted shader LOD, override and visibility paths remain caller services;
this is not a proof of complete `41040`.

The 4,170-case differential matches 524,362 output words bit-for-bit, including
priority overflow, plane/camera branches, NaNs, infinities, signed zeros,
equal-key swaps, suffix sorting, maximum capacity, strict rectangle touching,
mixed section kinds and remaining-capacity limits. The committed 550-case
fixture passes `retail_source_transparent_sort`. Standalone AddressSanitizer
and UndefinedBehaviorSanitizer checks also pass that fixture and explicit
invalid-range/section-capacity/opaque-payload retention tests.

Current renderer batching groups by material/lightmap/cluster and does not
retain all source owner/draw-unit/part identities. Integration needs the actual
source part and subpart ranges, posed centers, selected shader LOD/override
state, source token identities and actual visibility. `4DAA0` separately emits
whole parts or admitted subparts, coalescing adjacent strip ranges while list
subparts remain separate. Its visibility-mask producer is not supplied by the
sorter. Source effects, particle/light-volume queues and decals also contribute
commands through `40E90`; these producers are not replaced by layer 15 inference.
There is no claim that the complete native transparent schedule or visual
appearance is already identical to the original.
