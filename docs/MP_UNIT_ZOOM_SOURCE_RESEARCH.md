# Original unit zoom selector and admission checkpoint

The native module in `include/halo2/unit_zoom.hpp` and `src/unit_zoom.cpp`
implements the bounded original selector, admission and zoom-field consumers.
It does **not** make rendered right-click scope work: no live `Unit` caller,
camera FOV owner, transition clock or authored binocular overlay is integrated
by this work. The runtime's high-level player state is not admission evidence.

## Authority and immutable evidence

The unchanged owned disc1.1 executable is
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The original Lockout cache is SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The tool imports the unchanged frozen attachment fixture, SHA-256
`9895480d4d864d67ee92307d627c04a005e4ffb8f06761be68c9de0d7274563c`.

Separate retained source outputs are at
`analysis/multiplayer/integration/unit-zoom/measured-frozen/` and
`repeat-frozen/`; report SHA-256 is
`238c61759d2b565e3209dc72893661055b743b944f525f5fe0ad770c0975dbb2`.
They contain 132 executed original leaf cases and ten complete `C6C50`
unit phases. All 23 instruction listings include exact executable bytes.
`tools/oracle_mp_unit_zoom_1_1.py` executes unchanged source instructions;
`tools/export_mp_unit_zoom_fixtures.py` exports captured outputs, without a
native implementation producing expectations. `source-audit.json` and
`freeze-manifest.json` retain the independent repeat and byte verification.

The incoming zoom selector is an explicitly supplied native command input.
The 32 individual action bits, parent, magazine mode, authored count/range
controls and disabled unit gates are explicitly modified branch fixtures;
they are not ordinary action producers. The inherited fixture preallocates
its player/unit and declares object backing storage, world pose, resource
residency, unavailable audio/effect/transport and terrain-physics services.
No new game, platform, animation or admission success is supplied here.

The owned later1.5 executable is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Separate current instruction correspondence identifies `C9920→CAAA0`,
`C9CC0→CAE40`, `C9BE0→CAD60`, `1051C0→1060E0`,
`105120→106040`, and `C6C50→C7DD0`, with matching complete instruction
shape/internal branches. Those later addresses are a byte comparison;
this fixture executes1.1, not a newly constructed1.5 world. Runtime-global
relocations and all owning state producers remain separate.

## Exact state and data contract

| Source | Fields |
| --- | --- |
| Unit | parent datum `+0x14`; current/requested zoom signed bytes `+0x240/+0x241`; opaque raw motion bits `+0x26C` |
| Unit action component | signed-short component offset `+0x346`; action words at component `+4/+8` |
| Current equipment | signed slot bytes `+0x212/+0x213`; full datum slots `+0x218+n*4` |
| Weapon definition | flags DWORD `+0x12C`, no-binocular bit `0x00010000`; signed-short zoom count `+0x1FE`; float minimum/maximum `+0x200/+0x204` |
| Magazine | authored count DWORD `weap+0x2C0`; runtime signed mode `weapon+0x224+n*0x10` |
| Global player information | first pointer at `matg+0x134`; signed32 binocular count `+0xB8`; float minimum/maximum `+0xBC/+0xC0` |

The existing serialized player-information importer has stride `0x11C`.
These source zoom leaves directly follow the first `matg+0x134` pointer;
they do not introduce a new block-count gate. Stock SMG flags are
`0x00420000`, zoom count0, and do **not** forbid binoculars. The original
fallback has count1 and both range endpoints `0x40400000` (3). Stock BR
has weapon zoom count1 and range2/2. These are retained original values,
not a native tuning choice.

`C52C0` stores both zoom bytes `FF` at `C534D/C5353`. Its action-component
allocation at `C54AB..C54B5` passes alignment0, size`0x44`, descriptor
offset`0x344` to `BD270`. `BD270` writes the descriptor's offset word and
clears appended storage at `BD333/BD33A`. Its earlier allocation at
`C5482..C548F` is a different `0x124`-byte component at `0x33C`.
The genuine independently frozen whole-body constructor confirms this:
`analysis/multiplayer/integration/spawn-source/live-body-world/report.json`
cases1/2 first stages contain source body5750 bytes, component offset
`0x1632` from `unit+0x346`, and both action words0. This supports an
independent source action component initialized0 at construction; it
does not authorize resetting it whenever a native enum says Idle.

## Executed leaves and full unit consumer

`C9920` rejects a parent other than NONE, action word mask `0x0C440D03`,
or any equipped hand having an authored magazine whose signed runtime
mode is1 through6 inclusive. The action mask comprises bits26/27 and
types22,8,18,0,1,10,11, all in the first action word. `104A30` scans all
authored magazines when its selectorEBX is-1. There is no direct
weapon `+0x17C` countdown gate here. Other-hand equipment must be checked
when present; it cannot be omitted to force admission.

`C9CC0` uses the actual primary equipped weapon. Positive weapon zoom
count tail-calls `1051C0`: while any magazine mode is1..6, return the
supplied current selector; otherwise advance through the authored levels,
cycling the last to-1. For nonpositive weapon count, the no-binocular flag
returns-1. Otherwise positive signed32 binocular count admits
`signed16(current)+1` while that integer is less than count. Signed source
boundaries and low-word results are retained without invented clamps.

`151C90→C8140` copies the low byte of the incoming command selector
at `+0x24` into unit requested byte`+0x241`. `C6C50` reaches its zoom
consumer only when unit-definition`+0xBC` bit10 and unit-object`+0x10A`
bit2 are clear. Admitted requested zoom becomes current; otherwise current
becomes-1. A transition to-1 clears motion`+0x26C` and leaves requested
unchanged. It composes bit`0x40` into each equipped weapon's `1045E0`
input argument when current is not-1. All other input bits have separate
producers and are not generated by this module.

Two original incoming on/off pairs execute neutral command initialization
`14E780` (outputEDX), pressed edges`150DD0`, player actions`150F00`,
marshal`151C90` and complete`C6C50` without additional supplied returns.
They produce `FF/00→00/00` with carried-SMG zoom input`0x40`, then
`00/FF→FF/FF` with zoom input0. Additional complete phases execute
magazine/action denial and both disabled unit gates. Native tests compare
the three zoom fields and the zoom input bit against these full source
outputs; they do not claim parity for every unrelated `C6C50` side effect.

`C9A40` stores both bytes-1 and motion0 at `C9B00/C9B07/C9B0E`.
`source_zoom_reset` represents only those three stores: its other local-view,
control and sound effects remain outside this module.

## Magnification and camera boundary

`C9BE0` returns the float inXMM0, not the incidental EAX value. Its default
is1. An authored weapon with nonzero zoom count and no active magazine
uses `105240`; a forbidden binocular fallback returns1. Binocular current
-1 returns1. Otherwise the binocular fraction starts1; count other than1
uses separately rounded float(index)/float(count-1). Separate source SSE
subtract, multiply and add yield `(maximum-minimum)*fraction+minimum`.
Count1 therefore uses the **upper** endpoint. All 36 supported original
magnification results match native bits, including modified ranges.

Weapon `105240` differs: count1 uses fraction0; multiple levels use
float(index)/float(count-1); nonpositive bounds fall back to1. Its original
x87/CRT operation is `pow(upper/lower,fraction)*lower`. The bounded
one-level branch uses the lower endpoint and is measured against the
original CRT execution. Valid multi-level weapon magnification remains
explicitly unavailable (`nullopt`) in the native module: two retained
original cases prove that the unavailable path is not silently approximated.

The original input consumer `187610` divides the affected look-rate
components by `C9BE0` magnification. Additional actual call chain
`23E210→188B60→C9BE0` consumes a local-view magnification. Neither
this work nor an already-authored observer FOV conversion proves the
camera's zoom FOV, transition clock or overlay. These remain open.

## Action ownership required before a live caller

Read-only extraction from the previously frozen normal unit-phase suite
retains actual two-word control state across all captured phases:

| Existing source case | Captured states | Action words |
| --- | ---: | --- |
| Idle | 525 | `[0,0]` throughout |
| Supplied forward movement | 84 | `[0,0]` throughout |
| Held SMG fire | 420 | `[0,0]` throughout |
| Reload | 945 | `[0,0]`586 states; `[1,0]`359 states |
| Empty reload | 1575 | `[0,0]`1210 states; `[1,0]`365 states |

Files are unchanged under
`analysis/multiplayer/map-pack/spartan-equipped-unit-phase-suite/`.
These capture source input, normal `C7E90`, weapon, action, animation and
postpass phases with the older explicitly bounded fixture services.
They do not extend the proof to jump/crouch or arbitrary world actions.

Manual reload types0/1 select primary magazine0/1; types10/11 select
other-hand magazine0/1. All share actual `E88B0`. After `E8740` returns
success, `E8921..E8927` ORs the corresponding bit into action word.
`E8950` queries the actual current hand weapon's selected magazine through
`104A30`; `E7E60` keeps the bit while that query is true and clears it
when false. For the supported single primary SMG, this is bit0 and
magazine0. Successful `E8740` also calls `C9A40` to reset requested/current
zoom. Automatic reload inside `1021D0` does not itself execute that
unit reload request handler; its active magazine still denies zoom.

Source switch types8/18, grenade type22 and melee bits26/27 have distinct
owning action/animation lifecycles. Their requests being bound to keys
does not manufacture those owners. Jump/crouch control ownership remains
open at this checkpoint. A caller must retain actual source component
state and each supported reload transition, rather than infer zero from
the absence of a generic native handler. Camera ownership is required
before any rendered scope claim.

## Reproduction

Use separate output directories, preserving both frozen measurements:

```sh
.tools/python/bin/python tools/oracle_mp_unit_zoom_1_1.py --output /tmp/halo2-unit-zoom-reexecution
.tools/python/bin/python tools/export_mp_unit_zoom_fixtures.py /tmp/halo2-unit-zoom-reexecution/report.json /tmp/halo2-unit-zoom-reexecution/fixtures.tsv
```

The CMake test target uses
`analysis/multiplayer/integration/unit-zoom/measured-frozen/fixtures.tsv`.
The standalone normal and ASan/UBSan comparisons use143 rows:
43 admissions,51 cycles,36 bit-exact magnifications,ten full-unit field
updates,one initial state, and two explicit unavailable power cases.
No shared runtime, `Unit`, SDL routing, camera code or prior freeze was
edited by this research slice.
