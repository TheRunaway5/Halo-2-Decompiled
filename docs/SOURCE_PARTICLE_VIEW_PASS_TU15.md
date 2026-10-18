# Original particle view-pass packet

The particle kernel now owns the complete TU1.5 `2BCB0` body: 436 instruction
sites, with all 66 preceding bodies unchanged. The module contains 67 whole
bodies and 5,510 instruction sites. Authentication follows the existing
`13660` caller through its actual `13768` call, rather than treating a plausible
address as an entry.

The original prefix increments the word at `4C685C` and builds the three-word
`13430` packet from that serial, its sixth stack argument and the raw low word
of `XMM0`. This serial is a view-pass counter; a game-tick interpretation is
not established. The native value boundary preserves unsigned wrap and raw
floating-point payloads, including negative zero and NaNs.

`SourceParticleGraphicsBindingTu15::initialize_view_pass()` now obtains that
counter through the actual source-memory owner, writes the exact packet to
caller-owned storage, and runs complete `13430` against the prepared renderer
view. It validates the view and empty-queue admission before touching the
counter. The existing `initialize_view()` interface retains its behavior.
Neither interface reconstructs scheduling or prior-queue retirement.

Thirty native cases test five counter boundaries and six mode/distance pairs.
The lowered whole-entry prefix is observed at `13430` before executing its
first instruction. All 19 completed caller instructions, general/SSE registers,
defined flags, x87 state, MXCSR and 65,540 retained bytes per case match the
unchanged original TU1.5 code. No instruction patch or callee return is supplied.
This proves the packet prefix; it does not prove the complete `2BCB0` pass.

The renderer integration check consumes the generated packet and original
camera publication, verifies the actual counter and three retained words,
rejects reuse of a published resource before any reset instruction, and
transfers the published camera into the actual draw buffer. Its two complete
`12F80` calls match original execution and all 138,053 retained bytes per case.
The previous prepared-view API regression also passes.

Seven retained source draws pass the new packet route. Five GPU images remain
byte-exact to the preceding prepared-view check, with five positive RGB pixels
on cleared scene depth and no Vulkan validation errors. A separate 77-packet
allocation-growth check preserves camera transfer. It repeats captured draw
packets; it creates no simulation particle. The complete retained RNG and four
pool snapshots preserve all 192,520 compared bytes.

Normal CMake registers the two new tests, and six affected normal-source objects
compile. The coherent private diagnostics reuse the frozen accepted renderer
with the owned-method overlays recorded in their build receipts; they are not
a complete link of every current shared-tree source. A later unrelated network
header change is outside these objects' dependency lists. Both launchers still
select the shared `1fe89481` runtime, whose MCC gameplay status is unavailable.
This stage does not install an unverified combined application. The accepted
`7501144f` world runtime and 59 pinned shaders remain intact.

The actual World frame must still supply the original caller history and
consume/retire its preceding queues. The complete pass immediately calls
`289C0`, `15C030` and, on the drawing path, `40E80`; those services must run
through their own authenticated owners. The separate captured raw-parent probe
still stops at the unowned live `3FF611` cache read because it has not been
given the prepared context before collection. Current shader/matrix/vertex
stream ownership, original foreground ordering, ordinary world-depth
visibility, all 21 Xbox plus two Vista maps, authoritative networking,
late joins/resets, normal-launcher delivery and Xbox visual acceptance remain
required. The all-map goal stays active.

Evidence is retained in
`analysis/map-world-features/particle-view-pass-20261018/`, including the original
body listing, generated correspondence, native/original comparisons, GPU images,
normal-build objects and launcher-preservation receipts.
