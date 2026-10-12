# Retained model-resource startup and admission

The source-enabled native Match now constructs and activates the original
model-resource pools in its retained world. Actual session options drive the
original stage selector and physical backing reservation. The original model
admission owner joins the same dispatcher used by fatal cleanup; pending
headers remain nonresident until genuine resource I/O and fixups complete.

The eight complete added bodies contain444 original instructions:

| TU1.1 owner | TU1.5 pair | Responsibility |
|---|---|---|
| `12F020` | `1303C0` | Allocate and construct both resource pools and physical cache |
| `12F260` | `130600` | Activate/reset the two pools and resource queue counters |
| `13E8E0` | `13FC50` | Construct the physical cache and its nested pool/callback table |
| `1908E0` | `191CA0` | Select stage from actual options |
| `12F160` | `130500` | Reserve physical resource backing for the selected stage |
| `13F020` | `140390` | Apply the cache page capacity |
| `1703D0` | `1717B0` | Admit the authored model resource headers |
| `12F420` | `1307C0` | Touch a resident record or enqueue a nonresident header |

The complete body/call correspondence comes from the unchanged original
executables in the [resource report](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/effect-research/paired/report.json)
and [stage report](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/effect-research/stage/report.json).
The generator now emits284 complete startup bodies,16,100 instructions. Its
independent regeneration is byte-identical. Stored callback addresses remain
the original addresses; no replacement identity or resource result is supplied.

## Live connections

`MultiplayerObjectRuntime::Impl` calls `12F020` after real physical/async/arena
startup. It uses the existing original `11E150 → 33785C` allocator and the native
ordinal `B8` implementation, backed by actual owned storage. `12F260` executes
after new-map object/render-cache activation. Both512×12 and350×8 pools, the
512×24 nested cache pool and their original validity/callback fields are retained
in the same source bank as the live players and objects.

`configure_session` delivers the existing native multiplayer options, executes
`1908E0` with `ESI=game+8`, and passes its actual result to `12F160`. The source
multiplayer packet's first DWORD is2, so the selector returns1 and the original
callback reserves7 MiB. The size is selected by source execution. The original
caller `190930` invokes this callback through `45AD78+3C`; this implementation
isolates that callback on the native world's current physical-bank depth. It
does not execute or claim the other `190930` lifecycle callbacks or full retail
stage push/pop ordering.

Fatal cleanup reaches `1703D0` through its existing original call operand.
`12F420` allocates from the actual pending pool, stores the authored header
pointer, and sets its original queued flag. Its returned AL remains false for
that nonresident header. Admission alone does not load a model or enable shield
rendering. The original queue processor, actual cache-file publication, async
read, completion and fixup graph remain required.

## Original/native evidence

The [original capture](../tools/oracle_source_model_resource_startup_1_1.py)
executes the unchanged physical/async/object/cache startup prefix, then the
selected resource roots and repeated admission of an actual Lockout model.
The only new native input is a declared multiplayer options packet in ABI
scratch. Original instructions own pool construction, allocation cursors,
physical bounds, backing publication, callbacks and queue flags. No bank
snapshot, residency bit, loaded resource record or successful I/O outcome is
imported.

The reference produces1,075,354 ordered retained stores, including1,678 stores
after the prior complete startup prefix. It reserves7 MiB and creates one
pending model-resource entry. Admission modes0 and1, each repeated, retain the
same single pending entry and nonresident return. The fixture carries complete
final-bank hashes and the original instruction listing. Native comparison uses
real owned physical/static pages, two real savegame cache files and the existing
native worker, comparing source store sites, addresses, widths, before/after
bytes, return values and final banks.

All55 selected checks pass against the saved binaries, including the new
original/native resource comparison and the actual two-player Match binding.
The saved native binary's CLI check also passes. The actual fatal probe now
passes model admission and stops at the next unported owner `FB2FC → A8620`.
The [validation record](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/resource-validated-v1/validation.json)
retains the18 binary hashes,580 frozen project files, original repeats and
[actual Match log](../analysis/damage-shields-goal-20261011/live-fatal-cleanup/resource-validated-v1/match-camera.log).
This is a bounded source projection, not a complete binary/source bundle:
the record identifies the later shared datum-pool declaration and does not
claim whole-checkout stability during concurrent implementation.

The subsequent [session gate increment](SOURCE_FATAL_SESSION_GATE_DELIVERY.md)
connects complete `A8620` to that same fatal cleanup call path. Its actual
two-player Match probe passes the former resource-stage stop and now reaches
`CFB80 → BA980`. It does not change the resource residency or I/O boundary.

This increment does not close the broad damage-source census, shared
host/source vitality authority, natural-hit delivery, death/rebirth, model I/O
and fixups, or shield rendering/pixel comparison. The ordinary native Match
continues to use its existing host authority; this retained source-world path
is selected through the existing `--mp-source-object-world` integration option.
