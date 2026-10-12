# Original shadow owner and candidate submission

`source_shadow_submission` implements the bounded raw-owner portion of the
ordinary `335E0` shadow phase. It does not infer a live shadow from authored
placement metadata. Its inputs are actual full handles, current object/header
fields, the actual ordered `547F9C` collector and its flags, current clock,
per-view `4BAF0` model fade bytes, `3E9C0` local-player priority and retained
candidate scratch state.

The unchanged source executable is pinned to SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
`tools/differential_source_shadow_submission.py` executes complete `3D7F0`,
including `BAF80` root fallback, unchanged `B9CE0`, full `31F50` sphere traversal,
and unchanged `33400` through its call into `4A7C0`. The fixture supplies the
outputs of `4BAF0` and `3E9C0` as explicit services; it does not claim their
upstream per-view model/observer producers. It passes360 cases /67,965 words
bit-exactly. The frozen fixture and adjacent provenance are
`tests/fixtures/retail_source_shadow_submission.tsv` and
`retail_source_shadow_submission.provenance.json`.

## Current owner fields

| Source field | Literal location | Required meaning |
| --- | --- | --- |
| Definition shadow mode | definition+18, signed short | mode0 admits types0/1; mode2 admits other ordinary owners |
| Object type | object+AA, signed byte | source class0..12 |
| No shadow | object+4, bit16 | blocks the initial root fade admission |
| Dying | object+10A, bit2 | type0 death fade uses the source clock |
| Death tick | object+BC | signed tick difference, then float multiplication |
| Camouflage | unit+2B0 | initial type0 factor and final header-kind0/1 factor |
| Header flags/kind | handle record+2/+3 | source blocking bit4 and final camouflage class |
| Parent | object+14 | `BAF80` follows this chain all the way to its root |
| Hierarchy links | object+C/+10 | next sibling / first child, from B93B0 insertion |
| Cached sphere | object+30..3C | actual center and radius, including current bounds producer |
| Clock | `510C54` bank+8/+4 | current tick and seconds per tick |

`3D7F0` first computes model/death fade and its type0 camouflage factor. A zero
from that first camouflage factor does not trigger parent fallback: the
eligibility byte remains the source result until the final header-kind factor.
Ineligible attached objects inherit their ultimate root's fade, then apply the
current object's final header factor. The local-shadow output always describes
the current object's type12, independently of inherited eligibility. The
native helper preserves these distinctions.

`3D7C0/B9CE0` separately blocks an owner if its current header bit4 or object
flag bit0 is set, checking immediate parents in sequence. `31F50` consumes every
literal cached sphere in the source's order: current owner, recursively visit
its next sibling at+C, then iterate its first child at+10, regardless of
the individual node's shadow eligibility. Missing handles, repeated/cyclic
links and missing required per-view values are explicit unresolved inputs.

## Collector and cache seam

`33400` reads the actual signed-short collector count at+4, flag pointer at+8,
and full-handle pointer at+C. Mode0 admits every row; mode1 requires flag4000;
mode2 requires8000; other modes append none. It still calls `3D7F0` for admitted
collector rows after filling its32-candidate output. Candidate identity and
owner are both the literal full handle. Projected diameter consumes the
current inverse-view depth plane, viewport half metric and owner's cached
sphere through the existing exact `source_shadow_coverage` kernel.

The source writes candidate flag bytes0,1,2, but does not write byte3. `335E0`
allocates this640-byte output on an uncleared stack. Consequently that byte is
retained input to `4A7C0/4AC30`, rather than a zero default. The new API requires
an explicit byte for each appended output slot. Its output stops before the
separate selection-history helper. The current source view index selects a
`5246F8 + clamp(view,0,3)*484` cache; the ordinary call retains that history,
preferred budget4 and acceleration.005.

## Cold and retained observations

`analysis/multiplayer-shading/shadow-cold-fields-original.py` observes the raw
fields after742 unchanged isolated automatic factory prefixes, at `B8890`
entry before successful registration. These comprise278 SCEN,49 MACH and415
BLOC across21 maps. All have header flags0A, parent/child/siblingNONE and
dying bit clear at this phase. There are36 no-shadow owners. The definition
mode census is:

| Class | mode0 | mode1 | mode2 | mode2 with no-shadow clear |
| --- | ---: | ---: | ---: | ---: |
| SCEN6 | 240 | 9 | 29 | 3 |
| MACH7 | 39 | 5 | 5 | 5 |
| BLOC11 | 169 | 181 | 65 | 65 |

These last counts are cold field admission, not visible shadow counts. The
factory fixture's explicit allocator/completed-animation services and isolated
handle12340000 do not establish successful global registration or chronology.
No cold player/held constructor result is claimed.

The separate read-only `shadow-retained-owner-fields.py` validates the existing
paused Zanzibar and Ivory/Cyclotron player/held exports against actual full
handle records, cache definitions and v3 reference spheres. The roots are
type0/header-kind0, definition mode0, camouflage0, dying clear, and parentNONE.
Their weapons are type2/header-kind2, definition mode0 and parent=root. Literal
root+C isNONE and root+10 is the held weapon in both exports; the helper uses
these captured links literally. These are live snapshot observations, not
cold constructor values or a general attachment-history rule. Their current
clock, per-view fade/priority, `3DC90` state and collector/receiver queues were
not retained by this export. The JSON report records every input hash.

The later actual shadow phase fixture is described in
`source-shadow-live-phase.md`. It retains the missing clock, collector,
observed fade/priority, corroborated complete model-fade/local-player-priority
inputs, selection history,3DC90 state and original uniform outputs. The native
test computes both producers and retains the observed results as independent
assertions before candidate generation. It also establishes the semantic link
names directly: B93B0 stores
the old parent's+10 child at the new child's+C sibling, then stores the new
child at the parent's+10. The raw fixture order and31F50 arithmetic have not
changed; the public fields now use those verified names.

## Boundary before complete live shadows

`335E0` still requires actual `226D0/335C0` admission and current flags. Its
`32350` call re-evaluates `3D7F0`, requires no parent, `4BA020` and `3D7C0`, then
reads actual retained `3DC90` opacity at+1C. That opacity and runtime fade must
produce a value greater than.09. The owner sphere/hierarchy and light direction
then feed `320B0` view clipping, `C1D90 -> 1320F0` secondary collection, and
`165A20 -> 2CBC0 -> 44720` part/subpart admission. Generation layer10 and
receiver layer11 groups, geometry shadow modifiers, receiver OBB availability,
current PVS, successful ownership/registration order and retained histories are
independent prerequisites. Type12 local shadows follow a separate path.

The fixed GPU target/pass helper is documented separately in
`SOURCE_SHADOW_GPU_PARITY.md`. Its explicit draw callbacks can replay retained
source draw records independently of live candidate selection. Neither helper
currently proves a complete native live shadow producer or physical Xbox
frame equality.
