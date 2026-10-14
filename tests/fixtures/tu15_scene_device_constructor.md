# TU1.5 scene device constructor fixture

`tu15_scene_device_constructor.original.json` is generated locally from the
owned English OG Xbox TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It is not a Vista reference or a captured gameplay framebuffer.

`tools/oracle_scene_device_constructor_tu15.py` runs unchanged original
instructions over declared object-pool, stack, tag-table and portal-bank inputs.
No instruction or callee is replaced. Its initial 160 cases cover DEVI birth `10B5C0`,
MACH birth `117870`, fresh pose-change `10C0B0`, portal callback `118120` and
collision-type getter `A9BE0`, reached by the same source construction graph.
Each case retains the input banks, complete GP/XMM/flags, full resulting body
bytes, exact instruction sequence and all ordered writes with before/after bytes.
All 161 birth/portal/type CFG instructions and 46 of 63 pose-change instructions
execute across the cases. The live animation-channel call branches are explicit
remaining validation, and retain their real external owner in production.

The earlier fixture contains 344 cases: the original 160 plus 184 unchanged
group allocation/resolution, scenario-group, CTRL/LIFI post-birth and fresh
channel lookup cases. They compare 23,026 instructions and 5,133 ordered stores.
The reached callback/support generator now retains 6,728 complete original
instructions, including additional device update and Havok bodies. These
fixtures do not claim coverage of all live animation/Havok branches. Production
has no opcode decoder or original executable execution. The
original DEVI descriptor at `4747A8` contains birth callback `10B5C0` at `4747D4`.
The MACH descriptor inherits DEVI through `4748F8` and contains its own birth
callback `117870` at `47489C`. Original dispatch sites and bytes are authenticated
by the generator. The type getter is the complete six-byte original body
`B8 13 00 00 00 C3` (MOV EAX,19; RET), with full ABI comparison; no collision
calculation or other work is removed from that callback. The original executable,
generation manifests and complete
case evidence remain local under `analysis/vehicles/shading-tu15-20261013/`.

The 2026-10-14 fixture adds 56 unchanged original cases for the BSP machine-bank
getter `202AB0` and collision-reference publisher `2184A0`. It covers every one
of their 73 instructions, including absent banks, both supported object types,
invalid indices, disabled entries and multi-reference publication. The complete
400-case fixture compares 25,678 instructions and 5,529 ordered stores.

Cairo tram creation reached these bodies and exposed the missing `505E78`
storage owner. Native startup now invokes complete `1E7D90` immediately after
`1D0250`, preserving the actual `1CFAB4` startup call. Its retained allocator,
pool constructor and complete `140C80`/`140CF0` storage helpers supply that bank;
no fabricated pointer or successful service result is substituted. The fresh
Cairo tram constructor succeeds, and six subsequent cull queries match original
execution, including 45 WORLD-node reads. The 400-case fixture does not claim
complete allocator/startup branch coverage or a rendered Xbox frame.
New evidence is under `analysis/shader-parity-20261013/scene-machine-bank-*`
and `live-model-cairo-tram-cull-v3`.

Generate with the repository's oracle Python environment, then build and run
`original_tu15_scene_device_constructor`. Missing owned input returns a skipped
test; it does not count as parity evidence.
