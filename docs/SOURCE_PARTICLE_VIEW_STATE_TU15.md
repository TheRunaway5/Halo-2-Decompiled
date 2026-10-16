# Original viewport, scissors and bounds restoration

Six more complete original TU1.5 bodies now provide the viewport setter
`4021E0`, scissor producer `402840`, viewport matrix owner `400920`, matrix
multiply `407160`, float-to-integer helper `406FE0` and bounds restorer `15700`.
The particle module has50 bodies and3,964 instructions, preserving all44 prior
body-site sets. No SDK/game return is supplied by these owners.

Entry authentication follows complete original callers. The already-owned
`13430` calls `14AA0`, whose `14B11` call selects `4021E0`. That setter calls
`402840`, `400920` and `406FE0`; `400920` calls `407160`. The original
`2BCB0` exit calls `13610`, whose `13654` call selects `15700`. The generator
retains and verifies these containing-body/call-site links. Old canonical
`12D50`, `156F0` and `40770` labels land inside TU1.5 instructions/bodies and
are excluded. Raw operand/caller candidates remain analysis data, not entry
permission. In particular, `403ED0` is a scale helper; it is not substituted
for the discovered six-word viewport setter.

`4021E0` bounds the supplied viewport against its surface extent and device
limits, stores the six viewport words, produces scissors and recalculates the
matrix/depth state. A null viewport retains its current rectangle while still
performing the dependent work. `15700` republishes the configured depth bounds
through `400270`. These owners supply the actual producer arithmetic needed
by the per-view initialization and particle depth paths.

Twenty-four complete calls pass in eight ordered CPU sequences: viewport,
`13430` reset and bounds restore. Inputs cover full and split rectangles,
oversized bounds, a minimum viewport, explicit clipping, offset flags,
non-W/W modes and a null viewport. Whole original `400A60` produces both FVF
header variants. The fixture's extent fragment is produced by the original
layout owner; no physical allocation, fabricated SDK return or actual GPU
current-shader/framebuffer history is supplied. The setter changes neither
queue count; the subsequent original initializer owns their reset.

All24 calls match unchanged original i386 across GP/SSE, defined flags,
x87/MXCSR, ordered paths and all143,949 retained bytes per case. This includes
the emitted commands and matrices. Oversized bounds clamp to the supplied
surface and null input preserves its retained rectangle. The independent
Backwash regression also passes240 actual source-world particle frames,
14 final draws and repeated-view particle/RNG invariance with the expanded
module. This regression uses the retained earlier world libraries and does
not replace the fresh-current-archive startup proof in the predecessor stage.

The live graphics binding still requires owners for its actual device
`1C18` flags and current shader/viewport/resource state. The new CPU producers
are not inserted into live views with assumed zero device fields or headers.
The prior real-renderer prefix remains bounded before those missing fields;
this stage adds no GPU visibility acceptance. The native command consumer and
full original particle callback must complete before a combined world-effect
launcher delivery is accepted. Boundary2 target/clear services remain separate
required owners.

Evidence is under
`analysis/map-world-features/particle-view-state-20261015/`, including the
50-body generator, complete original comparisons, Backwash regression and
`acceptance.json`/`continuation-state.json`. The preceding
[per-view initializer stage](SOURCE_PARTICLE_VIEW_INITIALIZER_TU15.md) retains
its fresh current-world and actual-renderer proof. Later shared runtime
deliveries are preserved. Full21 Xbox plus2 Vista map effects/interactions,
network/latejoin/reset, original scheduling/resource/RNG histories, native MCC
entry and Xbox framebuffer acceptance remain active.

The subsequent [resident program binding](SOURCE_PARTICLE_PROGRAM_BINDING_TU15.md)
connects actual original particle pipelines and programmable-kind consumers.
Its source viewport and complete current-program history remain bounded.

The [projection correction](SOURCE_PARTICLE_PROJECTION_TU15.md) supersedes the
earlier matrix-input fixture: `400920` reads `CA0` and writes `980`. Whole
`4020D0` now produces that input before32 ordered projection/view/reset/restore
calls. The retained earlier fixture had initialized the output and left its
actual input poisoned; its original comparisons apply only to those inputs.
