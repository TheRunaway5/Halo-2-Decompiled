# Source-created dynamic objects in TU1.5 multiplayer

Source-created equipment, garbage and crates without scenario placements now
publish their actual full identities, definitions/parents, solved roots,
velocity, retained sphere, runtime/control flags, hidden/frozen state, current
model regions, looping-effect handles and current RGB channels. Their WORLD
matrices use the existing complete source pose bank. Protocol73 carries the
same state to clients; they allocate no second physical body or local birth.

The object roster reads the actual occupied2048-entry,12-byte header pool.
Reservations with no completed body remain with their original constructor.
Full salted lookup and header/body type/extent checks precede readback.

## Original field owners

Only the pinned English TU1.5 executable is used:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

* BC570/16FFE0 select body+B1's signed variant index.
* WholeBC6C0 resolves the signed118/11A descriptor: ten bytes per region,
  active permutation bytes, retained cached bytes and eight-byte state rows.
  State rows retain the chosen variant permutation, damage/property bytes and
  full looping-effect handle.1700E0 is the actual TU1.5 initializer reached
  byBC5C0; older model-helper labels are not used as executable addresses.
* WholeBCD80 reads the current half of the two RGB banks from124/126, with24
  bytes per channel. The native bridge preserves raw float bits and absent
  channels. It does not re-run color selection, clamp values or consume RNG.

`native_source_live_objects_tu15` checks full identity, reserved-header
exclusion, descriptor bounds, current versus initial RGB, raw signed-zero bits,
absent banks and rejection without writes. Its independent original-only
comparison executes unchangedBC6C0/BCD80:69 distinct instructions, two regions,
two colors and an out-of-range channel preserving its caller output. This is
field-reader evidence, not full physics/trajectory parity.

Authored prop mirrors also adopt the original current region/effect bank after
construction and each completed source world pass. This updates native query
geometry/presentation without selecting another permutation or converting the
authority into a network replica. Native initial-region admission for other
unresolved definitions still requires broader work.

## Renderer integration

Dynamic definitions prepare a resource union of actual authored sections
before GPU allocation. That preparation chooses no live variant/permutation;
active draws stay empty until a completed source selection arrives. Every
draw skins the actual WORLD hierarchy, shares the existing interpolation only
within the same full owner, and retains independent owner lighting/LOD history.
Current source flags and sphere supply the existing per-view query/LOD inputs.
Snapshot omission retires that presentation generation.

`SourceDynamicObjectScene` covers source equipment/type3, garbage/type4 and
crates/type11. Live acceptance below exercises dropped grenade equipment and a
power-module debris crate. Other categories retain their separately owned
player, corpse, vehicle, weapon and projectile consumers.

## Visible composed evidence

All files are under
`analysis/multiplayer/physics-live-tu15-20261012/`.

| Current-root run | Evidence |
| --- | --- |
| `live-root-dynamic-readback-v227/lan-objects` | Actual retained-world roster records the source-born power-module garbage crate and three dropped grenade equipment objects after the real blast. |
| `live-root-dynamic-debris-v228/lan-objects` | PASS: one debris owner moves2.026564 units;123 geometry submissions in each client view;182 exact authority comparisons and231 adopted dynamic-scene frames. Two actual point impulses compare all512 motion bytes against unchangedTU1.5. |
| `live-root-dynamic-regions-v230/lan-objects` | PASS after authored-region adoption: debris moves1.591729 units;174 submissions per client view;181 exact comparisons and321 dynamic-scene frames. The prop also retires/recreates through the actual source owners, with the surviving observer retained. Two impulses again compare all512 bytes exactly. |
| `live-root-dynamic-regression-v230/lan-vehicle` | PASS: Banshee entry, flight, turning and exit;902 exact comparisons,532 original seated-camera submissions and326 views after exit. Horizontal flight12.959475 units; climb4.625235 units. |
| `live-root-dynamic-regression-v230/lan-lift` | PASS:181 exact comparisons; maximum upward velocity3.964367. |
| `live-root-dynamic-regression-v230/lan-movement` | PASS:181 exact comparisons; walking, one jump,3.08 upward velocity and full crouch. |

The fourV230 cases retain unaltered, nonempty X11 recordings and verify one
stable native ELF throughout:
`d21e54c0d69105167503182a21420f4aa83076d3f679e29cdce49b9a3e0f4023`.
The current production target builds with these changes. The separately
selected launcher package remains unverified for this update.

## Remaining coverage

This is composed source creation, solved readback, presentation and UDP
evidence for the stated fixtures. It does not establish all dynamically
created definitions, garbage/type4 live cases, equipment pickup/lifetime
behaviors, attachments, all model-region transitions, exact original render
admission/scheduling or complete material/effect/lighting presentation. Other
vehicle/controller classes, all contact/constraint/activation modes, campaign,
all maps and quantitative retail trajectories remain required. Full TU1.5
physics parity is unfinished.
