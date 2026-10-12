# Actual ordinary shadow phase fixture

The native fade/priority/owner/candidate/selection/frame/uniform helpers match **135 of135
checked source words exactly** for one actual original Zanzibar shadow phase:
125 downstream words, seven fade-call count/identity/output words, and three
priority-call count/identity/AL words.
The fixture contains202 complete live owners, the literal41-row main object
collector, source clock/view inputs, the actual candidate scratch bank, active
selection history and cached lighting. The complete `source_model_fade` service
computes both corroborated original fade calls. Their independently observed
output bytes remain assertions; only the computed second bytes feed the native
candidate and root processing. The complete `source_local_player_priority`
service similarly computes priority from corroborated observer/local-user and
root-owner inputs; the original observed AL remains an independent assertion.

The fixture is `tests/fixtures/retail_source_shadow_live.tsv`, with adjacent
provenance JSON, generator `tools/generate_source_shadow_live_fixture.py` and
CTest `retail_source_shadow_live`. The source executable SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

## Session and phase authority

The original evidence HDD/base/EEPROM and retained snapshots were preserved.
A separate copied session restored `zanzibar-shading-baseline`, with unique
HDD, EEPROM, profile, display, QMP socket and GDB endpoint. Its neutral
keyboard-backed Xbox controller uses25 SDL_UNKNOWN bindings; no gameplay
input was sent. Only this copied session advanced. The foreign frontend
session was not accessed or changed.

This capture is **SourceBackendOpenGL / llvmpipe**, using the pinned xemu
0.8.136 revision `fc24584ce88f0915ad7f04775bb7712c2e3f49ee`. It is not the
later retained original20 Vulkan GPU frame and is not a physical Xbox capture.
The restored snapshot was originally paused at12B33E/tick12474. The actual
captured shadow phases below all belong to tick12475, frame111186, view0.

The private session stopped at each address through GDB Z1 emulator
breakpoints. No Z0 software breakpoints or guest-memory write packets were
used. The original335E0,4A1E0 and48EE0 instruction bytes were retained before
and after, with the uniform producer bytes verified against the original XBE.
The guest remains paused after3290A. Actual source arithmetic and the source
GPU uploads ran normally; no source math or upload service hooks produced
these expected values.

| Address | Recorded state |
| --- | --- |
| 335E0 | Entry gates/caller mode, complete owners and both spatial/cache pools |
| 33400 | Literal collector and uncleared640-byte candidate scratch input |
| 4BAF0 returns | Two actual per-owner model fade outputs |
| 3E9C0 return | Actual candidate priority byte |
| 4A7C0 | Candidate before history/selection and retained history input |
| 33632 | Selected candidate output and updated history |
| 32350 | Actual root/full handle and requested fade |
| 32410 | Actual3DC90 cached84-byte lighting state |
| 32875 | Actual secondary context/visibility/object admission output |
| 32893 | Actual post44720 draw-unit bank and source frame globals |
| 4A1E0 | Actual fixed-shadow camera and124-byte parameter bank |
| 3290A | Original c31..39/c40..48 banks after4A1E0/48EE0 uploads |

Immutable capture manifests and raw records are under
`analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl/`.
`capture.json` covers the first8 phases; `uniform-capture.json` covers the last2.
`shadow-live-original.json` verifies3,584 captured file sizes/hashes and records
the exact fixture comparison. Full physical RAM was deliberately not read in
this GPU-active phase: GPU-coherent memory access can block. CPU-owned banks
were exported directly instead.

## Concrete source inputs and outputs

The main collector admits one candidate, root `E2E60077`. Its five words are:

```
00000001 E2E60077 E2E60077 3F800000 458C9E69
```

The observed4BAF0 outputs are `(255,0)` for Warthog `E2FF0090` and `(255,255)`
for Chief/root `E2E60077`; only the second byte feeds3D7F0. The version3 fixture
contains each full reconstructed owner/definition/model bank and the literal
incoming XMM1 bits (`41A2D29E` and `3E5146C0`). The test calls the production-linked
`source_model_fade`, checks both computed bytes against these independently
observed outputs, and binds its computed second byte into the owner's state.
It also checks that the call's flags, parent and radius match the live shadow
owner. Original3E9C0 returns priority1 for `E2E60077`. The test computes that
priority with `source_local_player_priority`, checks its AL against the
independently observed result, then supplies only the computed byte to
candidates. Every owner without reconstructed call inputs keeps its
fade/priority absent; requesting an absent result throws instead of using zero.

The priority input bank contains current local slot0, callback`23C110` and
scalar`+0`, local-player datum`EC700000`, and root`E2E60077` with parentNONE,
headerkind0 and body+13C=`EC700000`. Parent/header/player/current-slot fields
match the earlier33400 exports. Observer/local-user records were exported
later at3290A; the consumed fields agree with both earlier complete RAM banks.
The actual original AL1 independently requires observer class0. This is
corroborated metadata, not a simultaneous3E9C0 raw observer export. The complete
provenance is under `priority-metadata-3290A/capture.json` and
`SOURCE_LOCAL_PLAYER_PRIORITY_PARITY.md`.

The generator also executes unchanged3E9C0/155760/BAF80 against the actual202
owner/header records and those corroborated raw observer/local-user banks.
It matches both original AL1 and the complete observed EAX`82D96901`, without
owner-lookup or return service hooks. A separate negative control changes the
local-player datum; it fails the independent priority assertion before
candidate generation. The test never feeds the observed priority directly
into the candidate callback.

The complete model metadata was exported later from the same copied session
paused at3290A/tick12475/frame111186/view0. Its manifest is
`shadow-live-phase-20261008/fade-metadata-3290A/capture.json`.
Warthog definition`E64E04D9` refers to model`E64F04DA`; Chief
definition`EDF10C6B` refers to model`EDF20C6C`. Both parents are NONE. The
complete prefixes match the literal Zanzibar cache, and the earlier exported
definition prefixes agree. Consumed definition/parent/radius/flag19 fields
match33400-entry. Chief's unused flag30 clears between phases; the fixture
retains the literal earlier flags`40001B10`, rather than substituting the later
`00001B10`.

Both actual fade calls enter with XMM2/XMM3 low lanes0 and exit with actual
radius×7 and1. The unchanged forced-return path performs no SSE writes; only
ordinary processing produces that transition. This independently establishes
call-time modifier0. The later direct4BA019 byte export is also0 and remains
separately labeled. Complete metadata and the literal global byte were not
exported simultaneously at fade entry. The reconstruction preserves this
temporal distinction, with full per-field/hash provenance adjacent to the
fixture and the complete arithmetic proof in `SOURCE_MODEL_FADE_PARITY.md`.

The controlled negative input replaces the observed Warthog metric with0.
Its computed bytes then fail the independent255/0 assertion, demonstrating
that the native fade producer is actually used. This is a test control, not
an additional observed gameplay state. Normal and ASAN/UBSAN CTests pass,
including CPU leak detection; no GPU is used. Exact test/source/binary/fixture
hashes and logs for the fade-only extension remain retained in
`analysis/multiplayer-shading/shadow-live-fade-validation.json`; the current
priority extension is recorded separately in `shadow-live-priority-validation.json`.

The source scratch byte3 input has32 literal bytes. The first is0, but the
bank also contains192,255,208,226,190,189 and other nonzero values. The fixture
retains all of them. Selection retains one history entry, changes its
last-seen frame111185→111186 and preserves creation frame98717. The cinematic
pointer was read through510C50; its actual byte5 is0.

The source root sphere is `(-10.166729927062988,13.273162841796875,
0.4817855656147003;0.4399999976158142)`. The actual3DC90 direction words are
`3F36F086 BE2DB9A5 BF2DBB6C`; cached opacity is `3F3FA8ED`. The source's
31F50 traversal includes root `E2E60077` and held weapon `E2E70078`.
The native29-field SourceShadowFrame matches the literal32350/C1A80 outputs.
Native c31..36 and c40..48 match the actual original uploaded60 words exactly,
using the literal4C1A80 feature bit10. This is a strict observation for this
phase; it does not upgrade the earlier broad uniform differential's tolerance
results to universal bit equality.

An additional unchanged31F50 run uses these actual virtual owner/header
records in an independent Unicorn clone. All88 executed instruction addresses
are checked against the original XBE. The clone has no math, lookup or target
service hooks; it independently records the literal two-sphere traversal.

## Verified link names

Original B93B0 attaches a child by storing the old parent's+10 first-child
handle into the new child's+C next-sibling link, then replacing parent+10.
Parent remains+14. The actual root has+C=NONE and+10=`E2E70078`; the weapon's
parent+14 is `E2E60077`.

`SourceShadowOwnerState` now names+C `sibling` and+10 `child`. Original31F50
includes the current sphere, recursively visits+C, then iterates+10. The
native traversal preserves exactly that source order. Both old and new
fixtures retain the original literal input order; the naming correction does
not change their arithmetic or expected words.

## Remaining integration boundary

Actual main and secondary visibility/context banks, all202 raw object bodies,
both512-head spatial pools and full render-cache records are retained. The
main bank has1 source frustum,15 clusters and15 clipped frusta. At the captured
secondary phase, there is1 source frustum,1 cluster,0 clipped frusta and1
ordinary owner (`E2E60077`). The post44720 CPU draw-unit bank has63 records;
the source secondary range is57..62. These are preserved source states, not a
new native registration/PVS/part-admission implementation. The nonzero queue
checked by32350 at54AA74 is the secondary cluster queue; its count1 is not a
claim about final GPU draw count.

The copied phase's ordered registration/collector/cache state, actual caller
XMM1 metric/current owner-model state/4BA019 production inputs, actual per-view
local-slot/observer/local-user inputs for3E9C0, and later source
part-program/receiver submission remain independent prerequisites for live
native shadows. No shader program after40E90 or full GPU silhouette is claimed
by this fixture. The separate retained original20 shadow silhouette replay has
its own GPU proof in `SOURCE_SHADOW_GPU_PARITY.md`; its caster/receiver data
must not be silently relabeled as this phase.
