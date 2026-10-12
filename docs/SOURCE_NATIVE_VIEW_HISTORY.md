# Native world and first-person attachment history

The existing full-surface native W-depth path now retains its initialized
world depth/stencil through world accumulation, sky and first-person rendering.
First-person accumulation also loads the view's existing accumulation color.
This fixes a demonstrated native attachment-history error. It does not admit
the complete original conditional clear scheduler or establish original image
equality.

## Original authority and bounded change

The [original view/clear audit](SOURCE_VIEW_CLEAR_POLICY_AUDIT.md) pins retail
`2BA10` initialization before `13680/2BCD0`, shared depth/stencil across color
targets0/1/3/18, dedicated sky after world lighting/shadows, and the late/FP
override that suppresses layer1's color clear. Layer14 preserves its target.
The named sky and first-person phases do not authorize clearing world depth.

The native renderer previously used its color/depth/stencil CLEAR accumulation
pass again for first person and cleared depth around the sky/accumulation
phases. The corrected path adds two compatible passes on the same accumulation
framebuffer and combined D24S8 attachment:

| Native phase | Accumulation color | Shared depth/stencil |
| --- | --- | --- |
| World accumulation | CLEAR | LOAD |
| First-person accumulation | LOAD | LOAD |

Existing native full-frame and per-view initialization still precedes these
loads. Interior depth clears are removed only for the existing full-surface
view with atmosphere. Main sky rendering follows world/environment and
shadows, before fog. The earlier sky base pass remains because authored sky
environment materials can sample accumulation extern3. Each material retains
its authored raster/depth state. Legacy and clipped paths retain their prior
initialization; the two added pass handles are destroyed/reset on resize.
An otherwise empty global accumulation prefix is skipped for full-surface
scenes with no global background, dynamic or foreground draws.

`full_surface` and atmosphere select this existing native path. They are not
substitutes for original132D0 caller/header fields, transition or owner state.
No source sky stencil or weather producer is activated by this change.

## Reproducible evidence

The [private source and build record](../analysis/multiplayer-shading/native-view-history-retention/)
starts from the preceding coherent4991-file source and records exactly three
amendments: renderer, regression test and its five-line CMake registration.
The final4992-file source excludes separately recorded later shared-source
changes. The application SHA-256 is
`5820d3b9b17e28471d15b937a96aac7c853274a4d1dc6f82777f5f74d71bb324`.
`verify-source-build.py` rechecks both source inventories, the three amendments,
44 compiled artifacts, seven borrowed CPU fixtures and final-build readiness.

`native_view_history_tests` invokes the production Renderer with controlled
geometry/materials. It reads actual RGB, D24-low24 and S8 at640x480 one player,
853x479 two players,1281x721 three players and a return to640x480. It checks
world depth outside the foreground footprint, nonvacuous foreground and sky
coverage, world occlusion of sky, split-view isolation, fresh/reused storage
and repeated rendering. The same final test object linked with the archived
old Renderer fails the intended world-depth check. The corrected Renderer
passes all four layouts and preserves1,359,455 nonclear world-depth samples
outside the foreground footprint. An independent audit recomputes the output
comparisons and verifies that only renderer.cpp.o differs between the renderer
archives. The initial test version's sky assertion was vacuous because its
diagnostic pixel program supplied alpha0 to authored SRCALPHA blending; that
failed fixture, logs and binaries remain archived separately. The final test
uses nonzero diagnostic alpha and observes both visible and occluded sky.

The [launcher delivery](../analysis/multiplayer-shading/native-view-history-retention/launcher-delivery/README.md)
holds the actual all21-map1080p and1281x721 three-player checks, focused GPU/CPU
results and guarded installation evidence. Those captures prove native
execution and attachment/synchronization consistency, not original appearance.

The [supplemental native frame inspection](../analysis/multiplayer-shading/native-view-history-retention/renderdoc/native-history-proof/report.json)
directly observes FP accumulation color/depth/stencil LOAD in a captured
base-disc Zanzibar/SMG frame at640x480. The complete attachment readbacks are
byte-identical across FP begin. Two selected world pixels retain nonzero
target1 color and exact D24 with no FP events in their histories, while a
positive weapon control changes. This closes the accumulation-color
observability gap without inferring a complete geometry mask. The report
preserves RenderDoc's inconsistent positive-control depth-test history flag;
its actual readbacks and captured LOAD metadata provide the bounded evidence.
All observed S8 remains zero. No original framebuffer or physical Xbox
equivalence is claimed.

## Remaining original inputs

Original132D0 conditional initialization still needs actual view header+2/+7/+8,
transition4670CD, saved-view history and defined attachment coverage. Exact
2BCD0 layer gates, dedicated4C690 owner/raw-part/layer15 admission, nonzero
source stencil writers, weather producers and fractional shader timing remain
separate requirements. Native initialized-zero S8 is weaker evidence than
preserving nonzero original stencil history.

The retained base-disc Zanzibar camera/player comparison remains a partial
software-emulator diagnostic. The native integer capture tick does not pin
fractional shader time or restore original startup, simulation, HUD or clear
history. No all-map original framebuffer equality or physical Xbox claim is
made.
