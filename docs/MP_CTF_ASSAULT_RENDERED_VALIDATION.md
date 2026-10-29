# CTF/Assault rendered validation, 2026-10-28

The earlier System Link cohort selected the older `mcc-056309` cache. Its sky
model identity selects an authenticated original Xbox projection, but its
material and texture contents differ from that projection's original input.
The unchanged ordinary textured loader rejects it in 1.04 seconds. The same
compiled loader accepts the launcher-selected `mcc-sky-water-tu15-20261025-v7`
cache. Validation must select that prepared cache explicitly; the old failing
report remains historical evidence.

The new `native_mcc_ctf_assault_sky_projection_tu15` fixture independently loads
the original Xbox model named by the immutable sky profile, authenticates its
input identity, and compares the actual prepared model's complete geometry and
material semantics. Lockout passes with 654 vertices and seven materials,
including complete texture/mip identities. The original model input SHA-256 is
`242c1e63c4b589b68894aba0052e269a0a9b82667b0b41df7c837f42c00c661d`.
The sky projection guard and semantic digest remain unchanged. Rendered
objective consumers, goal rendering and end-presentation tests require this
fixture, and their build targets include it.

The full native build exposed Clang's default constant-evaluation step limit in
the 1,808-entry world-kernel catalog. Emitting length-aware `std::string_view`
literals fixes metadata construction. A 2,048-entry regression reproduces the
failure before the change and passes afterward under the same compiler budget.
All 1,190 compiled native shader stage identities, their source identities and
the registry identity remain identical. No runtime instruction decoder or
executable Xbox payload is introduced.

Evidence is under `build/ctf-assault-rendered-20261028`. `diagnosis.json`,
`sky-probe-caches-3.json`, `sky-authority-5`,
`native-programs-after-metadata-fix.json` and the effective-source receipts
record the diagnostic and source boundaries. The independently qualified 57
native objective rule owners remain unchanged.

The current build also reached an existing native combat/action owner,
`1E32A0`, through the first-person context's `1CF45C` call. That context did not
dispatch the combat/action family, so Single Bomb, Single Bomb Fast and Blast
Resort stopped during complete System Link matches. The dispatcher now calls
the existing complete native owner with the actual memory, ABI, floating-point
control and nested services. No result or readiness state is supplied. All nine
return/restart cases now pass, including Single Bomb's late arrival,
three-player completion, departure and next-preset cases.

The rendered matrix covers Multi Flag CTF, CTF Classic, 1 Flag CTF,
1 Flag CTF Fast, Multi Bomb, Single Bomb, Single Bomb Fast, Neutral Bomb and
Blast Resort. Startup and remote-consumer cases preserve each stock preset's
original 304-byte options. End-presentation cases explicitly select the original
three-second time limit and single-round setting, allowing the genuine source
clock to finish without supplying a score, phase or winner. They compare actual
automatic scoreboard titles, columns, relative widget geometry, RGB and viewport
fades against authenticated original Xbox controls. The strengthened readback
check observes the actual font atlas and submitted glyph quads for both local
views, then verifies opaque font ink in the opening and settled GAME OVER
captures. Image previews initially appeared to omit text, but direct pixel
counts in the earlier files confirm the labels were present in all nine cases.
The final cohort passes the direct ink check. This is a visibility regression,
not an original Xbox framebuffer comparison. Temporary GPU captures stay inside
owned scratch.

`final-nine-native-25` builds the complete native application and passes all
22 selected CTests, without skips, on an NVIDIA GeForce RTX 5080 under Xvfb.
Khronos validation is enabled with no validation messages. All nine presets pass
27 local startup frames, 1,080 remote frames with 17,250 waypoint vertices,
and 2,691 natural timed-ending frames. The ending cohort observes 6,822 actual
native world-kernel material uploads, zero uploaded executable instruction
words, and all 63,288 checked font-ink pixels. Goal-shader visible/transparent,
opacity and animation controls also pass. The original option/audio, native
multiplayer/snapshot and shader-lowering/catalog regressions pass in the same
cohort. `acceptance.json` and `acceptance-matrix.json` retain the exact scope,
source identities, reports and cleanup status.

`cleanup.json` verifies removal of the complete 4,147,248 allocated KiB private
source/build/capture owner. No new binaries, shader products, raw banks or frame
captures are retained. Exact compressed CTest logs, small reports and source/
executable identities remain; original assets and launcher-selected runtimes
were preserved.

The two additional all-map diagnostics are separate failures, recorded in
`broader-sky-diagnostics.json`: the campaign Introduction model differs at
material slot 1, and the derived-allocation relocation test requires a cache
after its declared transformation while V7 matches its authenticated before
input. Their guards remain strict and their failures are not relabeled as
passing all-map results. This qualification covers the nine stock presets on
Lockout; it does not establish physical controller routes, speaker waveforms,
Xbox/NV2A framebuffer equality, every edited-rule combination, cross-machine
play, migration or launcher installation.
