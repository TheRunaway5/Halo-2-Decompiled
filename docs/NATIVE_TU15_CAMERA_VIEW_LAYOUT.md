# Native TU1.5 camera view layout

`src/rendering/world/native_tu15_camera_view_layout.cpp` implements complete
call-free originals `2F5E0` (28 sites), `2F620` (43) and `2F690` (102). The
algorithms use native integer values, wrapping DWORD/WORD arithmetic and
checked signed division. They do not execute source instructions, reconstruct
dead registers/private stack bytes, or supply a geometry/gameplay result.

The grid helper retains the signed count and the original mode1 tie rule.
It advances the actual balanced grid until its wrapping signed product covers
the count, and publishes columns before rows. Its typed grid value API shares
the same algorithm. Original termination behavior is retained without a new
iteration cap.

Tile selection publishes spanY=1 before spanX=1, then reads signed grid
dimensions and lazy count/mode arguments. Its optional ordinal adjustment and
both coordinate divisions retain signed quotient/remainder, DWORD wrapping
and the source's fresh width read after the first coordinate publication.
Divide-by-zero and INT_MIN/-1 reach a typed error containing the actual
dividend, divisor and original divide site; earlier stores remain committed.

Rectangle layout reads the four actual DWORD owners in order4922AE,4922B6,
4922AA,4922B2. It computes cell dimensions from the signed safe-area spans,
spills only owed WORD values, publishes outer then inset rectangle DWORDs,
and lazily revisits actual tile/span/grid inputs for edge decisions. Outer
edges restore the original window bounds; internal inset edges adjust by4.
Aliased output/input storage retains publication and read order. The private
packed DWORD high halves that never affect public WORD results are absent.

The existing mutable16B owner at4922AA is declared by
`src/rendering/renderer/native_tu15_renderer_request_data.cpp:20`; no data mount,
initializer or renderer permission changed here.

The binding admits only actual229A90 calls229B44/229B5F/229B7D, with real RET
word consumption and plainRET/RET12 ownership. All three helpers are void.
Their callers overwrite volatile results before use; nonvolatile registers,
opaque dead fields and all floating-point state are retained. The component
does not need a floating-point policy because the originals contain no FP/SIMD
operations. Intermediate rectangles are native values, not a guest stack.

The actual inset outputs are ordinary side-effect-free retained packet RAM.
Six diagnostic cuts at the four ADD-WORD sites stop the source before an
indivisible read/write instruction; native read/write access exposes one
operand read before its rejected write. The physical reference independently
authenticates that pending operand's exact site, address, two-byte width,
instruction bytes and value. Only that single read is admitted in the test;
all other prefixes retain exact ordered reads/stores. This is a named caller
profile limit, not arbitrary MMIO or atomic write-fault equivalence.

Evidence under `analysis/campaign-native-new-game-camera-20261028/` includes
`view-layout-original-v2.json`, exact `view-layout-original-repeat-v2.json`,
`view-layout-validation-v3/validation.json`, and the caller/component receipts.
The775 physical cases cover all173 sites,59,048 original instructions,
9,912 reads,3,796 stores,120 fault prefixes,8 genuine IDIV traps and3 genuine
unmapped accesses. GCC1/1, Clang ASan/UBSan1/1 and Clang9/9 including eight
rejected actual-source mutations pass. RET-read failure preserves completed
data and leaves transport state uncommitted. The first compiler rejection
(test JSON conversion) and first exact-read RMW-cut rejection are retained as
bounded diagnostics. The v2 reference adds pending-operand provenance only;
all completed cases and computations are unchanged.

`cmake/NativeTu15CameraViewLayout.cmake` adds only the isolated component.
Shared World routing, producer/generator retirement, a coherent MCC/GPU run
and whole native camera-parent/New Game closure remain separate gates.
