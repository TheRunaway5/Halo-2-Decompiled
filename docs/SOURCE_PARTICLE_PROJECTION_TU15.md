# Original projection setter and corrected viewport inputs

The particle module now includes whole original `4020D0`, authenticated through
`405CF0 ->405B10 ->4020D0` and its containing call sites. It retains all63 prior
body-site sets and instruction sites, for64 bodies and4,780 unique instructions.
Original identity data, matrix dirty-mask entries and the three built-in
pretransformed microprograms are imported unchanged from the owned TU1.5 image.
No SDK/game return is supplied.

`4020D0` copies the selected matrix into its indexed device cache. Index1 owns
the projection at device+`CA0` and its projection-derived W fields at
`93C/940/944`; it also calls the existing whole viewport-matrix and program
fixup owners. The unchanged matrix multiply `407160` reads its second/third
stack arguments and writes its first. `400920` therefore reads projection `CA0`
and writes `980`.

This corrects the preceding viewport fixture, which initialized identity at
`980` while its actual `CA0` input remained the explicitly poisoned device-bank
bytes. Its historical CPU comparisons remain valid for those declared inputs;
they did not establish a valid identity-projection view. The revised fixture
runs the complete projection setter before viewport/reset/restore and checks
its real input/output direction. A rejected diagnostic also overlapped the
projection input with the complete280-byte FVF header; the new input is at a
separate workspace address. Both rejected candidates and their logs are retained.

Thirty-two complete calls pass in eight ordered projection, viewport, `13430`
reset and `15700` restore sequences. Declared inputs include identity,
perspective, nontrivial/degenerate matrices, default and non-W depth modes,
split/clamped/minimum/null viewports, and all three pretransformed fog-program
choices. Actual FVF headers are produced by `400A60`. No original SDK boot,
current GPU viewport, resource history or framebuffer is inferred from these
CPU fixture inputs.

All32 calls match unchanged original i386 across GP/SSE, every defined arithmetic
flag, x87/MXCSR, ordered paths and all144,617 retained bytes per case. The verifier
tracks flag definitions/undefined results through each actual original
instruction; AF is compared after defining arithmetic and excluded when left
undefined. There are8,075 native instruction callbacks. The independent original
comparison confirms that identity stores W values1 and1 and reciprocal1; these
fields are retained as produced values rather than asserted camera near/far
planes. Projection/viewport setup preserves queued counts, and the complete
initializer owns their later reset.

The constructor investigation also identifies `405B10`'s literal raster offsets
`3F080000` and `401950`'s initial viewport source `40DC68`, whose maximum extents
are clamped by the real surface owner. These are literal/caller evidence,
not execution of the complete constructor or permission to fill unknown live
fields. Device+`1C18`, current native viewport/resource initialization and the
complete per-view GPU interface remain required owners.

The existing original program/declaration/start/constant consumer also passes
its real seven-draw GPU regression with zero Vulkan validation errors. All four
images remain exactly equal to the preceding declaration stage, with12 changed
RGB pixels against cleared depth. This adds no ordinary world-depth visibility,
new live map acceptance or Xbox framebuffer claim. The queued parent still
stops at its unowned `3FF611` cache load. Current normal CMake configuration and
the generated-module/updated-test objects compile with no source/header drift;
a whole application link or installation is not claimed.

Evidence, source/object pins, original comparisons and rejected candidates are
under `analysis/map-world-features/particle-view-ownership-20261016/`. The
[declaration stage](SOURCE_PARTICLE_DECLARATION_TU15.md) retains its own accepted
scope. Both launchers continue selecting the preserved shared runtime. The
accepted7501144f world binary and59 shader bank remain exact. Full native view
initialization before queue construction, stream/parent/method consumption and
combined launcher delivery remain required. All21 Xbox plus2 Vista maps,
network/latejoin/reset, original resource/scheduling/RNG histories, native MCC
entry and Xbox framebuffer acceptance remain active requirements.
