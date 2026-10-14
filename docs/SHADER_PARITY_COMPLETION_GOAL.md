# Shader and effect completion goal

Authority: unchanged OG Xbox TU1.5 executable, original Xbox cache assets and
authenticated original captures. Vista is not a gameplay visual reference.
Original emulator evidence and physical Xbox evidence must remain identified
separately. No guessed values, synthetic replacement effects, relaxed pixel
thresholds or invisible reference draws may close a gate.

The active goal covers the full shader, texture and lighting scope requested on
2026-10-13, including unfinished world-impact/grenade work. This matrix tracks
that scope. A passing material admission or isolated shader fixture is not a
completed live rendering path.

| Work area | Current implementation | Required completion evidence | State |
| --- | --- | --- | --- |
| Explosion illumination | Live BSP/model command and range owner; owned main-view geometry/palette handoff and ordinary vehicle/BLOC join; original command/opaque/palette checks pass; actual original-effect grenade replay visibly lights vehicles and Lockout BLOC power modules; paired controls isolate2,257 vehicle pixels /6,892 BLOC pixels;1080p and four vehicle views verified | Mesa GL/Lavapipe rifle/module illumination now passes exact pixels with source-derived constant-W handling; resolve remaining NVIDIA strict pixel failures and finish ordinary non-vehicle model families with matching base geometry | In progress |
| Scarab turret PRT | Per-owner layer5 banks and all six source LODs; upper/rear turret GPU ownership checks pass in three campaign maps; TU1.5 valid/empty/invalid PRT coefficient tests pass | Connect actual retained incident for empty/invalid-bank live owners; finish all affected cache/LOD admission and matched rendered response | Open |
| Dynamic shadows | Vulkan Generate/Filter/Apply stages and bounded draw plans | Live original caster/receiver collection, priority/history/fade inputs and matched gameplay shadows | Open |
| Shields and camouflage | Base and selected overlay/binder paths exist | Live override/clone ownership, original shader functions and scene-copy/pass chronology, matched visible transitions | Open |
| Vehicle/character shader functions | Ordinary lighting and bounded damage feedback exist | Damage/camouflage/engine/weapon inputs; moving-body root/velocity/freeze ownership; original permutations, poses and held weapons | Open |
| Weather | Isolated snow/plate graphics and source simulation/depth modules | Rain graphics, live particle/plate activation and visibility, current depth/stencil/sky/clear owners and matched frames | Open |
| Weapon effects | Selected authored particles and impacts run | Remaining trails, beams, casings, blade/lens attachments, lighting inputs and original lifecycle/blend/depth submission | Open |
| Scene composition | Admitted map lighting, environment, water, transparency, fog and bloom paths; Cairo authored `stencil_env` environment admission repaired | Remaining transparent/occlusion sorting, patchy-fog branches, conditional clears and original render-pass scheduling | Open |
| Existing shader pixel differences | Retained first-person, vehicle and grenade comparisons | Normal filtering differs across GL/Vulkan host backends with identical mip bytes and fixed gradients; source-derived constant-W specialization closes both matched Mesa GL/Lavapipe model illumination draws exactly, while the NVIDIA module gate still fails2,616 pixels; resolve with original evidence, plus remaining depth/arithmetic/compositing differences; preserve exact acceptance gates | Open |
| Coverage and delivery | Bounded campaign/map/vehicle/resolution captures exist | Matched frames for every admitted authored family and requested map/entity scope; resolutions, resize and split screen; freshly built combined runtime and launcher checks | Open |

Work order: finish remaining actor/weapon/PRT explosion receivers and the
retained strict original filtering/arithmetic/compositing comparisons first;
then Scarab PRT, shadows and shield/camouflage inputs; weather and weapon effects;
scene composition; the complete matched-frame and installed-runtime sweep.
Each source fix must retain its failing reproduction, original evidence,
focused validation and resulting screenshot when it changes visible output.

The first implementation checkpoint is `TU15_SKINNED_LIGHT_VERTEX_20261013.md`:
819 authored Lockout/Cairo sections, original compression/declaration banks and
6,353,172 exact original-emitter/native GPU output words. This closes the bounded
raw input/vertex consumer gap, not live model illumination or the full goal.

The Cairo environment checkpoint is `TU15_CAMPAIGN_ENVIRONMENT_20261013.md`:
thirteen authored materials retain their original environment program across
twenty-six LODs. Native720p/1080p captures are available; matched original
campaign-frame validation remains open.

Current detailed evidence is maintained in `TU15_MODEL_LIGHT_RECEIVERS_20261013.md`,
`TU15_GRENADE_LIGHT_FRAMES_20261013.md`, `VEHICLE_SHADING_TU15_20261013.md`,
`CAMPAIGN_CHARACTER_SHADERS_20261013.md`, `WEATHER_RENDERING_PARITY.md`,
`WEAPON_EFFECTS_20261013.md`, and `MULTIPLAYER_SHADING_PARITY.md`.
