# Armory PRT material boundary

The current native client naturally finishes The Heretic and loads The Armory,
then fails before its first Armory frame because a definition-residency caller
forces an authored PRT shader through the dynamic-only campaign model owner.
This is a missing campaign material/resource owner, not evidence that the MCC
shader asset should be replaced or that the draw can be omitted.

The actual failure is `panel_mech_tile_ext` under
`scenarios\objects\solo\spacestation\ss_exterior_gun\ss_exterior_gun_hinge\shaders`.
The natural transition, difficulty1 and zero script faults are recorded in
`analysis/campaign-native-new-game-frontend-20261028/cohort-v9/iteration-1/graphics-progression.json`.
That run rendered2,154 Heretic frames and subsequently returned1. It does not
qualify the New Game menu path, Armory control or a native-only engine.

## Reproduction and primary inputs

`tools/diagnose_campaign_armory_shader.py` reproduces the exact failure in
3.474seconds using the frozen client and the actual `01a_tutorial.map`:

```text
python3 tools/run_test.py --scratch-root .local/test-scratch/armory-shader-20261029 -- \
  python3 tools/diagnose_campaign_armory_shader.py --runtime PINNED_CLIENT \
  --runtime-sha256 dfca204c6ae5f090ba7d54a4858f3c8d3680bee1391a6ca8042b571cb1afd261 \
  --borrow-receipt analysis/campaign-native-new-game-frontend-20261028/cohort-v9/current-source-cohort.json \
  --mcc-root ACTUAL_MCC_HALO2_ROOT --report-output OWNED_SMALL_REPORT
```

This direct-map command is diagnosis only. The exact map is21,893,120bytes,
SHA256`f71460e42eb43ea1d74a67af2589e8bccea645f62d921a430f94e946ac4bf08e`.
`01b_spacestation.map` is Cairo and was not used for this reproduction.
The retained result is
`analysis/campaign-native-new-game-camera-20261028/armory-shader-direct-red-v2.json`.
No XBE/executable reference was opened by the client, the imported caches were
not written, and the client hash stayed unchanged. The earlier v1 diagnostic
failed at snapshot disk allocation under`/tmp`; it did not reproduce the shader.

The failure further reduces to one actual`Cache::load_material` followed by
`prepare_dynamic_material(cache, material, {}, true, 0.f)`, exactly matching
`SourceCampaignModel::prepare_materials`' arguments. Both independent executions
agree. The probe is`tools/diagnose_campaign_armory_material.cpp`; its bounded
receipts are`armory-material-minimal/report.json` and
`armory-material-owner/report.json` in the same analysis directory. They borrowed
the frozen owner's archives and headers read-only; their executables were owned
temporary files and have been removed.

The actual MCC tuple is:

| Input | Authored value |
| --- | --- |
| Machine definition | `E43102BB`, `ss_exterior_gun_hinge` |
| Render model | `E43302BD` |
| Shader | `E43402BE`, `panel_mech_tile_ext` |
| Template | `shaders\shader_templates\opaque\prt_simple` |
| Highest shader mask | `13C22`; layer5 exists, layer3 does not |
| Model regions/sections | Five singleton sections0..4, identical across the six geometry LOD collectors |
| PRT definition | order3, clusters10, PCA vectors4, one PRT LOD,50,052stream bytes |

The same named shader was independently decoded from the original cache with
the authenticated TU1.5 XBE, SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The LOD mask/range bounds and complete base/PRT vertex bytecode and pixel words
match the MCC material. Other pass constants/remap contents and final pixels
were not compared. The absence of a dynamic layer is therefore authored; adding
a default dynamic shader would change the input contract.

## Owning source and required closure

`src/app/main.cpp:1534` includes every content object's definition in the
secondary residency census; its unowned definitions are constructed and
prepared through`SourceCampaignModel` at`:1545`.
`src/campaign/source_campaign_model.cpp:90` unconditionally attempts dynamic
preparation and throws the observed guard at`:94`.
`src/rendering/world/world_material.cpp:631` correctly requires its selected
layer3 implementation2 or supported independent passes. The PRT-only shader
satisfies neither contract.

The existing PRT owner is substantive: `SourceObjectGeometry` loads each
selected geometry LOD and applies its actual PRT stream at
`src/gameplay/objects/source_object_geometry.cpp:55`, prepares section/LOD
coefficient banks at`:99`, and binds selected PRT materials at`:148`.
`prepare_prt_material` at`src/rendering/world/world_material.cpp:684` requires a
real c68 bank with42or82constants and the exact supported pass family. A shader
mask alone is not proof that this complete binder succeeds for the gun hinge.

`SourceCampaignModel` currently neither applies the PRT stream nor transports
its bank. Its ordinary draw API rejects lighting classes outside1/2 at
`src/campaign/source_campaign_model.cpp:183`. The secondary generic draw also
uses that API at`src/campaign/source_campaign_secondary_view_tu15.cpp:102`;
the application's specialized callback currently consumes weapon draws only
(`src/app/main.cpp:3734`). Simply excluding the machine from eager residency
would leave its ordinary/secondary consumers without a real owner.

`SourceMachineScene` supplies a reusable complete family interface:
`src/gameplay/objects/source_machine_scene.cpp:582` validates the actual full
owner/model/LOD and retains selected geometry;`:600` obtains the current
PRT coefficient bank and`:609` transports it with the draw. Its live source
reader at`src/rendering/models/source_live_model_geometry_tu15.cpp:316` checks
the current owner cache, region, node count, exact PRT dimensions and actual
arena extent. Campaign does not currently construct this machine scene.
Its retained view slot and frame lifetime must remain separate for the ordinary
view and a texture-camera view; reusing private view0 state is insufficient.

The concrete next work is a campaign PRT resource and selected-draw owner that
reuses these genuine stream/material/current-bank services and covers both
ordinary and secondary consumers. A resource-only preparer can be separated
from per-draw coefficient binding, but a successful resource preparation must
not authorize drawing without the actual current bank. No fake bank, shader
default, asset replacement, skipped draw or completed native PRT claim was
introduced by this diagnosis.

The independent source ownership review is
`analysis/campaign-native-new-game-camera-20261028/armory-material-native-ownership-review.json`.
All diagnostic scratch was removed, including the failed first invocation;
only bounded JSON/log receipts and the reusable diagnostic sources remain.
