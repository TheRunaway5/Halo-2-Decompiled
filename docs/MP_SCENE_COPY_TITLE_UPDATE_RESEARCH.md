# Scene-copy code across Xbox title updates

All21 bounded scene-copy owners have complete static correspondences in
retail, TU1.1 and TU1.5. The42 later-image comparisons check complete reached
instruction tokens, internal branches, switch topology, referenced read-only
bytes and direct references between admitted owners. This establishes code
correspondence for the listed owners. It does not execute a title-update copy
or establish corresponding live resource banks.
[Inspection tool](../tools/inspect_scene_copy_title_updates.py),
[complete report and relocated operands](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/report.json)

A fresh execution from a separately frozen compatible source tree reproduces
all64 outputs byte-for-byte: the report and63 complete owner listings. Its
manifest pins the source inventory, original report, command and log. The
report SHA-256 is
`feb111ab67ddb596a62469adb26b299960876206d2d928400ad806b07e397005`.
[Frozen reproduction](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/reproduction.json)

The outer scene copy is retail`352E0`, later`352D0`; the quad owner is
retail`34A90`, later`34A80`; its builder is retail`34420`, later`34410`.
The attribute kernel is retail`34100`, later`340F0`; the position kernel is
retail`350E0`, later`350D0`. Other admitted owners include target dimensions,
selection/surface publication, vertex preparation/layout/program selection,
pixel/texture publication, viewport setup and SDK dirty flushing. Each original
and corresponding later owner's complete listing is pinned in the report.
[Whole owner inventory and listings](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/report.json)

Two target-table leaf functions encode the table base as an immediate `ADD`
operand. The comparison identifies these addresses explicitly as mutable
target-record table pointers, retaining the original98-byte record stride,
field offsets and every other arithmetic operand. The corresponding dimension
and surface owners must select the same later table base. Actual calls from
the already matched copy/target owners constrain the leaf identities. The
report retains both raw operands and backing bytes; it makes no claim that
file-initialized mutable records are live-ready resources.
[Explicit target-table operands and owning call constraints](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/report.json)

The locator trims candidate prefixes to complete instructions. The preserved
earlier attempts show why a64-byte slice can end within a relocation-bearing
instruction and miss its actual counterpart, and why arithmetic table-pointer
relocations need an explicit recorded contract. Prefix matches locate
candidates; they do not establish correspondence. The complete comparisons
and reference constraints establish the reported result.
[First incomplete locator attempt](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/attempt-prefix-locator1/report.json),
[Second incomplete locator attempt](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/attempt-prefix-locator2/report.json)

The source pins cover the owned original retail/TU1.1/TU1.5 XBEs. The TU1.5
image remains the Xbox authority for this project; this comparison neither
substitutes Vista/MCC assets nor treats those products as matching original
Xbox balance or rendering. The earlier retail CPU corpus retains its exact
resource, camera and device inputs as a separate evidence boundary.
[Original image pins](../analysis/multiplayer/game-variants/scene-copy-title-updates-20261012/report.json),
[Retail whole-copy CPU execution](MP_CAMOUFLAGE_SCENE_COPY_SOURCE_RESEARCH.md),
[Actual target19/21/22 executions](MP_SCENE_COPY_DESTINATIONS_SOURCE_RESEARCH.md)

No external callee not included in this inventory gains equivalence from a
masked call operand. No title-update resource capture, original GPU sampling,
submission/completion, framebuffer output, normal camouflage/water scheduling
or additional game-mode admission follows. Full operational acceptance remains
the original data-driven local/network match flows.
[GPU evidence boundary](MP_SCENE_COPY_GPU_ORACLE_FEASIBILITY.md),
[Full goal](GAME_MODE_PROGRESS.md)
