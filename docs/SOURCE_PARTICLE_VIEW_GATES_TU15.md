# Original view-pass selection and finish owners

The particle kernel now includes whole TU1.5 `289C0`, `15C030` and `13610`,
authenticated through their actual calls from `2BCB0`. The module contains
70 original bodies and 5,992 instruction sites. All preceding correspondence
is retained; 66 earlier lowered bodies remain exact, and the only changes to
`2BCB0` are its three authenticated calls entering these new owned bodies.

Nine direct `15C030` cases cover every one of its 32 instruction sites. They
exercise zero/nonzero selection, the absent index, fractional boundaries,
NaN, absent/present retained references and the final referenced word. The
predicate leaves the pass serial and queue counts unchanged. The source
field values are declared CPU inputs, not reconstructed original startup or
simulation history.

Eight complete `2BCB0` early-exit cases now run the original packet producer,
complete `13430`, actual `289C0`/`15C030` selection, complete `13610` mode-zero
finish and the caller's six-argument return. They cover both initial serial17
and unsigned wrap, the two original early-exit choices, and `289C0`'s actual
retained-state cleanup. No game/GPU return is supplied and no instruction is
patched. These selected paths do not reach `40E80` or construct a draw queue.

All 17 calls match unchanged TU1.5 execution in general/SSE registers, defined
flags, x87 state, MXCSR, complete instruction paths and all 144,773 retained
bytes per case. The current-test export also matches every input/output bank
and state/path report from that original comparison. The only test-source
change between those exports is a comment naming raw original fields.

Measured coverage is 32/436 sites in `2BCB0`, 39/432 in `289C0`, 32/32 in
`15C030` and 6/18 in `13610`. Whole-body ownership therefore does not imply
acceptance of every rendering branch. Mode-nonzero finish still requires its
actual target services; non-null `289C0` paths require additional original
render owners and state. Their calls remain explicit required owners.

The original CPU fixtures demonstrate the initializer's reset instructions.
They do not establish consumption or retirement of a live preceding queue.
The native binding's empty-queue guard is unchanged; this stage does not relax
it or erase a live queue to force admission.

The captured seven-draw GPU regression and separate 77-packet arena-growth
stress pass with this kernel. Five images remain byte-exact to the preceding
view-pass stage, with five positive RGB pixels on cleared depth and no Vulkan
validation errors. All 192,520 compared bytes of the retained RNG and four
complete particle pools remain exact. This retains the native prepared-view
route; it does not submit the complete original parent or prove ordinary
world-depth visibility.

Normal CMake registers the new CPU test, and both affected normal-source
objects compile. Private checks retain the pinned coherent renderer backend
and source overlays from the preceding stage. The combined current application
has not been linked or installed by this stage. Both launchers retain the
shared `1fe89481` runtime and report native MCC gameplay unavailable. The
accepted `7501144f` world runtime and 59 shaders remain intact.

Evidence is retained under
`analysis/map-world-features/particle-view-gates-20261018/`. The next original
rendering path reaches `40E80`, followed by the actual collection/render owners.
Its resource and queue lifecycle must be established before connecting the
prepared view to live World collection. Current shader, matrix, vertex-stream
and GPU-method ownership, foreground ordering, complete map behavior,
network/latejoin/reset acceptance, normal-launcher delivery and Xbox visual
acceptance remain open. The all-map goal stays active.
