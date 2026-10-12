# Original view and layer clear policy

Ordinary `2BA10` view construction sets the render-view signed short at+8 to2.
`132D0` therefore requests a clear for that caller, but the aspects come from
the actual byte4670CD: zero selects color, depth1 and stencil0; nonzero selects
color0 only. Color-target switches do not imply new stencil storage. Original
targets0,1,3 and18 share depth/stencil surface509364.

This is a CPU-only source audit and planner specification. It implements no
live clear scheduler, changes no renderer and performs no GPU or guest work.
The [reproducible report](../analysis/multiplayer-shading/view-clear-policy/report.json)
and [literal disassembly](../analysis/multiplayer-shading/view-clear-policy/source-disassembly.txt)
pin the retail executable, readable source, all25 layer rows and two immutable
original Lockout entry snapshots. The executable SHA256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The expanded tool verifies 57 unchanged inputs, 42 original function blocks,
38 literal phase anchors, 13 direct global references, 9 transition/view
call sites and 17 known direct clear call sites. This is static evidence;
it is not a whole-caller oracle. The clear inventory includes literal
preceding instructions and does not infer indirect calls or executed history.

## View construction and initialization

`132D0:132D7..132E3` copies the actual0x298-byte caller into485600. The signed
short at+2 selects main target18 only when its value is exactly1; every other
value selects target0. The routine disables stencil testing, calls the source
camera binder, then reads copied signed short+8. This is an attachment operation
separate from the stencil state reset.

| Actual input | Ordered effect at132D0 |
| --- | --- |
| short+8 equals0 | Bind the selected target with depth enabled; no14B60 clear. |
| short+8 nonzero, byte4670CD nonzero | Bind target, clear all color lanes to0. Preserve depth and stencil. |
| short+8 nonzero, byte4670CD equals0 | Pack caller RGB at+0C through131ED0, bind target, clear color/depth/stencil with depth float1 and stencil0. |

The literals at1334E/1335A are semantic flags1/61. `14B60` maps them to the
actual D3D/NV clear masksF0/F3. Semantic bit20 maps to depth bit1, bit40 to
stencil bit2, and bit10 to alpha bit80. Original SDK `3F9FC0:3FA314..3FA329`
uploads the flags to method1D94 unchanged. The independently pinned NV2A
register definitions name the same depth, stencil and RGBA bits. This does not
assert physical Xbox conversion precision.

The clear rectangle is the bound source viewport. The SDK clear with no
explicit rectangles starts from the current viewport at3FA1A0..3FA1F4,
intersects each supplied rectangle and then applies the actual device scale
fields before emitting inclusive hardware bounds. `14BC0` derives the
viewport from the actual render-view rectangle, view mode, target and source
scale inputs. A native planner must receive the resulting admitted physical
rectangle; it cannot replace it with an unconditional full-frame clear.

There is an additional color-only operation: if render-view byte+7 is nonzero
and current main target4858B8 is0, `14B60` first calls14AC0. That routine saves
the viewport, temporarily selects a literal640×480 viewport, clears color0
only and restores it. The following requested clear still uses the restored
viewport. Neither this operation nor stencil-disable erases stencil bytes.

The actual caller builders differ:

| Caller | Source clear short+8 |
| --- | --- |
| `2BA10:2BB93` ordinary view | Literal2, after zeroing the full caller record. |
| `2C560:2C653` | Literal0. |
| `2C790` |1 when caller DWORD+0 differs from1;0 otherwise. |
| `2C8A0:2C986` | Literal1. |
| `2CA00:2CA83` | Literal1. |
| `13C20` saved-view restore | Calls132D0 on the actual prior4850D0 stack record. Honors that record's current field and global state; it is not a guaranteed no-clear restore. |

Before restoring a prior view, `13C20` also issues SDK clear mask80 when
actual4BA04C is greater than1. That is an **alpha-only** clear; depth and
stencil remain intact. The source view stack index and caller records must be
preserved if this nested path is represented. Static caller construction alone
does not establish that a particular nested path executed in a capture.

## Producer of byte4670CD

The byte belongs to the original transition state at4670CC. `28980` sets both
bytes4670CC and4670CD to1 and records its actual caller input and clock.
The literal direct caller `2C5224:2C529C` supplies address509334 while setting
up the level-select screen. `289E0` reads the flag at entry, runs its original
transition work and clears byte4670CC at29008. At its completion branch
2902E it writes zero to the full flag DWORD. `28950` independently zeros that
state; `2C52AF:2C52E2` tail-calls it during the corresponding screen teardown.

The direct-reference inventory finds no other direct absolute writer in known
original function starts. It does not exclude pointer-alias writes or prove
runtime invocation history. The full transition arithmetic, graphics and clock
are not ported by this audit. Until that producer is represented, a live
planner needs the actual state byte as an explicit input rather than a default
derived from whether a sky or map is visible.

Two independently captured original Lockout snapshots corroborate a bounded
ordinary state: at36B50 and391F0 entry, byte4670CD is0, current main target is0,
short+8 is2, the source viewport shorts are `[0,0,480,640]`, and the entire
active saved0x298-byte view equals the current copy. Their depth pointer is
00406A4C. These are unchanged paused-entry banks from the private original
OpenGL/llvmpipe session. They do not establish original20 Zanzibar clear history
or the history before the observed entries.

## Layer target switches preserve shared depth and stencil

`14BC0` case0 at14BEB, case1 at14C41, case3 at14C27 and case18 at14C56 all load
the same509364 pointer. Its actual AL `use_depth` input controls whether that
pointer reaches1CD30; color-target selection alone never clears it.
`1CD30` selects the real target/depth pair and updates cached pointers. It has
no attachment-clear operation.

The literal43FA1C table and `44370` yield the following relevant operations:

| Layer | Target and clear after admission |
| --- | --- |
|1 | Target1; color0 only, if both485A75/485A76 override bytes are0. |
|14 | Target1; no clear. |
|19 | Target3; no clear. |
|6 and17 | Current main target; alpha0 only, if both override bytes are0. |
|15 | Current main target; no clear. Dedicated4C690 sky also has no attachment clear. |
|10,4BA014 bit4 set and bit5 clear | Target10; semantic1 colorFFFFFF only. |
|10, other bit4/bit5 state | Target9; semantic60 depth1/stencil0. |

Layer10's special zero-table-flags branch is independent of the two clear
override bytes. Target9 has its own original depth/resource alias path and
must not be collapsed into the ordinary shared509364 storage. The other
literal table rows and unresolved ninth DWORD remain in the report.

Layer admission first uses the actual collection layer mask and the2/8/17
feature bytes. A missing layer does not bind or clear a target. Ordinary layer1
also has an inlined2BCD0 path whose color-only clear follows the same override
gate; it is not an instruction to clear stencil when accumulating color.

## Main, sky and first-person order

Initialization precedes accumulation in the original ordinary caller.
`2BA10` calls `132D0` at `2BC1B`, then reaches `13680` at `2BC77` after the
actual caller and object-preparation gates. The normal branch of `13680`
supplies `(1, 1, caller_flags3, caller_flags4, 0, 0)` to `2BCD0` at `13788`.
Its alternative view and two-context branches are separate input profiles;
they must not be admitted by assigning ordinary defaults.

For the `2BCD0` first argument equal to 1, the source order is:

| Phase | Literal source sequence | Attachment effect |
| --- | --- | --- |
| Initial geometry | Layers 1, 3, 5, 8, 20 (`2BDB2` through `2C015`) | Layer 1 binds target 1 and may clear **color only**. The others retain the main/shared depth and stencil. Each actual layer-mask and feature gate still applies. |
| Lighting and shadows | `2C370(0)`, `335E0`, layer 7, `31C80`, `2C370(1)`, layer 2, `2C370(2..6)` | The ordinary shared attachment remains intact. Shadow layer 10 has the distinct target 9/10 profile described above. |
| Dedicated sky | Actual gate at `2C097..2C0B0`, stencil mode 2, `3EDE0` at `2C0BC`, then mode 0 | `4C690` uses the already populated depth and stencil. There is no attachment clear before or after this named sky phase. |
| Fog, late geometry and weather | `25D50`, layer 16, conditional layer 6 and `3C3C0`, `25DC0`/`2C340`, `C3E90`, `36B50`, then scheduled layer 15 at `2C173` | Layer 6 and `3C3C0` may clear **alpha only**. The scheduled layer 15 is distinct from the earlier dedicated sky. |
| Late object / first-person bracket | Set `485A76=1` at `2C1D9`; conditional `2AFA0`; `3FDF0(phase0)` at `2C2B3`; `404B0` at `2C2B9`; `3FDF0(phase1)` at `2C2C2`; optional `40870`; reset `485A76=0` at `2C2D3` | The override suppresses the late layer-1 color clear as well. All these named paths preserve the ordinary depth and stencil. |
| Final mask and postprocess | Conditional `2DD30`, conditional layer 17, `20770`, `13870` at `2C322` | Layer 17 may clear alpha only after the override has been reset. |

The first argument equal to 2 shares the initial inlined layers but excludes
the named sky/fog and later first-person/postprocess portions. Other admitted
first-argument values use the earlier `44370` sequence 1, 3, 5, 2 and skip the
inlined lighting segment. The sixth argument equal to 2 further suppresses
several sky, fog, mask and postprocess operations. These are actual source
branches, not equivalent ordinary-view aliases. The report retains their
original instructions; a live planner still needs the actual arguments.

`3FDF0` phase 0 builds an explicit filtered object collection, then attempts
layers 1, 3, 8, 20, 7 and 2. Phase 1 attempts layer 16. The actual EAX filter
is all bits for input 0, bit `0x10` for input 2 and bit `0x20` for other inputs;
live flags, nonzero owner admission, part availability and shader identities
also gate the collection. Its only clear, at `40066`, is semantic 1. Both
override bytes must be zero, so the enclosing `485A76=1` bracket suppresses it.

`404B0` consumes the actual `509440` owner count and each positive per-owner
count, constructs a separate admitted collection through `402B0`, and attempts
layers **11, 14, 12, then conditional 13**. Layer 14 binds target 1 without a
clear. The other admitted layers bind the current main target; all bind the
same ordinary shared depth through `14BC0` with AL equal to 1. The conditional
layer 13 depends on both bits 0 and 1 of the constructed `4BA014` context.
The count, owner identity, part filters and constructed context cannot be
inferred merely from a native foreground vertex list.

For flagged entries, `4D640` invokes `12D50` to change projection/depth-range
state. `12D50` has no attachment-clear call. Thus using its source first-person
depth range does not authorize erasing world depth or stencil. A blanket
first-person accumulation color clear is also unsupported by this profile:
the source layer-14 target is preserved, and the preceding layer-1 clear is
suppressed by the override.

## Clears outside ordinary layer 10

The remaining relevant named clear paths do not authorize the current native
pre-sky or foreground depth clears:

- `132D0` initializes the selected ordinary target from the actual view header
  and transition state. A saved-view restore through `13C20` may call it again
  with a different saved header; that nested history remains explicit.
- `13450` performs a direct `F3` color/depth/stencil clear only when its actual
  context DWORD at +4 equals 2. It selects target 18. `13680`'s two-context
  branch can produce this path, so the context cannot be omitted from the plan.
- `13C20`'s separate `80` clear is alpha only. Transition `289E0` clears are
  color only in the inventoried branches. `14AC0`'s temporary viewport clear
  is color only.
- `3C3C0:3C507..3C515` pushes color `0xFFFFFF`, depth 0 and stencil 0, but
  EAX is literal semantic `0x10`: **alpha only**, not depth. It requires the
  actual scenario record, caller gate and completed `12DE70` resource admission.
  Its optional `3BD00` offscreen sky/water operation also clears color only.
- The inlined layer-1 clears in `41CC0` and `423C0` are color only and retain
  their actual collection and override gates.

The inventory also retains SDK device initialization/reset callers that emit
depth/stencil mask 3. This audit does not treat those as per-view scene clears.
No complete indirect callback or SDK reset-history claim is made. The proved
absence of a clear is scoped to the named main/sky/first-person scheduling
paths and their explicit original attachment operations.

## Minimal safe production phase plan

Changing the accumulation pass to LOAD while leaving it before the first
initialization would read undefined native storage. The correction must move
initialization and preserve its attachment identity together:

1. Before the first admitted accumulation or main draw, perform the actual
   `132D0` bind/clear event on the source physical rectangle. A new native
   depth/stencil generation may be loaded only after an admitted source clear
   or a supplied, already initialized attachment history defines the read
   region. A source color-only/no-clear event cannot invent prior depth or
   stencil on a newly allocated or resized image.
2. Run admitted initial layer 1 on target 1 with shared depth/stencil LOAD and
   source color-clear policy. Finish its writes and make target 1 readable by
   the selected main pass. Continue main layers 3, 5, 8 and 20 using the same
   depth/stencil generation. Splitting native images requires a proved transfer
   of both aspects; sharing the existing native attachment avoids that extra
   equivalence claim.
3. Preserve this history through source lighting/shadow phases and submit
   dedicated sky afterward, with its actual mode-2 profile and owner/part
   admission. Remove no clear merely because a sky material is present; consume
   the explicit source phase profile. Do not retain a synthetic before/after
   sky depth clear in the admitted ordinary profile.
4. Preserve the target and depth/stencil through the late/first-person bracket.
   Reuse target 1 for admitted layer 14 without a blanket color or depth clear.
   Source first-person projection/depth-range adjustment is draw state, not
   initialization. Restore context/override state after the bracket.
5. Keep all source color-only and alpha-only clears as exact ordered events,
   preserving their unaffected aspects. Retain the separate target-9/10
   lifetime, nested-view events and alternate context-2 clear explicitly.

The smallest supported change can be limited to an explicit ordinary caller
profile. Native menu, diagnostic, clipped-view and unsupported alternate
profiles should retain their own declared initialization instead of being
silently assigned this schedule. Actual collection masks, source context,
owner/part admission and first-person counts remain inputs. This is a source
plan, not approval to activate missing sky/weather producers or a proof of
full frame timing and interpolation.

## Minimum live planner contract

The smallest source-backed planner can emit ordered **bind** and **clear**
events over caller-owned attachment identities. Its inputs are:

- The exact render-view header fields+2,+7,+8, packed clear color from131ED0,
  resolved source viewport, and actual transition-state byte4670CD.
- Actual target resources and their shared depth/stencil generation. Targets0,
 1,3,18 must refer to one represented history, or a proved equivalent transfer
  must preserve both aspects across independently allocated native images.
- The ordered source layer, actual collection mask/feature gates, both clear
  override bytes, bit4/bit5 of4BA014 and current main target.
- Actual nested-view push/pop events, saved records and4BA04C if13C20 is used.

An ordinary native view supplied through the proved2BA10 construction may
derive short+8=2; it still requires the genuine transition/caller mode and
rectangle producer. An admitted target switch without a clear event preserves
stencil. Per-layer color or alpha clears preserve it as well. Later sky writes,
mode resets, depth-only operations and weather reads retain their own ordered
profiles. A clear cannot be inferred from an empty stencil observation, a
material layer, a color-target name or a new native framebuffer.

Live source call/target history, transition clock and all-map sky submission
admission remain separate inputs. Current host-accumulator interpolation also
means a fixed native simulation tick does not pin fractional shader time;
retained captures with different sky RGB cannot establish a causal depth
resource change from tick equality alone. The source clear contract is exact
at the named branches; this document does not claim full-frame or physical
Xbox appearance parity.

Run `.tools/python/bin/python tools/audit_source_view_clear_policy.py` to
recreate this audit in its dedicated analysis directory. Prior sky/weather,
encoding and native capture artifacts are read-only inputs and are unchanged.
