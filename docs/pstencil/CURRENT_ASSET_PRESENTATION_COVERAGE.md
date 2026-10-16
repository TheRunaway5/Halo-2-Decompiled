# Current asset presentation inventory

This is a resident metadata inventory of all 28 supplied primary current maps. It does not certify live shader/part/LOD admission, framebuffer parity, or complete gameplay coverage. Current assets are preserved; exact 2003 path matches identify references to investigate, not values to copy blindly.

| Current map | Object definitions | Resident material definitions |
| --- | ---: | ---: |
| 00a_introduction.map | 79 | 288 |
| 01a_tutorial.map | 103 | 221 |
| 01b_spacestation.map | 266 | 579 |
| 03a_oldmombasa.map | 259 | 567 |
| 03b_newmombasa.map | 287 | 648 |
| 04a_gasgiant.map | 138 | 435 |
| 04b_floodlab.map | 153 | 398 |
| 05a_deltaapproach.map | 235 | 543 |
| 05b_deltatowers.map | 168 | 475 |
| 06a_sentinelwalls.map | 207 | 519 |
| 06b_floodzone.map | 235 | 445 |
| 07a_highcharity.map | 173 | 504 |
| 07b_forerunnership.map | 184 | 470 |
| 08a_deltacliffs.map | 163 | 396 |
| 08b_deltacontrol.map | 218 | 555 |
| ascension.map | 142 | 279 |
| beavercreek.map | 150 | 300 |
| burial_mounds.map | 143 | 267 |
| coagulation.map | 148 | 296 |
| colossus.map | 145 | 288 |
| cyclotron.map | 161 | 324 |
| foundation.map | 144 | 291 |
| headlong.map | 162 | 361 |
| highplains2.map | 171 | 317 |
| lockout.map | 147 | 292 |
| midship.map | 156 | 300 |
| waterworks.map | 165 | 291 |
| zanzibar.map | 199 | 342 |

Across the caches: **1249 unique object definitions** in 12 families and **2359 unique material paths**. The full audit retains 2652 loose 2003 files and 537 shader/template user links with hashes and offsets.

The independent [caster resource audit](../../analysis/pstencil/caster-resource-audit/summary.json)
also visits **32,735 resident raw sections across all 28 maps**, including all
BSP clusters, instance definitions and render-model sections. All indexed
caster sections successfully bind the current position program and execute its
world-position prefix. There are zero resource/declaration errors. The literal
part totals are 8 type 0, 0 type 1, 71,631 type 2, 5,941 type 3, 13,467 type 4,
and 435 type 5, counting repeated resident assets in each cache. No type-1
shadow-only part occurs in these caches. The tool uses explicitly synthetic
identity palettes and a model owner token to check stored stream/index bounds;
this is resource coverage, not live pose, visibility or 2003 admission proof.
Live poses and independent casters without color draws have separate v4 capture
evidence.

| Literal path relationship | Count | Meaning |
| --- | ---: | --- |
| Current objects with an exact loose 2003 path | 90 | A comparison candidate; template/value/geometry equivalence remains unproved. |
| Current objects without an exact path match | 1101 | Preserve current authored roles and adapt supported stages; no historical alias is inferred. |
| Current shaders with an exact loose 2003 path | 214 | A comparison candidate; template/value/geometry equivalence remains unproved. |
| Current shaders without an exact path match | 2145 | Preserve current authored roles and adapt supported stages; no historical alias is inferred. |

The [complete evidence](../../analysis/pstencil/full-presentation-inventory/evidence.json) lists every object→model→render-model→material link, template LOD/layer/pass metadata, current bitmap dependencies, literal brightness bytes/selectors, source hashes, and 2003 path matches. The current decoder reads resident metadata only. A material declaring a layer is not proof that its live implementation has been admitted.

The additional [resident 2003 cache inventory](../../analysis/pstencil/2003-cache-inventory/summary.json)
reads 71 supplied version-6 maps and retains 4,543 unique group/path identities.
Two other supplied map tables remain unresolved at the salted datum check and
are listed explicitly. The original 00037CA0/00038120 functions establish the
32-byte tag-instance ABI. [Resident path comparisons](../../analysis/pstencil/2003-cache-inventory/current-path-matches.json)
find 81 exact current object group/path matches and 177 exact current shader
paths. These counts differ from loose-tag matches: resident presence is an
additional reference, never a substitute for selected shader values or a frame
trace. The 11 matching vehicle definitions include Warthog, Ghost, Banshee,
Wraith, Scorpion, Pelican, Phantom, Creep, both turret families and gravity
throne. Later equipment keeps its own authored material roles.

Live-v7 additionally renders selected root bodies for current Warthog, Banshee
and later Spectre, using actual native netgame factories, collectors and palettes.
Independent point-caster controls only darken their dynamic receivers (7,652 /
2,121 / 3,150 pixels respectively); authored specular controls only brighten
output, and all three restore exact Original pixels. These declared camera/light
fixtures do not cover every LOD, damage state, child turret owner, or historical
frame. [Controls](../../analysis/pstencil/live-v7/asset-final-controls/verification.json).

Use the [visual-identity audit](VISUAL_IDENTITY_AUDIT.md) for grounded vehicle, weapon, armor and prop examples, and the [caster/face audit](CASTER_FACE_PARITY.md) for exact predicates and unresolved cross-version gates. Full 2003 response/accumulation/environment execution remains an implementation and rendered-reference task.

Reproduce without changing any reference or game asset:

```sh
python3 tools/pstencil/audit_visual_identity.py --full \
  --output analysis/pstencil/full-presentation-inventory-new/evidence.json
```
