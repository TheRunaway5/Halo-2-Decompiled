# Original root composition and pose palettes

The ordinary object caller `BD090` constructs the actual root through `BDC40`,
converts local node0 through `1421F0`, then calls `142A60(root, local0, WORLD0)`.
Every remaining node uses its already rooted parent WORLD transform. The source
inverse-bind producer `1376C0` consumes this complete WORLD bank. Applying
placement after the local hierarchy, or after vertex skinning, changes floating
point operation order and does not reproduce this caller.

`BDC40` supports explicit actual parent, scale and mirrored state. An attachment
producer still has to supply those states; placement metadata does not prove
their live values. `BD090` can also admit physics node overrides, graph updates
and type-specific root callbacks. The later `BD610` is a deferred animation/unit
override producer, rather than an unconditional placement operation. This
correction does not replace or infer those branches.

Cold scenery initialization `10A1B0` writes object+138 to NONE before its later
pose. The class callback `10A390`, dispatched by `1090D0`, applies its additional
node0 transform only when that field is non-NONE and `BADC0` resolves it.
Machine and prop descriptors have no corresponding root callback. The observed
automatic Slayer cold factory prefixes confirm +138 is NONE for all278 scenery
owners. Unit/weapon `112DC0` has its own extension+28 guard and remains separate.

The full untouched cold prefixes stop at `B8890` before registration. Their742
owners comprise278 SCEN,49 MACH and415 BLOC;741 have a render model. Actual
locals can reside in the allocated object bank or source scratch `4DC2F0`, and
the observation retains the correct bank in either case. The741 render owners
produce1,032 complete WORLD nodes and12,384 original `1376C0` palette words.
The production rooted locals, bind and direct-frame APIs compare bit-exactly
against these original outputs. A separate CPU application check verifies that
400,350 native vertices consume these recorded palette matrices and preserve
their UV/material/section/part identities. Those vertex checks are not original
GPU exports or physical hardware comparisons.

The ordinary first-person path has two different root operations. At `167D9D`,
the weapon definition+308 translation is converted by `1416C0` and passed through
`1CBD50` to `1DD1C0`. It is applied at graph node0 before descendants. Later
`165F16` combines the camera and optional inverse camera-control adjustment;
`165E9F` left-composes that resulting transform separately with every completed
node. Moving the camera before the hierarchy would also change the source
arithmetic. The mirrored `1CBD80 -> 1DD290` path changes a selected local node
and its parent bank, and is not covered by the ordinary-root API.

The independent ordinary FP oracle executes `1416C0`, complete `1CBD50/1DD1C0`,
original `141590/142A60` and complete `165E9F/1376C0`. Its graph, local poses,
offset and camera are explicit inputs. Literal mode-local controls do not claim
that their declared diagnostic graph topology is the original animation graph.
The proof validates transform ordering without inventing animation phase,
controller/attachment state, mirrored behavior, registration order or IK.
It passes643 cases /5,790 nodes, comparing220,020 actual original pre-camera,
post-camera and palette words. Diagnostic native vertex application controls
add138,960 words, for358,980 compared words total. Both native FP meshes consume
the same resulting palette, with empty explicitly supplied IK constraints.

All403 supplied model skeletons in the bind fixture have exactly one
parent-NONE node, node0. The generic native API applies a supplied root at every
declared root; `1DD1C0` itself starts its traversal at node0, so disconnected
diagnostic animation graphs do not have source-equivalence evidence.

The local quaternion reciprocal convention and physical Pentium III seed
boundary remain those documented in `source-model-pose.md`. The cold fixture's
explicit allocation and completed-animation-resource services do not establish
later physics, scheduling, asynchronous residency, source RNG chronology or
frame equality. No live original process is read or modified by these oracles.

Evidence is retained in `model-root-cold-original.json`,
`model-root-fp-original.json`, the frozen `retail_source_model_root_assets.tsv`
and `retail_source_model_root_fp.tsv` fixtures with adjacent provenance, and
`tools/differential_source_model_root.py`. The cold observer is
`analysis/multiplayer-shading/model-root-cold-original.py`.
