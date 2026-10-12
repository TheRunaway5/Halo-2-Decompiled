# Original Xbox Halo 2: collision, actors, combat, scripts and sound

Verified 2026-10-07. This records retail cache layouts, recovered routines and
read-only local asset checks. It distinguishes data extraction from reproducing
the original simulation. Cache/tag addressing and resource pointers are
described in [RESEARCH.md](RESEARCH.md).

## Primary sources and scope

| Reference | Pinned revision | Local ignored checkout |
| --- | --- | --- |
| [Matching Xbox recovery](https://github.com/kirklandsig/halo2-decompiled/tree/5f08630655e3ddc9ac45909090f11ae799fa0c27) | `5f08630655e3ddc9ac45909090f11ae799fa0c27` | `tools/reference/halo2-decompiled` |
| [Native recovery](https://github.com/BirchWoodGod/halo2-decomp/tree/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888) | `080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888` | `tools/reference/halo2-native-reference` |
| [Assembly](https://github.com/XboxChaos/Assembly/tree/fd14ef3c46135e9422966c6a64cd926fb0bd23fb) | `fd14ef3c46135e9422966c6a64cd926fb0bd23fb` | `tools/reference/assembly` |
| [Reclaimer](https://github.com/Gravemind2401/Reclaimer/tree/36224d7a4581f2a1f255099bf706aa56be9f6e21) | `36224d7a4581f2a1f255099bf706aa56be9f6e21` | `tools/reference/reclaimer` |

The local `assets/disc/default.xbe` SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`,
matching both recovery targets. Numeric offsets below are hexadecimal and
refer to little-endian original Xbox cache records. Ordinary tagblocks have
32-bit count and pointer; tag references have group at `+0`, datum at `+4`.
Translate metadata virtual pointers with the correct ordinary or BSP magic.

Assembly and Reclaimer declare GPL-3.0. Both recovery repositories declare
CC0 for their own work; vendored dependencies keep their own licenses. This
document describes formats and observed behavior; it copies no implementation
bodies. [License/provenance details](RESEARCH.md#pinned-primary-sources).

## Native reference: what actually builds

BirchWoodGod's pinned source contains 389 recovered routines built as a C11
shared library and diagnostic host. The implementation is mainly engine
containers, allocators, random/CRC helpers, networking, serializers and
lifecycle plumbing. It provides actor, command-script and Havok component
storage initialization. It does not contain a complete actor behavior engine,
weapon/projectile simulation, script VM, audio mixer or game renderer. Its
README explicitly says game boot is unimplemented.
[Status and host boundaries](https://github.com/BirchWoodGod/halo2-decomp/blob/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888/README.md),
[actual build inputs](https://github.com/BirchWoodGod/halo2-decomp/blob/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888/CMakeLists.txt).

Locally, CMake/Ninja configuration and the full library/host build passed with
GCC 16.2.1 and OpenSSL 3.6.5. The command
`tools/reference/halo2-native-reference/build-research/halo2-engine-host --probe-pools assets/disc/default.xbe` passed
native allocation/lookup/deletion for command scripts (40 records, stride
`D4`), joint commands (10, `8C`), Havok components (512, `A0`), actors
(256, `888`) and online tasks (24, `14`); actor firing-position ownership
insertion/lookup/removal also passed. This was the diagnostic probe, not the
upstream differential suite or a game boot.

The library preserves 32-bit guest addresses and engine little-endian memory,
mapping the pinned original image for data. Host code cannot replace guest
pointers with 64-bit C++ pointers in existing structures. OpenSSL Crypto and
`libm` are required; compiler flags include strict warnings and selected
`-ffp-contract=off`. Its heap is one arena, aligned to 16 bytes, without thread
safety; scheduling, TLS, physical aliases and several SDK services remain
boundaries. [Memory and ABI analysis](https://github.com/BirchWoodGod/halo2-decomp/blob/080b4d46e1f292a0f0b5cb1950c7b70dd8ef3888/ENGINE.md).

## Static collision and instance collision

`sbsp +14` is the collision BSP tagblock, stride `40`. Its eight nested blocks
are below. Each listed offset is relative to a collision record. Instance
definitions at `sbsp +138`, stride `C8`, embed the **same eight blocks directly
at `70..A8`**; they do not point to another `40`-byte collision record.
[Xbox sbsp layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/sbsp.xml).

| Block | Collision offset | Instance definition offset | Stride and fields |
| --- | --- | --- | --- |
| BSP3D nodes | `00` | `70` | `08`: plane i16 `+0`; signed 24-bit child `+2`, child `+5` |
| Planes | `08` | `78` | `10`: float `nx,ny,nz,d` |
| Leaves | `10` | `80` | `04`: flags u8 `+0`; 2D reference count u8 `+1`; first reference i16 `+2` |
| BSP2D references | `18` | `88` | `04`: plane i16 `+0`, node i16 `+2` |
| BSP2D nodes | `20` | `90` | `10`: float `nx,ny,d` at `+0,+4,+8`; child i16 `+C,+E` |
| Surfaces | `28` | `98` | `08`: plane i16 `+0`, first edge i16 `+2`, flags u8 `+4`, breakable index u8 `+5`, material i16 `+6` |
| Edges | `30` | `A0` | `0C`: six i16 values: start vertex, end vertex, forward edge, reverse edge, left surface, right surface |
| Vertices | `38` | `A8` | `10`: float3 position `+0`; first edge i16 `+C`; unused i16 `+E` |

Retail `14A280` computes `dot(normal, point) - d`. A nonnegative distance
selects the child at node `+5`, a negative one the child at `+2`. Child bit
`800000` marks a leaf, `FFFFFF` is NONE, and leaf index uses mask `7FFFFF`.
These branch semantics are firmer than the plugin's front/back labels.
Retail `14A300` strips plane index with `7FFF`; bit `8000` negates all four
plane components. Both routines are marked `matched` in the pinned upstream
inventory. [Recovered traversal and plane access](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_14a280.cpp).

Surface flags: bit 0 two-sided, 1 invisible, 2 climbable, 3 breakable,
4 invalid, 5 conveyor. Invisible is a rendering property and does not imply
absence of collision. Polygon traversal in recovered `1EDD00` chooses the
end vertex/reverse edge when `edge.rightSurface == surfaceIndex`, otherwise
the start vertex/forward edge, until the first edge is reached again.
Apply the instance scale, basis and translation already used for render
geometry. Validate edge adjacency, vertex ranges and a bounded visited set.
[Original surface-shape construction](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1ee340.cpp).

Read-only Lockout check: BSP metadata file `EC0E10`, collision record
`EC11C8`; counts are 4388 nodes, 1331 planes, 2313 leaves, 5989 2D references,
4020 2D nodes, 3162 surfaces, 5947 edges, 2587 vertices. Instance definition 0
at file `F2A73C` has 68 nodes, 24 planes, 51 leaves, 115 references, 141 2D
nodes, 99 surfaces, 195 edges and 98 vertices.

Static ray/sweep collision extracted from these polygons is useful, but does
not establish retail biped or Havok behavior. The matched BSP setup routine
`1C2910` constructs Havok representation and calls further physics routines.
Dynamic bodies, contacts, stepping/sliding, moving devices, surface material
responses and breakable state need their actual behavior recovered.
[BSP-to-Havok setup](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1c2910.cpp).

## Player and authored actor placement

The scenario layout supplies these tagblocks. Palette entries reference
definitions; placement records are not enemy entities by themselves.
[Xbox scenario layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2Xbox/scnr.xml).

| Scenario block | Offset | Stride |
| --- | --- | --- |
| Biped placements / palette | `60` / `68` | `54` / `28` |
| Weapon placements / palette | `90` / `98` | `54` / `28` |
| Player starting profiles | `F8` | `44` |
| Player starting locations | `100` | `34` |
| Squads | `160` | `74` |
| Zones | `168` | `38` |
| Character palette | `178` | `08` |
| Orders | `240` | `7C` |

Biped/weapon placements share palette i16 `+0`, object-name i16 `+2`, flags
u32 `+4`, position float3 `+8`, orientation float3 `+14`, scale float `+20`.
Palette datum is `+4`. Flags include bit 0 not automatically placed,
bits 1/2/3 not easy/normal/hard, bit 6 never placed, bit 8 create at rest.
Do not instantiate every authored biped immediately: cinematic actors,
script-created objects and difficulty exclusions share the table.

Player starting profiles: name ASCII32 `+0`, starting health/shield damage
floats `+20/+24`, primary weapon datum `+2C`, loaded/total rounds i16
`+30/+32`, secondary datum `+38`, loaded/total `+3C/+3E`, four grenade counts
u8 `+40..43`. A round count of `-1` requests the weapon default. Player
locations have position `+0`, facing float `+C`, team i16 `+10`, BSP i16 `+12`.

Squad fields: name ASCII32 `+0`, flags u32 `+20`, team i16 `+24`, parent group
i16 `+26`, delay float `+28`, normal/legendary counts i16 `+2C/+2E`, major
upgrade i16 `+30`, vehicle palette i16 `+34`, character palette i16 `+36`,
initial zone i16 `+38`, primary/secondary weapon palette i16 `+3C/+3E`,
grenade type i16 `+40`, initial order i16 `+42`, starting-location block `+48`,
placement script ASCII32 `+50`, script index i16 `+70`. Relevant flags:
respawn bit 7, blind 8, deaf 9, brain-dead 10, initially placed 12.

Squad starting locations have stride `64`: name stringID `+0`, position
float3 `+4`, reference frame i16 `+10`, facing float2 `+14`, flags u32 `+1C`,
character palette i16 `+20`, primary/secondary weapon i16 `+22/+24`, vehicle
i16 `+28`, seat i16 `+2A`, grenade type i16 `+2C`, swarm count i16 `+2E`,
actor/vehicle variant stringIDs `+30/+34`, initial movement distance float
`+38`, movement mode i16 `+3E`, script ASCII32 `+40`, script index i16 `+60`.
Flags include asleep bit 0, always place bit 3, initially hidden bit 4.
An unspecified per-location character/weapon index uses the squad default.
Character palette datum is `+4`.

AI script reference type is the top two bits: 0 squad, 1 squad group,
2 runtime actor, 3 starting location. Squad/group index occupies low16;
a location encodes squad in bits16..29 and location in low16.
Only classify actual AI literal nodes (source/result type `13` hexadecimal);
a function-name node of result type2 with opcode19 decimal means `sleep`.
Local examples: tutorial AI node1054 value `1` names squad `johnson`; Cairo
node11953 value `40000021` names squad group `bay2_cov_fnl`.
`273480` handles squad/group iteration and individual locations before calling
`201520`; it also writes AI runtime state. Replacing `ai_place` with a generic
spawn-all call changes authored activation and counts.
[AI index interpretation and placement](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_272b70.cpp),
[squad placement implementation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_2003a0.cpp).

Team0 means resolve defaults, not an enemy faction. Recovered `200C20`
initializes squad runtime byte `+76` from authored squad team `+24`; when
zero it follows the **squad** default character palette `+36`, character's
unit datum `+10` (else creature datum `+18`), then unit DefaultTeam at `+C0`.
If still zero it resolves inherited general properties via `1E4990` and
indexes a 20-short actor-type/default-team table at retail VA `44AEE8`.
The original table bytes are
`3,7,3,3,3,3,1,2,2,4,4,4,5,5,-1,3,7,7,7,4`.
The routine is upstream `todo`; the table was directly checked in the XBE.
Per-location character overrides do not alone redefine the squad team.
[Squad initialization and defaults](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_2003a0.cpp).

Local Cairo Station (`01b_spacestation`) has 117 squads, 20 character palette
entries, 94 authored biped placements/11 biped palette entries, and 95 weapon
placements/11 weapon palette entries. Squad 1 `1st_hum_floor_mar` has normal
count 3 and legendary count 2. Location 0 is
`(60.821899,-15.169637,7.000264)`, facing `(1.286393,0)`, flags `8` (always
place), character `marine_female` and battle rifle. Starting profile 1 equips
SMG and magnum. Profile 0 references no weapons; scripts determine the
transition/loadout behavior.

## Character parameters, movement and AI boundaries

`char` parent character reference is `+4` (datum `+8`); its unit/biped
reference is `+C` (datum `+10`). Parameters can be inherited: Cairo's base
grunt and marine have **zero local perception records** and parent
`ai\generic`. Parse parent chains and reject cycles before resolving a
missing block. [Character layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/char.xml).

| Character block | Offset/stride | Useful fields relative to record |
| --- | --- | --- |
| Vitality | `3C` / `70` | normal body/shield floats `+4/+8`; legendary `+C/+10`; body recharge delay/time `+40/+44`; shield `+48/+4C` |
| Placement | `44` / `34` | upgrade chances by difficulty in three sets of four floats |
| Perception | `4C` / `34` | max vision `+4`; central/max/peripheral angles `+8/+C/+10`; peripheral distance `+14`; hearing `+18`; combat/guard/idle acknowledgement time `+24/+28/+2C` |
| Weapon properties | `CC` / `CC` | weapon ref `+4`; max/min firing range `+C/+10`; nested firing patterns `+BC`, stride `40` |

Local grunt normal/legendary body vitality is 25/40, no shields; marine is
100/150, no shields. These are parameters, not the full damage calculation.
Zones contain a firing-position block `+28`, stride `20`: float3 position
`+0`, reference frame i16 `+C`, flags u16 `+E`, area i16 `+10`, cluster
i16 `+12`, normal float2 `+18`. Orders and areas constrain actor movement.
Microsoft's Halo 2 AI documentation describes behavior-tree transitions,
character settings, squads, orders and firing positions; a chase/attack
state machine alone does not reproduce those systems.
[Halo 2 behavior list](https://learn.microsoft.com/en-us/halo-master-chief-collection/h2/ai/aibehaviorlist),
[AI engineering outline](https://learn.microsoft.com/en-us/halo-master-chief-collection/h2/ai/aiengineeringoutline).

Player movement globals: `matg +130` player-information block, stride `11C`.
Walking speed float `+24`; run forward/back/side `+2C/+30/+34`, acceleration
`+38`; sneak forward/back/side `+3C/+40/+44`, acceleration `+48`; airborne
acceleration `+4C`. Cairo reads run 2.25/2/2, acceleration 9.6; sneak
.9/.65/.6, acceleration 4.8; airborne acceleration 1.05.
[Global layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/matg.xml).

Biped movement/camera: jump velocity float `1F8`; standing/crouching camera
heights `218/21C`; standing/crouching capsule heights `268/26C`; radius
`270`. Use the actual player biped definition. Retail `DD7D0` in `bipeds.cpp`
is a camera/crouch-related recovered candidate, but upstream marks it `todo`;
its ABI and numeric behavior require instruction-level comparison before
calling it a faithful native routine.
[Biped layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/bipd.xml),
[recovered biped routines](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/bipeds.cpp).

The retail gravity scalar is **4.1712594 world units/sec²**, not 9.81 in
unconverted map units. Matched initialization/reset `1EB550/1EB5F0` write
that value to the first float of globals `g_51E9C4`. Original initialization
instructions read VA `4455D8`; direct bytes `F5 7A 85 40` decode to
`4.17125940322876`. Projectile gravity scale multiplies this scalar;
physics-set-gravity builtin `07F`, evaluator `2A2B30`, also multiplies its
argument by this base. Actual world units must stay consistent across speed,
position, capsule size and gravity.
[Physics globals initialization](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_1eb550.cpp),
[physics script evaluators](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).

## Weapons, projectiles and damage

Weapon blocks: magazines `2C0`, stride `5C`; triggers `2C8`, stride `40`;
barrels `2D0`, stride `EC`.
[Weapon layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/weap.xml).

| Record | Fields relative to record |
| --- | --- |
| Magazine | flags u32 `+0`; rounds recharged/initial/max reserve/max loaded/max inventory i16 `+4/+6/+8/+A/+C`; reload time float `+10`; rounds reloaded i16 `+14`; chamber time float `+18` |
| Trigger | input/behavior i16 `+4/+6`; primary/secondary barrel i16 `+8/+A`; prediction i16 `+C`; autofire time/throw floats `+10/+14` |
| Barrel | rounds/sec float pair `+4/+8`; acceleration/deceleration time `+C/+10`; shots/fire i16 pair `+1C/+1E`; recovery float `+20`; magazine/rounds per shot/min rounds i16 `+28/+2A/+2C`; projectiles/shot i16 `+6A`; distribution angle float `+6C`; error angle float pair `+74/+78`; projectile datum `+90`; damage effect datum `+98` |
| Barrel firing effect | block `+E4`, stride `34`: shot-count i16 bounds `+0/+2`; firing effect ref `+4`; misfire/empty refs `+C/+14`; corresponding damage refs `+1C/+24/+2C` |

Effects must be followed through their `effe` definitions to find sound,
particles and other event behavior. Reload times may be zero because authored
animation events control transitions. A generic fixed reload delay changes
retail behavior.

Projectile `proj`: flags u32 `BC`, collision radius float `C8`, arming time
`CC`, detonation timer float pair `D4`, max range float `E0`; detonation,
attached, super and impact damage datums `104/10C/11C/14C`. Air gravity scale
float `164`, air damage range pair `168`, water gravity `170`, water damage
range pair `174`; initial/final velocity floats `17C/180`; acceleration
distance pair `18C`. Flag bit 5 means travels instantaneously, bit 1 AI
ballistic aiming, bit 4 distance damage scaling. Material responses at `19C`,
stride `58`, include material name stringID `+C`, material index i16 `+10`,
default/potential responses i16 `+2/+14`, chance float `+18`, impact angle
pair `+1C`, velocity pair `+24`, angular/velocity noise `+38/+3C`, friction
floats `+48/+50/+54`.
[Projectile layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/proj.xml),
[recovered projectile motion](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/projectiles.cpp).

Damage effect `jpt!`: radius float pair `0`, cutoff scale `8`, general flags
u32 `C`, side effect/category i16 `10/12`, damage flags u32 `14`, AOE core
radius float `18`, damage lower bound float `1C`, upper bounds float pair
`20/24`, stun float `34`, stun time `3C`, acceleration `40`, general/specific
damage stringIDs `50/54`. Damage flags include not hurting owner bit 0,
headshots bit 1, not hurting friends bit 3, shield-only bit 6, bypass shield
bit 9, not hurting players bit 15.
[Damage layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/jpt!.xml).

Retail damage includes material/armor multipliers from `matg +D0`, nested
damage groups and armor modifiers, plus difficulty/team scaling. Recovered
`D5BC0` multiplies all found combinations of two damage-group and two armor
IDs, defaulting to 1; it is upstream `matched`. Maximum and normalized
current body/shield vitality are distinct. Headshots, regions, vehicle riders,
shield transfer and death side effects need the actual damage pipeline.
[Recovered damage routines](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/damage.cpp).

Local Cairo battle rifle: weapon datum `FF041CCC`, metadata file `EB4013C`.
Magazine 0 initial/max reserve 108/144, max loaded 36. Barrel 0 fires at
15 rounds/sec, shot bounds 3/3, recovery .26 seconds, one projectile/shot.
Projectile datum `FF111CD9` is instantaneous (`flags=20`), initial/final
speed 400, gravity 0, max range 40. Impact damage datum `FF141CDC` has
lower/upper 6/6/6, flags `2002`. Do not implement this bullet as a slow
ballistic actor solely because a projectile tag exists.

## Compiled scripts and VM

Retail scenarios use a contiguous expression tagblock at `scnr +238`, stride
`14`. There is **no embedded pool header preceding these cache nodes**.
`scnr +1A8` legacy syntax dataref was zero in every local level map. Script
strings use dataref `1B0` (byte count and virtual pointer), scripts use block
`1B8`, globals `1C0`; script/global records have stride `28`: name ASCII32,
script type/return type i16 `+20/+22`, root salted datum u32 `+24` (globals
have their type at `+20`).
[Xbox script cache layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/Layouts/H2X_Layouts_Scenario.xml).

| Node field | Offset | Meaning |
| --- | --- | --- |
| Salt | `0` u16 | High16 of valid datum; low16 selects array slot |
| Opcode/value type | `2` i16 | Builtin or script index for call; source type for literal |
| Result type | `4` i16 | Type after cast |
| Flags | `6` u16 | Primitive bit0, script-call bit1, global-reference bit2, compiled bit3 |
| Next expression | `8` u32 | Next argument/sibling salted datum or `FFFFFFFF` |
| String offset | `C` u32 | Offset into script string data |
| Value | `10` u32 | Literal union, global index, or first child datum |

Resolve a nonprimitive call's `value` to its function-name child, then walk
that child's `next` as arguments. For script-call flag bit1, the call opcode
indexes the scenario scripts; otherwise it indexes builtin definitions.
The function-name child may have flags `9` in either case, so its flags alone
cannot distinguish script calls. Global references use low16 of value;
bit `8000` selects external rather than scenario global. Literal real is an
IEEE float bit pattern; boolean, short and long use their actual widths.
Use salts, bounds and traversal budgets to validate graphs.
[Recovered expression evaluator](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209520.cpp),
[argument evaluation and sibling traversal](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209d50.cpp),
[type enum](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/hs.h).

Actual string literals have a stale authoring/runtime pointer in raw `value`;
use `stringOffset` and the relocated string-data base. Retail fixup `2A09C0`
compares node result type `+4` to9 and writes `node.value = node.stringOffset
+ scnr.stringData`. Direct retail disassembly confirmed those fields. All
118 tutorial string literals had old base `0DE87CA8`, all288 Cairo literals
old base `13E77F30`, unrelated to their cache string-data pointers. Tutorial
node60 has stringOffset `66` and stale value `0DE87D0E`; its text is read at
the map's translated string data plus `66`. Function-name strings also use
the stringOffset field, although their result type is2.
[String fixup implementation](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_29f5b0.cpp).

Script types: startup 0, dormant 1, continuous 2, static 3, stub 4,
command_script 5. Useful value types: boolean 5, real 6, short 7, long 8,
string 9, script A, AI 13, object-list 1F, sound 20, looping sound 23,
object definition 26, difficulty 2C, team 2D, object 32, unit 33.
[Opcode, parameter and type table](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Blamite/Formats/Halo2Xbox/H2X_Scripting.xml).

| Builtin | Opcode |
| --- | --- |
| begin / begin_random / if / set | `000` / `001` / `002` / `004` |
| and / or; add / subtract / multiply / divide | `005` / `006`; `007` / `008` / `009` / `00A` |
| sleep / sleep_forever / sleep_until / wake | `013` / `014` / `015` / `016` |
| players; volume_test_object; physics_set_gravity | `01C`; `020`; `07F` |
| object_create / object_create_anew / object_destroy | `02E` / `030` / `034` |
| unit_add_equipment | `0FA` |
| ai_place(ai) / ai_place(ai, short) | `122` / `123` |
| ai_kill / ai_erase | `127` / `129` |
| switch_bsp | `1FC` |
| sound_impulse_start / sound_impulse_stop | `255` / `25B` |
| sound_looping_start / sound_looping_stop | `25E` / `25F` |

The matching XBE's builtin table is at VA `4744E0`, an array of 32-bit
definition pointers. Definition: return type u16 `+0`, flags u16 `+2`,
evaluator VA `+4`, parse/help pointer `+8`, argc i16 `+C`, parameter types
i16 array `+E`. Names are **not stored in this definition**. Direct string
search found no `ai_place`, `begin` or `sound_impulse_start` in the retail
XBE; actual names are present in compiled scenario function-name nodes.
Primary naming/signature authority is the versioned opcode table plus those
retail strings, not invented address labels.

| Opcode | Definition VA | Evaluator VA | Verified parameter types |
| --- | --- | --- | --- |
| `000` | `44B098` | `207C20` | Special form |
| `013` | `44B1C8` | `2087E0` | Special form |
| `122` | `44C734` | `2A4FD0` | AI (19 decimal) |
| `123` | `44C748` | `2A5010` | AI, short (19,7) |
| `255` | `44DE5C` | `2A9E00` | sound, object, real (32,50,6) |

The runtime separately allocates `9000` expression slots of size20 and
attaches cache nodes via `20A750 -> 16B6B0`. Its data-array header is `4C`
bytes (name32, capacity `20`, stride `24`, alignment `28`, valid `29`, flags
`2A`, signature `2C`, allocator `30`, free/high-water/actual counts
`34/38/3C`, next salt `40`, guest data pointer `44`, bitmap pointer `48`).
HS threads use a different pool: capacity `140`, record stride `418`.
[Script initialization and attachment](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209580.cpp),
[pool header](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/include/data_array.h).

Retail sleep converts script ticks at a nominal 30Hz: delay times
`0.03333333507180214 * gameTickRate`, rounded by x87 `fistp`; thread deadlines
are simulation ticks. `sleep_forever` uses `-2`; completion uses NONE `-1`.
Evaluation pushes persistent frames and can yield while evaluating arguments.
Startup/dormant completion differs from continuous scripts. Unknown builtins
must be reported or block the script; silently treating them as successful
changes mission progression.
[Thread runner, sleep and special forms](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209520.cpp).

Across all 27 local level maps: 5295 scripts, 798 globals, 337147 expression
slots, 295146 active; remaining slots have poison `BABA` flags and must be
skipped. Every legacy syntax dataref was zero. All 89655 builtin calls
resolved to names matching Assembly XML (390 distinct builtin names), and
all 3367 script calls resolved to their script names; no child salt mismatch
was found. Tutorial has 77 scripts/7068 slots/5988 active; Cairo has
418 scripts/27091 slots/24316 active. This proves parsed data relationships,
not execution semantics of those calls.

## Sound resource metadata and Xbox ADPCM

`snd!` is a `14`-byte compact descriptor. Its fields are flags i16 `0`, class
u8 `2`, sample-rate u8 `3` (0=22050,1=44100,2=32000), encoding u8 `4`
(0 mono,1 stereo,2 codec), compression u8 `5` (1 Xbox ADPCM,4 WMA), playback
index i16 `6`, pitch-range start i16 `8`, pitch-range count u8 `A`, scale u8
`B`, promotion/custom-playback u8 `C/D`, extra-info i16 `E`, max playtime
i32 `10`. Do not derive channel count as encoding+1 when encoding is codec.
[Sound descriptor and resource reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/SoundTag.cs).

Find the map's global `ugh!` tag. It supplies these blocks:

| Block | Offset/stride | Relevant fields |
| --- | --- | --- |
| Playback parameters | `0` / `38` | min/max distance floats `+0/+4`, skip fraction `+8`, gain base/variance `+10/+14`, pitch i16 bounds `+18`, cone floats `+1C/+20/+24` |
| Sound/import names | `10` / `4` | stringID |
| Pitch-range parameters | `18` / `A` | natural pitch and bounds i16 |
| Pitch ranges | `20` / `C` | name i16 `+0`, parameter index `+2`, first permutation i16 `+8`, permutation count i16 `+A` |
| Permutations | `28` / `10` | name i16 `+0`, skip fraction i16 `+2`, gain u8 `+4`, sample size i32 `+8`, first chunk/count i16 `+C/+E` |
| Chunks | `40` / `C` | raw pointer u32 `+0`, raw size u32 `+4` masked with `00FFFFFF` |

Raw pointer high2 selects local/mainmenu/shared/single_player_shared; low30
is file offset. Concatenate chunks in permutation order before decoding;
do not substitute bytes from a similarly named tag in another map. The
permutation sample-size field matched compressed byte size in the tested
clip; it is not decoded PCM frame count.
[Gestalt resource reader](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Blam/Blam/Halo2/SoundCacheFileGestaltTag.cs),
[playback/gestalt layout](https://github.com/XboxChaos/Assembly/blob/fd14ef3c46135e9422966c6a64cd926fb0bd23fb/src/Assembly/Plugins/Halo2/ugh!.xml).

Xbox ADPCM uses WAVE format `0069`, four coded bits/sample, block alignment
`36 * channels`, 64 decoded frames/channel/block, bytes/sec
`sampleRate * blockAlign / 64`. Reclaimer's extra bytes specify size2 and
samples/block64. [Header implementation](https://github.com/Gravemind2401/Reclaimer/blob/36224d7a4581f2a1f255099bf706aa56be9f6e21/Reclaimer.Core/Audio/XboxAdpcmHeader.cs).

FFmpeg added `AV_CODEC_ID_ADPCM_IMA_XBOX` at commit
`c3083b3266cf01fdc43e5d03e27bea35d7c1faae`. Initialize codec with
bits_per_coded_sample 4, block_align 36*channels and the actual sample rate;
output is planar S16. Headers contain signed little-endian predictor and
step index per channel, followed by four-byte nibble groups per channel,
low nibble first. Its decoder drops the extra final sample to produce64;
ordinary IMA WAV's 65-frame block handling is unsuitable. Submit complete
blocks, then convert/interleave and resample for SDL as required.
[FFmpeg decoder introduction](https://ffmpeg.org/pipermail/ffmpeg-cvslog/2024-December/146567.html),
[pinned decoder source](https://github.com/FFmpeg/FFmpeg/blob/c3083b3266cf01fdc43e5d03e27bea35d7c1faae/libavcodec/adpcm.c).

Local Cairo contains 4175 `snd!` tags, all compression 1. Actual
`sound\weapons\battle_rifle\fire_burst`, datum `FF161CDE`, metadata file
`EB43E1C`, is 44100Hz stereo Xbox ADPCM, pitch-range index 3924, four
permutations. Permutation 0 reads shared.map chunks:

| File offset | Byte count |
| --- | --- |
| `44B9000` | 32760 |
| `44C1000` | 32760 |
| `44C9000` | 32760 |
| `44D1000` | 43920 |

Total 142200 bytes, SHA-256
`1cb6abce424b27f658cad0e0f7388a5c37365eb5257cf2d4610fbe8eaac6a67a`.
Transient local FFmpeg decoding passed without warnings to 126400 stereo
PCM frames (505600 S16 bytes), SHA-256
`d1c8145e714480b284e1df55526b808c74ce08e28448dd273ea60e7b25887acf`.
All 1975 block headers had step indices 0..84. Decoding proves codec/resource
compatibility; playback/spatialization, event timing, permutation choice,
gain and loop transitions still require retail behavior.

## Match evidence and practical behavior boundaries

The following statuses come from the pinned upstream inventory, not a local
rerun of its original-compiler checker:

| Candidate | Status | Behavior/source |
| --- | --- | --- |
| `14A280`, `14A300` | matched | BSP leaf traversal and oriented plane access |
| `1C2910` | matched | BSP Havok representation setup, dependencies required |
| `D5BC0` | matched | Damage group/armor multiplier product |
| `20A700`, `20A750` | matched | Script storage setup and cache-node attachment |
| `1EB550`, `1EB5F0`, `2A2B30` | matched | Base gravity initialization/reset and script scaling |
| `2A4FD0`, `2A9E00` | matched | `ai_place`/sound impulse evaluator wrappers, dependencies required |
| `DD7D0`, `101980` | todo | Biped crouch/camera candidate and ammunition initialization |
| `1EDD00`, `201520`, `273480` | todo | Collision shape construction and AI placement |
| `2099F0`, `209850`, `209D50` | todo | Expression/frame evaluator and argument scheduler |
| `2087E0`, `208750`, `2089B0` | todo | Sleep/sleep-forever/sleep-until |
| `200C20`, `2A09C0` | todo | Squad default-team resolution and script string fixup |

[Upstream function inventory](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/config/functions.csv).

A matched wrapper does not recover the engine function it calls. Recovered
`todo` source is useful analysis input but needs ABI recovery and a retail
instruction oracle before a faithful native claim. Data-driven collision,
spawn inspection, weapon parameters, graph inspection and actual audio
decoding are concrete next steps. Collision approximations, generic enemy
brains, limited opcode dispatch and simplified combat must remain explicitly
identified as partial implementations until the corresponding authored
mission paths and original simulation behavior are verified.

## Native continuation evidence: scalar HS casts

The new native differential harness separately checks `20A000`, `20A020`,
`20A070`, `20A090`, `20A0B0`, `20A0D0`, `20A0F0`, `20A100` and `20A110` against
local retail instructions. Eight are `matched` in the pinned upstream
inventory; `20A110` remains `todo`. Fifteen source/target type pairs pass
15,210 raw-bit comparisons, including random encodings, infinities, NaNs,
negative zero and signed short bounds. Original helpers use a stack argument,
`EAX` return and `ret 4`, unlike the biped crouch register convention.
[Recovered cast bodies](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_20a000.cpp),
[cast table initialization](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_20a130.cpp).

Notable instruction facts: `20A000` tests the raw dword for zero and writes
`sete` into the low byte, preserving the other three bytes. `20A020` tests the
signed word and also preserves the other bytes. `20A0B0` converts the signed
word plus one to real. `20A0D0/20A0F0` use `cvttss2si`, whose nonrepresentable
result is `80000000`; the short conversion overwrites only the low word.
`20A100` sign-extends a short; `20A110` preserves the raw dword. These are
preserved directly, including behaviors a host-language truth conversion
would change. String/object casts and the full HS evaluator are outside this
instruction-oracle claim.

`2099F0` narrows global references to a word, with bit `8000` selecting an
external global. The workbench masks the index to 16 bits and explicitly
rejects external references. Primitive node `+2` is its source type; node `+4`
is the requested result type. Function-name nodes use `+2` as opcode instead,
so they must not be decoded as primitive casts. See
[original expression and cast dispatch](https://github.com/kirklandsig/halo2-decompiled/blob/5f08630655e3ddc9ac45909090f11ae799fa0c27/src/unknown_209520.cpp).

Implementation, supported operations, remaining fidelity boundaries and live
GPU/audio evidence are documented in [NATIVE_SYSTEMS.md](NATIVE_SYSTEMS.md).
