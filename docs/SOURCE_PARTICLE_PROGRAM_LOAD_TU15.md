# Original particle program loading and native packet consumption

The particle module now has55 authenticated whole TU1.5 bodies and4,205
instructions. The added `1C2C0`, `1C570`, `400D40` and `400C50` follow the
already-owned `247D70` call at `247F42`. The generator verifies this containing
caller and each subsequent lookup/load/chunk-copy call. All51 preceding body
site sets remain unchanged.

`1C2C0` selects the actual vertex-tag classification and instruction bytes.
`1C570` maintains its program cache and avoids a repeated upload when the
selection is unchanged. `400D40` copies the instructions into the non-pure
CPU program cache and emits slot0 plus chunked program packets through
`400C50`. These bodies are lowered in full; no SDK or game return is supplied.

`SourceParticleGraphicsBindingTu15::load_program` admits a declared native
raw-program caller using the actual original material bytes and slot0. The
whole original loader owns its CPU cache writes and emitted instructions.
Before realizing the resident pipeline, the native consumer validates every
program packet against that exact code. The actual class7 Backwash program
has51 instructions and emits852 bytes in seven instruction chunks plus the
load method. Unknown slots, malformed methods, excess/truncated chunks and
changed instruction bytes are rejected. Unwritten CPU cache words remain
unavailable to source readers. This is native cache storage, not an asserted
original SDK constructor or boot history.

The mixed command consumer retains constants before and after the validated
program range. It verifies the complete stream before publishing the192-register
bank. Six malformed packet cases leave that bank unchanged. A syntactically
valid but uncompiled program cannot authorize pending packets after native
pipeline realization fails. Repeated loading, foreign or unissued controls,
unowned cache readers/writers and stale packet ranges remain guarded.

Four complete original CPU calls pass: actual vertex-tag lookup, first cached
load, repeated cached load and raw upload backed by a real renderer. The lookup
uses exact fragments from the captured source-created Backwash world. Its CPU
caller/cache/device fixture inputs are declared; original resource/boot history
is not inferred. GP/SSE, defined flags, x87/MXCSR, ordered paths and all retained
bytes match unchanged original i386. The raw call retains136,340 bytes and the
other calls137,524 each. The final raw-call memory result is captured before
native pipeline/header publication. The latest failure-guard build reuses this
original execution only after every fresh input/output byte, ABI and path is
verified identical for all four cases.

The actual seven-draw GPU test selects code through original `1C2C0` in the
captured world and loads it through `400D40`. Program and192-register constant
packets feed the same production resident pipeline. Direct, packet and repeated
images are exact, including the preceding four-image checkpoint, with12 changed
RGB pixels against cleared depth and zero Vulkan validation errors. These are
unchanged source particles; ordinary world-depth visibility is not claimed.
The complete queued parent still stops before the per-view control cache at
`3FF611`. Its534 completed callbacks/532 hardware steps match original i386 over
all60,436,574 retained bytes after the actual program was loaded. Its956 emitted
bytes include852 program bytes and104 parent bytes. No GPU/game return is supplied.

The eight constant-storage regressions also pass. Fresh normal CMake
configuration registers the new program-load test with the GPU lock and bundled
validation environment. Seven current-source objects compile successfully,
including both current renderer implementations and the expanded original
module, with no source/header drift during compilation. This verifies ordinary
build membership and object compilation; it is not a whole application link,
launcher installation or live all-map acceptance. The GPU proof uses a pinned
coherent earlier backend with the exact new methods overlaid.

Evidence and source/object/build pins are under
`analysis/map-world-features/particle-program-load-20261016/`. The earlier
[resident program stage](SOURCE_PARTICLE_PROGRAM_BINDING_TU15.md) retains its
own accepted scope. Both normal launchers select the preserved shared runtime;
this continuation has not replaced it. The accepted7501144f binary and59 shader
bank are preserved. Actual viewport/device flags, per-view initialization before
queue construction, current declaration transitions and full parent/packet
consumption remain required before combined launcher delivery. Full21 Xbox plus2
Vista maps, networking/latejoin/reset, original scheduling/resource/RNG histories,
native MCC entry and Xbox framebuffer acceptance remain active.

The subsequent [declaration/header continuation](SOURCE_PARTICLE_DECLARATION_TU15.md)
adds whole declaration/cache/type owners and same-kind native header/start
consumption, with41 original CPU comparisons and exact seven-draw GPU images.
Viewport and complete stream/parent integration remain open.
