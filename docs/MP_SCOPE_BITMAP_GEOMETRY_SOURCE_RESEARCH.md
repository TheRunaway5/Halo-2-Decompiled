# Original binocular bitmap CPU geometry

The actual Lockout binocular mask uses authored bitmap flags **decimal28 = hex0x1C**. Its original CPU producer stretches one selected sprite and emits the base, horizontal mirror, vertical mirror and combined mirror, in that order. Rendering one ordinary sprite quad cannot reproduce this authored mask. The new isolated helper implements these actual source operations; root owns live HUD connection and final frame verification.

This checkpoint does not modify shaders, the HUD/runtime, or any earlier source freeze. It preserves the separate [scope HUD status/model-request checkpoint](MP_UNIT_ZOOM_HUD_SOURCE_RESEARCH.md) and [camera checkpoint](MP_UNIT_SCOPE_CAMERA_SOURCE_RESEARCH.md).

## Owned source and correspondence

| Item | Owned original evidence |
| --- | --- |
| Base executable | `assets/disc/default.xbe`, SHA256 `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| Disc1.1 executable | `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| Ownedu5 executable | imported `Halo 2 DLC/content/u5English/default.xbe`, SHA256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |
| Actual input cache | `assets/disc/maps/lockout.map`, SHA256 `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9` |
| Whole bitmap producer | base `22AFF1` →1.1 `22D0F1` →u5 `2319F1`, all560 original instructions/internal branch targets match after address correspondence |
| Stretch operand | base `45DC00` →1.1 `465070` →u5 `46A110`, bytes `00002041`, float10.0 |
| Mirror operand | base `45DC0C` →1.1 `46507C` →u5 `46A11C`, bytes `00000040`, float2.0 |

`tools/research_mp_scope_bitmap_updates.py` locates entries through unique actual instruction prefixes, then retains whole original bodies, paired direct calls, branch-table target indices, image memory operands and changed nonbranch immediates. There are29 source routine pairs per update; all complete body/internal branch shapes match. Relocated writable globals and indirect callback targets remain their own initialization/context boundaries. Serialized initial bytes and annotations do not establish those runtime producers. The measured execution below uses the base image; no title-update execution is claimed by the static mapping.

`22AA16` is a separate text producer. The actual bitmap producer here is `22AFF1`; the binocular flag word is `1c00`, not `2800`. This widget has no0x20 flag or nine-slice branch.

## Actual authored widget

`ui\hud\masterchief` / `binocular_mask` is at cache file offset `01EB6AFC`:

* flags+1E: `1c00`;
* anchor4, layout sequences0/1/2, offsets `[0,0]` in all three layout slots;
* registration fractions `[1,1.25]` in all three slots;
* bitmap `E9D50850`, `ui\hud\bitmaps\new_hud\scope_masks\binoculars_human`;
* shader `E2950120`, `ui\hud\shaders\scope_masks`;
* special type0 and no widget effects.

The measured input suite also retains the actual SMG `weapon_background_right` and `crosshair`, each special0/flags0. There are120 untouched authored input cases: three widgets, video flags0/1/8/9, user counts1..4 and every original local view. Forty are actual binocular inputs. A further32 cases explicitly replace only the transplanted binocular flag word with0..31; these branch controls are labeled and are not authored asset claims. Together, the original CPU emits312 requests for152 cases.

## Source execution and excluded consumer

`tools/oracle_mp_scope_bitmap_geometry.py` inherits the unchanged `Retail` fixture and pointer transplantation from `tools/oracle_multiplayer_hud_positions.py`. Actual widget, bitmap, image, sequence and sprite bytes are unchanged except declared relocation of child-block pointers. The older explicit NTSC/video flags, user-count/session/projection/appearance and zero HUD inputs remain supplied fixture boundaries; this experiment does not create a genuine local multiplayer session or run its visibility/status producer.

The original bitmap owner executes through its normal return. The older oracle stop at `22B299` is removed in this new tool so the actual UV flips, stretch and mirror requests execute. Captures are:

| Original site | Captured source input/output |
| --- | --- |
| `22B1B5` | frame+48 anchor8, frame+0 horizontal axis8, frame+30 vertical axis8, frame+3C registration UV point8, frame+38 texture-width4, frame+8 texture-height4, frame+54 sprite bounds16 |
| `22B299` | original initial four positions at frame+0C,32 bytes |
| `22A664` | original draw request: ECX four positions32, EAX sprite UV bounds16, stack+4 bitmap, +8 image, +0C shader |

The graphics consumer is excluded at the genuine **void** request boundary. Literal original `RET0Ch` at `22A858` is independently checked (`c20c00`); the tool advances ESP/EIP at that ABI and does not supply an EAX result, bitmap readiness, shader/device success or GPU completion. The original owner’s subsequent position/UV operations use its own retained state. The four normal binocular request return sites are `22B695`, `22B701`, `22B754`, `22B7D3`.

Original CPU fixture control words are FPCW027F and MXCSR1F80. Listing bytes are verified against the corresponding owned executable; the independent source runs and static comparisons are byte-identical. This establishes CPU request geometry, not hardware clipping, texture sampler behavior, shader results or final Xbox pixels.

## Native scalar contract

`include/halo2/hud_bitmap_geometry.hpp` / `src/hud_bitmap_geometry.cpp` expose `source_hud_bitmap_geometry`. Its explicit inputs are the selected source sprite bounds `[left,right,top,bottom]`, bitmap dimensions, authored registration fractions, actual prior anchor/effect axes and flag word. Its explicit constants come from the two original operands above. Selection, effects, visibility, special-type shading and final graphics consumption are caller-owned.

For the regular special0 path, each multiply/add/subtract has separate float32 rounding:

1. Registration UVx = `(right-left)*registration.x + left`; UVy = `(bottom-top)*registration.y + top`.
2. Local extents are `-floor((UVx-left)*width)`, `ceil((right-UVx)*width)`, `-floor((UVy-top)*height)`, `ceil((bottom-UVy)*height)`.
3. Each vertex is `horizontal_axis*x + vertical_axis*y + anchor`, in original order **top-left, top-right, bottom-right, bottom-left**.
4. Flags1/2 swap the respective UV bounds after the initial positions are constructed.
5. Flag10 stretches the left vertices by subtracting `(point1.x-point0.x)*10`, top vertices by subtracting `(point2.y-point1.y)*10`, and expands left/top UV bounds by subtracting their spans times10.
6. Emit this base quad. Flag4 adds horizontal extent times2 to vertices0/3, then emits a copy. Flag8 adds vertical extent times2 to vertices0/1 from the base, then emits a copy. With both flags, apply both changes to a fresh base copy and emit the fourth quad.

Every mirrored request retains the expanded UV bounds and original vertex indices. There is no per-mirror UV flip. The positions are deliberately oversized; a caller must preserve its actual view clipping rather than substitute a single fullscreen sprite or clamp the source quad vertices. The helper returns up to four ordered position/UV requests and no graphics success result. Other special-type geometry branches are not supported by this bounded regular path.

The native tests use exported original outputs, never native-computed expected geometry. Strict compilation requires no fast-math and no float contraction. Both optimized and ASan/UBSan builds match all152 cases/312 ordered requests bit-exactly, including all32 flag combinations.

## Reproduction and frozen checkpoint

Canonical directory: `analysis/multiplayer/integration/scope-bitmap-geometry`.

* `measured-complete/report.json` and `repeat-complete/report.json`: SHA256 `e9f68b1060107e6a2b593c70d82a300e9ef6eb858fbcf6ede0ec2b3b0961e7a1`.
* `measured-complete/fixtures.tsv`: SHA256 `4d55f44088fa7019e1993b47e2ef3d2c4930e62f7a59797814748c5417dfe336`.
* `update-measured/report.json` and `update-repeat/report.json`: SHA256 `2b8ea44707742b268e71d9bf12dc05f2e93c16e8a0e035e06e86f8956f3d835e`.

Use fresh output paths for reexecution, then `tools/export_mp_scope_bitmap_fixtures.py` for captured word fixtures. `tools/audit_mp_scope_bitmap_geometry.py` verifies exact listing bytes, independent repeats, authored widget flag words, request counts/order and preserved prior checkpoint pins. `tools/freeze_mp_scope_bitmap_geometry.py` runs optimized/sanitized fixture consumers and writes the new immutable manifest. Root owns live integration and rendered verification; this checkpoint does not claim those steps have occurred.
