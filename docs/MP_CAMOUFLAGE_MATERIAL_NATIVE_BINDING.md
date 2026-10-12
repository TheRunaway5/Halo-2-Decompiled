# Equipped-weapon camouflage material binding

The ordinary native multiplayer renderer now resolves scalar 37 from each
current equipped weapon's attachment to its current biped and binds it before
shader animation. This connects the recovered first-biped context kernel to
the live world/first-person material banks. It does not implement the separate
biped camouflage clone or admit a previously gated variant.
[Native owner binding](../src/multiplayer_runtime.cpp),
[original owner/parent evidence](MP_CAMOUFLAGE_OVERLAY_SOURCE_RESEARCH.md)

## Ownership and material order

The native projection checks the complete initial unit/weapon identity and
uses the registered hand's full weapon identity for first-person banks. Its
parent service represents the native equipped weapon's actual attachment to
the current unit. It does not claim to reconstruct the original complete
object-header pool. The resolver stops at the first biped and clamps that
biped's primary fade; it does not consume secondary fade or the lighting
system's ultimate-root lookup. Captured reference-player rendering keeps its
separately supplied context.
[First-biped kernel](../src/source_camouflage_context.cpp),
[original scalar proof](MP_CAMOUFLAGE_RENDER_SOURCE_RESEARCH.md)

`bind_object_camouflage` preserves other live program inputs and updates the
selected pass's retained base as well as its current pixel bank. Ordinary
animation then runs over that base, preserving original `19CA0` before `17420`
ordering even when an animated mask overlaps camouflage. Shared child passes
are detached before mutation so owners cannot overwrite each other. The
native frame binds each equipped world/FP bank before its ordinary animation;
the authored body/hands census contains no scalar 37 mapping and receives no
invented shader input.
[Material binder](../src/world_material.cpp),
[all-map mapping/mask inventory](../analysis/multiplayer/game-variants/camouflage-render-context-20261011/animation-masks.json)

## Verification

The unchanged retail `19CA0` and whole `17420`/`176A0`/`1B580` owners produce 1,280
declared material compositions. Cases cover fragmented and unfragmented
programs, both constant banks, filtered permutations, direct/inverse scalar
operations, and overlapping/disjoint animation masks. The native comparison
checks all 60 words before/after animation, retained bases, repeated frames,
and shared-child isolation. Missing/late-binding negative controls detect the
incorrect results. A fresh original run reproduces fixture SHA-256
`dc5ef3c8f4dad0d9f715fb1899558fb62450d0dbd9c2a7b59060e6e800dd2a89`.
[Original composition tool](../tools/oracle_camouflage_material_animation.py),
[native comparisons](../tests/camouflage_material_tests.cpp),
[frozen verification manifest](../analysis/multiplayer/game-variants/camouflage-material-native-20261011/verification.json)

The frozen complete build passes 12 focused CTests, including ordinary rendered
multiplayer, LAN startup, King/camouflage/damage arithmetic and native gameplay,
retail content, props and grenade delivery. A separate corrected GPU fixture
passes against those unchanged production libraries with Vulkan validation
enabled. Two independent copies of the actual Lockout carbine-display material
retain their compiled vertex/pixel programs, textures, raster state and
animation on declared front-facing test surfaces. Camouflage changes 60,214
components per owner at 1280x480 and 135,475 at 1920x720. Unchanged owners and
reset captures agree within one byte per component. This proves material-bank
delivery and isolation, not the full posed gun or Xbox framebuffer effect.
[GPU fixture](../tests/camouflage_material_gpu_tests.cpp),
[GPU output](../analysis/multiplayer/game-variants/camouflage-material-native-20261011/verify-20261011T225019.485728Z/gpu-required.log)

The initial configured GPU fixture lacked a useful visible response from its
posed carbine capture. Its failed logs/captures remain in prior-attempts.
The corrected test source/object/binary are separately frozen and linked
against the unchanged complete production build. The old configured GPU test
is excluded from the successful 12-test CTest run; the manifest records the
separate successful GPU command and all ten captures. This is 12 configured
checks plus one corrected GPU check, not 13 passes of the unchanged old suite.
ASan/UBSan and leak detection pass for the changed production material owner
and complete original-comparison test; remaining libraries are built normally.
The runtime-owner binding is exercised by the ordinary rendered test, but a
positive live equipped-weapon camouflage match is not established by the
isolated GPU fixture.
The manifest separately records concurrent changes in the shared live checkout;
the complete-build claims apply to its pinned source and test-only overlay.
[Commands, source/assets/binaries and scope](../analysis/multiplayer/game-variants/camouflage-material-native-20261011/verification.json)

Reproduce from a fresh source freeze with
`.tools/python/bin/python tools/verify_camouflage_material.py`.
The saved run used `--resume-run` and `--gpu-test-source` to preserve the
immutable production build while correcting only the test fixture.

## Remaining requirements

The separate source collector clone, global override shader, original scene
copy, layer 18/19 submission, depth/color alias, camouflage shadow consumers,
authored pickups and intrinsic suit control remain unfinished. Original pass
resources and their exact scheduling are now documented separately; scalar 37
delivery cannot substitute for those effects. Complete ordinary local/network
match flows and all editable rules remain required by the active 39-variant goal.
[Pass/target research](MP_CAMOUFLAGE_PASS_SOURCE_RESEARCH.md),
[operational acceptance](GAME_MODE_PROGRESS.md)
