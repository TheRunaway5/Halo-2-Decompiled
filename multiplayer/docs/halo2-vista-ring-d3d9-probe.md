# Vista ring D3D9 diagnostic boundary

The original Vista progress ring binds a cubemap to pixel sampler1 even though
its selected `ps_2_0` program declares that sampler2D. The original resource,
compiled shader and static parameters are pinned in
[the ring investigation](halo2-vista-matchmaking-roster-evidence.md). A successful
Linux D3D9 draw does not resolve the original Windows interpretation of this
mismatch.

The diagnostic corpus is `.local/vista-ring-d3d9-probe-20261012`. It uses the
unchanged792-byte original pixel shader, the exact submitted RGBA constants1..3,
and all mip levels of the actual bound Vista bitmap resources. Its64×64
pretransformed diagnostic quad directly supplies diffuse/specular and texture
coordinates. It does not execute the original vertex shader, frame owner,
fog producer, camera, geometry or animation. Cubemap faces are uploaded in
stored ordinal order; original directional orientation is not established by
this probe.

The runner uses Wine GE8-26 with the existing reference prefix, selecting either
DXVK2.5.3's native D3D9 or Wine's builtin wined3d through an explicit DLL override.
DLL-load logs identify the selected implementation. The DXVK run uses the
RTX5080 Vulkan device; wined3d uses Mesa llvmpipe. The prefix was not reset and
its wine server was not stopped. The retained input, source, executable and
runtime hashes accompany every report.

The first probe accepted all four resource bindings and returned success from
`ValidateDevice` and both draws in each case. The `polygons` material differed
from a sampler1-null control on both implementations. For
`progress_illuminated`, DXVK's actual binding differed from null while
wined3d's actual binding was identical to null. This first comparison is
insufficient: its initial bindings leave the mismatch implementation's
state-update path uncontrolled.

`clean-binding-order-controls` therefore tests two explicit orders. Before
each draw it sets the pixel shader and all four textures to null. It then
either sets the original shader followed by the textures, or binds the
textures followed by the original shader. Each order has separate actual and
sampler1-null controls, and all other stages are restored. The intermediate
`binding-order-controls` experiment retains already-bound initial textures;
it is preserved as a diagnostic, not used as the clean order control.

All eight clean executions and16 draws succeed. The pixel results are:

| Material | Implementation | Shader before textures: pixels differing from null | Textures before shader: pixels differing from null |
|---|---|---:|---:|
| polygons | DXVK2.5.3 |4096|4096|
| polygons | wined3d |4096|4096|
| progress_illuminated | DXVK2.5.3 |4096|0|
| progress_illuminated | wined3d |0|0|

For the illuminated null control, the implementations differ by one8-bit
level in red and green: DXVK BGRA(113,65,24,255), wined3d(113,64,23,255).
Thus even the constant diagnostic control is not a bit-exact cross-renderer
framebuffer oracle.

DXVK2.5.3's [pinned device source](https://github.com/doitsujin/dxvk/blob/v2.5.3/src/d3d9/d3d9_device.cpp#L6342-L6401)
explains an order-sensitive implementation difference. Its shader update sets
the mismatching sampler bit directly. Its texture update first shifts the
sampler index into a bit and then shifts by that bit again. For sampler1 this
marks sampler2. The downloaded source SHA256 is
`6e6f480dbe78dbd84fb1e6caf6200de834d647788f75bf3e5d0658d29ad34e7f`.
The clean controls are consistent with this implementation defect; they do
not establish the original Windows driver's behavior. Microsoft's
[sampler declaration documentation](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dcl-samplertype---ps)
distinguishes2D and cube coordinates but does not supply an original Halo2
framebuffer reference for a mismatched resource.

The separately traced Vista material driver ordinarily calls
669DD0→66632E→device+1AC `SetPixelShader` before
669E63→6679D9→667AF2→65F600→66EBC7→device+104 `SetTexture`.
The shader call may be skipped when state is already current; the earlier
669F2D reset clears stages0..7. See the separate frame-owner investigation
for the original blocks and their execution boundaries. This makes the
DXVK-specific difference relevant to Wine diagnosis, without turning it into
Vista material authority.

The native material implementation must not infer cubemap sampling or a2D
face reinterpretation from the first DXVK image. Original Windows reference
evidence for this resource/declaration mismatch and the live UI fog constant
lifetime remain unresolved. The decoder and geometry fixtures, shared idle
animation and native widescreen captures establish their individual contracts;
they do not establish exact Vista material rendering or activate the staged
native launcher.
