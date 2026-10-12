# Authored multiplayer explosive props and gravity lifts

The local assets contain the reported objects and their collision, damage and
phantom definitions. This research has not executed their entire world
activation, Havok contact or explosion delivery path. The native port must not
substitute a common blast radius, a distance-only lift trigger, or an upward
impulse for these unresolved producers.

The inventory tool is [inspect_mp_sandbox_source.py](../tools/inspect_mp_sandbox_source.py).
It reads all 12 original multiplayer maps and the nine owned Map Pack maps,
retaining exact raw records, original addresses, file offsets, placements and
tag identities. [asset-inventory.json](../analysis/multiplayer/sandbox-source/asset-inventory.json)
contains 30 relevant `bloc`/`mach` definitions. The Assembly schemas are used as
navigation; instruction consumers below establish only the particular fields
they actually read. All prior research freezes remain unchanged.

## Directly applicable asset facts

| Map and definition | Scenario placements | Model / collision / physics | Authored damage or phantom records |
|---|---:|---|---|
| Lockout `power_module`, `E6F80583` | 14 crates | `E6F90584` / `E7080593` / `E7090594` | Damage-info body `+28` = 30, shield `+8C` = 0; section 1 response 0 delay `+2C` = `3E4CCCCD` (0.2), damage ref `E72B05B6` |
| Lockout `lockout_lift_tube`, `E6ED0578` | 1 crate | `E6EE0579` / NONE / `E6F3057E` | Two phantom shapes and rules; rule 0 flags 4, axis `+28/+2C` = 10/10 and direction `+30/+34` = 10/4; rule 1 flags 0, direction 10/4 |
| Colossus `gas_container`, `E7BD0648` | 10 crates | `E7BE0649` / `E7C90654` / `E7CA0655` | No `hlmt+60` damage-info record |
| Colossus `gas_container_full`, `E7F4067F` | 6 crates | `E7F50680` / same collision / same physics | Body 20, shield 0; section 0 response 0 flags `00100081`, zero delay, damage ref `E80E0699` |
| Colossus `grav_colossus`, `E3E90274` | 4 machines | `E3EA0275` / `E3ED0278` / `E3EE0279` | Phantom flags `0802FFE4`, axis 0.5/10, direction 1/10 |
| Colossus `col_power_lift_high`, `E7E50670` | 1 crate | `E7E60671` / `E7E80673` / `E7E90674` | Phantom flags 0, direction 100/9 |
| Foundation `fusion_coil`, `E61F04AA` | 9 crates | `E62004AB` / `E62B04B6` / `E62C04B7` | Body 15; distinct death, model-state and delayed destruction responses |

The Lockout tube and several other authored lift models have **no `coll` tag**.
Their `phmo` phantom shapes are present. A system importing only model collision
tags cannot establish these lift contacts. Conversely, the explosive props
have genuine collision-model references; rendering their geometry alone does
not instantiate an original object or collision owner.

The Lockout damage tag is named
`objects\gear\human\military\damage_effects\fusion_coil_explosion`, despite
the crate name `power_module`. Its raw floats are radius bounds `+0/+4`
`3F4CCCCD/40000000` (0.8/2), core `+18` = 0.5, lower `+1C` = 60,
upper `+20/+24` = 80/80 and acceleration `+40` = 4.
The full gas-container response instead references
`objects\gear\damage_effects\object_large_damage`: radius 1/3, lower 50,
upper 150/150 and acceleration 0. These are authored inputs, **not** a proof
that area damage is a sphere with uniform damage or that the response has
already been delivered. The area owner must preserve attenuation, obstruction,
materials, ownership and actual targets.

The raw inventory also retains collider tag identities, complete relevant
`hlmt`, `phmo`, response and effect records, scenario transforms and flags.
Unplaced tags remain explicitly unplaced, including Colossus's resident
`cov_power_lift` and low lift. Tag residency is not placement authority.

## Original object and lift ownership

The 1.1 type definition at `46F620` contains name pointer `45A42C` (`crate`),
group word `626C6F63` (`bloc`), object size `130` and callbacks:

| Type slot | Original 1.1 address | Base address and role |
|---|---|---|
| `+2C` | `11D1C0` | `11BBF0`, create |
| `+40` | `11D2B0` | `11BCE0`, update |
| `+4C` | `11D330` | `11BD60`, shared machine/crate callback |

The exact original listings are retained under
[lift-scalar/measured/instructions](../analysis/multiplayer/sandbox-source/lift-scalar/measured/instructions).
`11D1C0` clears object `+12C`. Definition `+BC` bit 0 sets its bit 1; otherwise
it follows definition `+38` → model `+24` physics tag and inspects every rigid
body at `phmo+3C`, stride `90`, **motion type `+1E`**. All types 1/2 (or zero
rigid bodies) also set that bit. Creation returns the actual `20E370` model
query result with a positive rigid-body count. This is an owning dependency,
not permission to fabricate the query or a Havok handle.

`11D2B0` reads object `+C0` bit 12 and `+B4` component datum. With the bit set
and datum different from NONE, it looks up the real `A0`-byte Havok component
in `[526248]+44` and calls `1D5E90(component, 1.0)`. It also writes the source
game tick into object `+BC` when object `+C1` bit 0 is clear. `11D330` uses
the actual component and `1D6F40`; its complete callback role is unresolved.

`1D5E90` requires component flags `+4 & 00104000`, a nonzero supplied factor
outside the original near-zero threshold, and the actual contact array `+94`.
It follows the component object's definition/model/physics references.
The contact's signed shape byte indexes `phmo+44`, stride `C`; that record's
signed word `+8` indexes the **68-byte rule array at `phmo+2C`**. The same
array is named "Materials" by the schema; the original consumer proves the
`+8` rule-index use. The rule itself is not a contact-volume primitive.

Contact admission/removal are owned by `1D5830` / `1D5A60`. They read genuine
Havok entity property pairs (`entity+30`, count `+34`), keys `2001` and `2002`,
then resolve original component/object identities. `1D56F0` applies the
authored lower/upper size gates, attached/nonattached flags, dead-biped gate,
and `1 << (object_type + 5)` ignore mask. Admission uses the source allocator
at `487638`, creates the `C`-byte contact array and 8-byte entries, and counts
matching shape/body/kind/object links. Removal decrements the count and swaps
the last entry when it reaches zero; optional bit `01000000` owns biped `+3E4`
localization. No such records, membership or flags are supplied by this suite.

The force consumer obtains marker transforms through real `B9C20`/`BB050`
and real body position/velocity through `1D44C0`/`1D43C0`. It uses center,
axis, direction and orbit channels independently. Direction's scalar target
is the actual immediate `461C4000` (10000), not an invented destination.
Original flag bits 25/26 enter linear/angular handling. Its complete vector,
gravity-compensation, contact, body-write and world-step composition remains
unexecuted here. Thus the next live gate is the source-produced phantom shape
and real overlapping player's body/contact, followed by this owning update.

## Independently executed scalar slice

[oracle_mp_lift_scalar_1_1.py](../tools/oracle_mp_lift_scalar_1_1.py) executes
unchanged original `1D5C70` in two fresh Unicorn machines. The repeated reports
are byte-identical: 512 cases across actually placed lift rules in six maps,
89 executed instruction addresses. Authored axis/direction acceleration,
maximum velocity, exponent, dead radius and flags are read from owned maps.
Projected velocity, axis distance, mass argument 1 and game step `3D088889`
(1/30) are explicit scalar inputs. This is **not** a fabricated original
contact or object constructor. All tested rules have scale-by-mass bit 1,
exponent and force-acceleration bit 25 clear.

ABI: `BL` is allow-reverse (real consumer sets 1); stack words are
scale-by-mass, mass, target, acceleration, current projected velocity,
velocity limit, position/dead radius, exponent, force-acceleration, ratio
output pointer. It returns scalar velocity delta in low `XMM0`, writes the
ratio float, and `RET 28` restores 44 bytes including the return address.
The original epsilon at `4650E0` is bits `3A83126F` (approximately 0.001).

The operation order is visible in the pinned 134-instruction listing. It
optionally computes a power before scaling acceleration by inverse mass;
then multiplies by source delta time, subtracts position from target, and
separately handles positive and nonpositive acceleration. The positive
non-forced branch compares against braking distance and the current step,
or computes x87 square root of twice step×remaining distance. It applies
allow-reverse/current-velocity bounds, derives absolute velocity/limit ratio,
and finally clamps delta to ±absolute step. Preserve separate SSE operations
and the original x87 square-root operation. No cases claim the unexecuted
positive-exponent `pow` path or full Xbox arithmetic/hardware bit parity.

The base helper `1D2280` has the corresponding 134-instruction structure;
the separately retained base listing is read-only mapping evidence, not an
additional executed baseline suite.

## Explosion response owning path

Original `D7160` resolves object definition `+38` and model `+60/+64`, proving
the damage-info block. Original `DC0A0` walks 38-byte sections and their
50-byte instant responses; updates the object region-state byte/word fields;
checks threshold, rider/capability, total-damage threshold, special-death
flags and skip probability; and either dispatches `DB710` or records a
pending response. The source pending word stores index in low 3 bits and
rounded frame count in upper bits, with delay read from response `+2C` and
original signed-short tick rate `[518444]+2`. It does not permit replacing
0.2 seconds with a guessed host timer.

Original `D73E0` owns pending update: decrements upper bits by one tick,
preserves low 3 response-index bits, rechecks `DB570`, and calls `DB710` when
the countdown reaches zero. `DB710` sets the response's destroyed bit,
dispatches actual model-state change, `DB360` visual effect and **`DB470`
damage effect**, then applies destruction/child/constraint/physics side
effects. Its source authority and options-mode gates are material.

`DB470` calls the actual marker query `B9C20`; a NONE/zero marker or successful
query permits a damage request. It invokes `D7C60` to initialize the genuine
damage record, copies the original owner and marker position/forward,
gets the root location via `BB1F0`, sets owning-object identity and source
strength, and calls area damage owner `D8280(data,NONE)`. This static source
inspection establishes why connecting only health depletion or a particle
effect is insufficient. It does **not** establish that these callbacks,
targets, area collisions or effect allocations have executed in a native
match. These owners and the real source-created crate lifecycle are the next
explosive integration gates.

Correspondence: base `D5B60`→`D7160`, `DAAA0`→`DC0A0`, `DA110`→`DB710`,
`D9E70`→`DB470`, `D6C80`→`D8280`. Names from the decompile aid navigation;
the original instruction operands and calls establish each contract.

## Local update version and latest-patch boundary

The separate [local-executable-inventory.json](../analysis/multiplayer/sandbox-source/local-executable-inventory.json)
inventories all 43 `XBEH` files under `assets/**/*.xbe*`, including duplicated
launcher extraction directories. The retail game code found is the base
executable and the Map Pack **1.1** update. Certificate version is correctly
read at certificate `+AC`: base `00000003`; eight update variants
`00000303..0000030A`. Every update section is identical across those variants.
These are certificate variants, **not** proof of gameplay versions 1.3–1.10.
Installer/download/movie/dashboard executables are not additional game title
updates, even where a title ID is shared.

The disc's English confirmation explicitly says 1.1; see the preserved
[Map Pack research](MULTIPLAYER_MAP_PACK_RESEARCH.md). Its update SHA is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
base SHA is `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The common build string is not final-update version authority.

Microsoft's [April 18, 2005 announcement](https://news.xbox.com/en-us/2005/04/18/halo-2-autoupdate/)
confirms a mandatory Halo 2 autoupdate. Its [July 20, 2005 announcement](https://news.xbox.com/en-us/2005/07/20/halo-2-autoupdate-is-here/)
confirms another later autoupdate. The [April 17, 2007 Blastacular announcement](https://news.xbox.com/en-us/2007/04/17/halo-2-blastacular-map-pack/)
confirms original-Xbox Tombstone and Desolation, which are not among the nine
locally extracted Map Pack maps. These primary pages do not establish the
exact final executable bytes, numeric final version or complete final tuning.
No later payload was downloaded. The current local implementation cannot be
described as applying the latest original-Xbox patch; closing that requirement
needs the user's owned later executable/content and verified version authority.

## Reproduction and validation

Run the inventory twice with different output paths, then run the scalar tool
twice into fresh measured/reexecution directories. The frozen auditor verifies
all inventory record bytes against the original maps, the original code bytes,
the repeated cases, ABI cleanup and the immutable manifest:

```sh
.tools/python/bin/python tools/audit_mp_sandbox_source.py
```

No runtime code, original asset or older frozen suite is changed by this work.
The manifest records `native_gameplay_update_applied:false`; no complete
explosion, collision, contact, lift, hardware or latest-patch parity is claimed.
