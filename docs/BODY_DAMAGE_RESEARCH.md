# Retail body impact, armor and damage sections

Research date: 2026-10-07. Target: the local original Xbox build
`02.09.27.09809`. The useful next integration boundary is an actual body-impact
state transition: armor admission, selected/default damage section, normalized
body damage, protected minimum vitality, death admission, stun and effect
requests. It is not complete combat, model-hit selection or death-response
parity. The native capsule trace does not identify a model damage-region entry;
headshots require an authoritative hit-region source before enabling them.

## Primary evidence and pins

* Local executable [default.xbe](/home/eric/Developer/Halo-2-Decompiled/assets/disc/default.xbe),
  rechecked SHA-256
  `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
  Its instructions are behavioral authority.
* CC0 reconstruction, commit `5f08630655e3ddc9ac45909090f11ae799fa0c27`,
  [damage.cpp](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/src/damage.cpp).
  The manifest has `D8CB0/D9020/D5D20/DAAA0/D6800/D6A70/DB110` as `todo`,
  `DB090/D5BC0/D5B60` as `matched`. A retail annotation is not a match proof.
  [functions.csv](/home/eric/Developer/Halo-2-Decompiled/tools/reference/halo2-decompiled/config/functions.csv)
  owns these statuses. The local Ghidra export
  [all.c](/home/eric/Developer/Halo-2-Decompiled/analysis/retail/all.c)
  is navigation evidence.
* Pinned Assembly layout facts, commit
  `fd14ef3c46135e9422966c6a64cd926fb0bd23fb`,
  [hlmt.xml](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/hlmt.xml)
  and [matg.xml](/home/eric/Developer/Halo-2-Decompiled/tools/reference/assembly/src/Assembly/Plugins/Halo2/matg.xml).
  No GPL implementation bodies are copied.
* Original instruction listings:
  [D8CB0](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/d8cb0-research-disassembly.txt),
  [D9020](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/d9020-research-disassembly.txt),
  [DB090](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/db090-research-disassembly.txt),
  [DAAA0](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/daaa0-research-disassembly.txt),
  [D6800](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/d6800-research-disassembly.txt),
  [D6A70](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/d6a70-research-disassembly.txt),
  [DB110](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/db110-research-disassembly.txt),
  [upstream D7B80](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/d7b80-body-research-disassembly.txt).
  These ignored artifacts can be regenerated, for example:

```sh
RETAIL_XBE="$PWD/assets/disc/default.xbe" .tools/python/bin/python \
  tools/reference/halo2-decompiled/tools/disasm.py d8cb0
```

## Original calling ABI

Offsets below are from **entry ESP**, including the return address at `+0`.
These are the original compiler's register assignments, not a generic C/C++
ABI inferred from the reconstructed declarations.

| Entry | Register inputs | Stack arguments in increasing offsets | Return |
|---|---|---|---|
| `000D8CB0` | EDI=accumulator | `+4` object datum; `+8` effect definition; `+C` event | void, `RET 12` |
| `000D9020` | EAX=object datum; ECX=accumulator; EBX=effect definition | `+4` event | XMM0 float, `RET 4` |
| `000DB090` | ECX=damage info; EDX=model hit entry or null | `+4` object; `+8` owner; `+C` damage; `+10` inverse maximum body; `+14` accumulator | XMM0 float, `RET 20` |
| `000D5D20` | EAX=object datum | `+4` ignore-difficulty bool, low byte | XMM0 float, `RET 4` |
| `000D5BC0` | ECX=damage group A; EDX=damage group B | `+4` armor A; `+8` armor B | XMM0 float, `RET 8` |
| `000DAAA0` | none | `+4` info; `+8` object; `+C` owner; `+10` section index; `+14` section damage; `+18` accumulator | void, `RET 24` |
| `000DB110` | none | `+4` object; `+8` players-only bool; `+C` walk-siblings bool | AL bool, `RET 12` |
| `000D6800` | none | `+4` object; `+8` owner; `+C` notify-parent bool; `+10` special/exclusive bool | void, `RET 16` |
| `000B58C0` | none | `+4` update datum; `+8` dirty mask | void, `RET 8` |
| `001E9720` | EAX=scaling kind, 1 for body; ECX=signed team | none | XMM0 float, plain `RET` |
| `001E4A10` | ECX=actor-character datum | none | EAX=actor vitality settings or null, plain `RET` |
| `00176780` | EDX=object; EBX=owner; XMM0=scale A | `+4` effect tag; `+8` scale B; `+C` origin pointer; `+10` direction pointer | EAX=effect datum, `RET 16` |

`D8CB0` retains normalized body damage in XMM1 through `B58C0`; supplied
callback hooks must not casually overwrite that register. `DB090` retains its
region pointer across the region callback and reloads the original damage
after `DAAA0`. Callback stack cleanup errors can appear as physics/damage
disagreements. Primary evidence: the listings above and original entries
`D5D20`, `D5BC0`, `1E9720`, `1E4A10`, `B58C0`, `176780`.

## Fields and material selection

The object pool global `004E0300` points to a structure with header-array
pointer at `+44`. A header is 12 bytes, its object pointer at `+8`, type at
`+3`, flags at `+2`. The datum's low 16 bits select the header. The body-impact
fields are:

| Structure | Fields read or written |
|---|---|
| Object | `+0` definition datum; `+C/+10/+14` sibling/child/parent; `+AA` type; `+D4` update datum; `+E4` maximum body; `+E8` maximum shield; `+EC/+F0` normalized body/shield; `+F4` shield auxiliary state; `+106` signed body-stun ticks; `+10A` damage flags; `+120/+122` signed region-state byte length/offset; `+12C` actor datum; `+138` signed team; `+13C` player datum |
| Accumulator, 34 bytes | `+0` model hit entry or null; `+4` damage-info pointer; `+1C` remaining damage; `+28` normalized requested body damage; `+30` result flags |
| Event, 88 bytes | `+4` flags; `+8` owner (player datum, object datum, signed team); `+7C` signed global material index; `+84` packed reporting byte |
| Effect definition, jpt! tag `+10` | `+2` category; `+4` flags; `+40/+44` source damage groups |
| Damage info, hlmt `+60` block, stride F8 | `+0` flags; `+8` signed default section; `+28` default maximum body; `+2C` stun-damage threshold; `+30` body-stun seconds; `+B0` shield-depleted effect datum; `+BC/+C0` section count/pointer; `+CC` shield material; `+CE` indirect/default body material |
| Damage section, stride 38 | `+4` flags; `+8` vitality share; `+C/+10` response count/pointer; `+24` section-stun seconds |
| Runtime section state, stride 8 | `+0` destroyed response bits; `+2` level byte; `+4` packed pending response/ticks; `+6` signed stun ticks |
| Model hit entry, model `+58/+5C` count/pointer, stride 14 | `+6` signed damage-section index; `+10` signed global material index |
| matg runtime view | `+D4` damage-table pointer; `+150/+154` global-material count/pointer, stride B4; each material's armor keys at `+10/+14` |

The default body material is not the shield material. Original upstream
`000D7DE6..000D7DFA` selects the model hit entry and copies its material at
`+10` to event `+7C`. Without a selected hit entry, `000D7E06..000D7E0D`
copies damage-info `+CE`. `D9020` reads this event material, validates the signed
index, and applies the material's armor keys. Sources: original instructions;
pinned `damage.cpp:45..299,2824..2838`; Assembly layout facts above.

## Exact ordinary-impact sequence

1. `D9020` begins with accumulator remaining damage. Object damage flag `80`
   suppresses it to positive zero. Damage-info flag `20` (“Only Damaged By
   Explosives”) also suppresses it unless effect-definition flag `20` is set.
   A valid event material still invokes `D5BC0` after these gates, multiplying
   its ordered group-A/armor-A, group-A/armor-B, group-B/armor-A,
   group-B/armor-B product by the admitted damage. Missing groups/armor keys
   contribute identity; repeated keys are applied repeatedly. Invalid material
   indices bypass armor entirely. (`D9020..D910B`, `damage.cpp:1371..1390`.)
2. Only positive scaled damage enters body subtraction. `D5D20` obtains object
   maximum body, multiplied by kind-1 team scaling for types 0/1 unless effect
   category is 1. The inverse is float `1 / maximum` if maximum is positive,
   otherwise zero. (`D8CF4..D8D3E`, `D5D20..D5D76`.)
3. An explicit valid section with flag `80` and effect-definition flag 2 may
   cause a headshot prezero. The exception is game state `+8 == 1`, object type
   0, and a non-NONE player datum. Otherwise body becomes zero, event reporting
   byte becomes `(old & 3F) | 40`, and accumulator flags gain `1C0`. This runs
   **before** section routing and protection. The byte operation discards old
   bit 7 while preserving the lower six bits. (`D8D42..D8DCC`.)
4. `DB090` uses the info's signed default section unless an explicit model
   entry exists, in which case its signed `+6` overrides the default. An invalid
   explicit section does not fall back. For a valid section with positive
   vitality share, it calls `DAAA0` with `(damage * inverse_maximum) / share`,
   using float operations in that order. It then returns zero if section flag
   1 absorbs body damage, **even if the section vitality share was nonpositive**.
   Region response precedes absorption. (`DB090..DB104`.)
5. The body subtracts `inverse_maximum * post-region_damage`. Info flag `80`
   (“Cannot Die From Damage”) or object damage flag `4000` floors it at float
   bits `38D1B717`, value `0.00009999999747378752`. Otherwise negative vitality
   is retained. The accumulator's `+28` is the normalized requested subtraction
   **before** protection; it is not the actual vitality difference. A non-NONE
   update datum requests `B58C0(update, 40)` before writing `+28`.
   (`D8DEF..D8E5E`.)
6. Death admission runs even when the positive-damage branch was skipped.
   For types 0/1, it always applies kind-1 team scaling to object maximum body
   here, including category 1. If the resulting absolute body vitality is
   nonpositive, `DB110(object,false,false)` tests riders/children. A true result
   sets object flag `1000`; otherwise, if flag 4 is not already set, it calls
   `D6800(object,event+8,true,(acc_flags>>10)&1)` and then adds accumulator flag 1.
   (`D8E63..D8F10`.)
7. Post-region damage strictly greater than info `+2C` sets body-stun ticks.
   An actor override may supply seconds from vitality-settings `+40`.
   The signed tick rate from `g_510C54+2` converts to float; its float product
   with seconds is passed to x87 `FISTP` in nearest-even mode, then stored as a
   signed short. A `7FFF` existing stun is **not protected**. Equality does not
   reset it. (`D8F14..D8F8F`.)
8. If event flag 1, object type 0, and object definition `+260` contains an
   effect datum, request the attached effect when the body is not depleted and
   post-region damage is strictly greater than 10, or accumulator flags contain
   `41`. Death/region callbacks can have changed these flags before this test.
   Finally subtract post-region damage from accumulator `+1C`, even for negative
   damage or an absorbed hit. Do not clamp the remainder to zero.
   (`D8F96..D9013`.)

Finite valid body inputs in these three requested routines agree with the
pinned C's branch directions; no contradictory branch-direction recovery was
found. Their declarations still conceal the real ABI and intermediate numeric
contract. In particular, a replacement must preserve the inverse/multiply/
divide order, strict inequalities, negative vitality and damage remainders,
post-region stun admission, and callback order. Ghidra output is not a substitute
for the original SSE/x87 instructions.

## Region and death boundaries

For positive section damage, `DAAA0` divides by
`max(previous_threshold-next_threshold, float(1/255))`, scales by 255, adds the
existing level and uses **truncation** (`CVTTSS2SI`), then clamps to 0..255.
Section-stun conversion uses nearest-even, a different rounding operation.
At level 255 it resets the level and processes responses at the current equal
threshold, preserving their authored order. Model-state gates, rider gates,
under/over threshold flags, exclusive-special-death flags, skip RNG, delays,
effects, permutations and constraints belong to that broader state machine.
The response array has stride `50` and its fields are described by
`damage.cpp:216..271` and the pinned `hlmt.xml`.

Two source-reading traps outside the ordinary body boundary:

* Original `000DAADD..000DAAE9` sign-extends the
  region-state byte length and then uses a **logical** right shift by 3. The
  C reconstruction divides a signed short by signed 8. These disagree on
  malformed negative byte lengths. Valid runtime records must be nonnegative;
  do not use a corrupted-memory probe as authored parity evidence.
* `000DAD15..000DAD2E` keeps `random16 * float(1/65535)` in x87 PC53 until
  comparing skip chance. Reading the C expression as an already-rounded float
  product can change equality near a skip threshold. This is an intermediate
  precision boundary, not proof that a particular compilation of the recovered
  C differs. RNG draw order must remain coupled to the complete response loop.

The no-response/default-section body branch is therefore a meaningful first
integration slice. It covers actual Chief and marine default body impacts;
the same boundary can request region responses without claiming to implement
them. Parent/rider hierarchies, armor source selection by real mesh collision,
delayed response execution, model permutations, constraints, difficulty rules,
and damage aftermath remain separate acceptance requirements.

Original `D6800` writes body-depleted flag 4 **before** `B8B70` and dirty-update
callbacks. It applies flagged death sections, child and parent handling, then
calls `D6A70`. That routine clears object `+F4`, adds shield-depleted flag 8, and
invokes `BA7F0(object,NONE,2,NONE)` after an optional shield-depletion effect.
Neither routine itself zeros the body or shield fraction. A body-only oracle
whose `D6800` callback writes just flag 4 must explicitly exclude these other
state writes; a production depletion adapter should apply known shield writes
and retain the broader callbacks as unresolved services. Primary evidence:
original `D6800..D6A5F`, `D6A70..D6AEB`, `damage.cpp:626..700`.

## Direct cache fixtures

These values are read from Cairo Station
`assets/disc/maps/01b_spacestation.map`, SHA-256
`5f79832ce3ae8035e547c5c673008802a4971fbf60551755fcc0553482843ae0`.
File offsets are physical cache offsets, not executable virtual addresses.

| Biped/model | Info offset | Flags | Default section | Body maximum | Body material | Shield material | Stun threshold / seconds |
|---|---|---|---|---|---|---|---|
| `objects\characters\masterchief\masterchief` | `EA44484` | `3` | 0 | 30 | 56 | 147 | 0 / 10 |
| `objects\characters\marine\marine` | `E801BFC` | `0` | 0 | 100 | 9 | -1 | 0 / 4 |
| `objects\characters\miranda\miranda` | `E868834` | `80` | 0 | 50 | 9 | -1 | 0 / 0 |

Chief default section at `EA4457C` and marine default section at `E801CF4`
both have flags 0 and vitality share 0. Miranda's default section at `E86892C`
has flags 0 and vitality share 1. All these default sections have no response
entries. Chief's head section has flag `80`, share 0; marine's head section
has flag `80`, share approximately .1. Across the 30 local maps, the scanner
found 11 resident cannot-die (`info.flags & 80`) biped records, all Miranda.
Resident records are not a complete inventory of externally referenced tags.

Cache evidence:
[body-cache-research.json](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/body-cache-research.json),
[body-invulnerable-cache-research.json](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/body-invulnerable-cache-research.json).
The scan uses the repo's version-8 pointer/bounds validation in
`tools/audit_rendering.py::Cache` and reads original bytes.

## Reproducible whole-routine fixture

[body-impact-research.py](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/body-impact-research.py)
executes whole `D8CB0`, `D9020`, `D5BC0` including original search/comparator,
`D5D20`, `DB090`, no-response `DAAA0`, childless `DB110`, and childless
`D6800/D6A70/D5B60` unchanged. Only `1E9720`, `176780`, and `BA7F0` are supplied
services. `B8B70` runs unchanged because the synthetic header omits flags that
would admit its visual/spatial services. The fixture uses nearest-even x87
PC53 (`FPCW 027F`) and normal SSE `MXCSR 1F80`.

The synthetic allocation includes the pool/header/object, runtime matg material
and damage tables, original-style tag table with object definition/model/damage
info chain, one damage section and eight-byte object section state, hit entry,
event, definition, accumulator, time and game globals. Object sibling, child,
parent, actor and update datums are NONE. A parent-free vehicle fixture can
also execute the original child iterator on an empty list; arbitrary hierarchy
cases need additional object records or an explicit `DB110` supplied boundary.

Run from the repo:

```sh
.tools/python/bin/python analysis/systems/body-impact-research.py
```

Its 27 deterministic original-instruction probes include the cache-derived
Chief/marine/Miranda values, ordinary and lethal damage, existing depletion,
zero/negative damage, explosive/object immunity, absorbing/invalid sections,
campaign-player headshot exemption, AI/multiplayer headshots, category/difficulty
bypass, stun sentinel/equality, impact effects, nonidentity armor, duplicate
source groups and invalid material. Results are in
[body-impact-research.json](/home/eric/Developer/Halo-2-Decompiled/analysis/systems/body-impact-research.json).
This is independent original-instruction evidence, **not a native differential**.

Observed anchors suitable for native fixture assertions:

* Chief damage 5 / maximum 30 gives normalized damage `0.1666666716337204`,
  body `0.8333333134651184`, stun 300 at 30 Hz.
* Marine damage 5 / maximum 100 gives normalized damage `0.04999999701976776`,
  body `0.949999988079071`, stun 120.
* Miranda damage 100 / maximum 50 retains body bits `38D1B717` while the
  requested normalized damage remains 2.0; no death request occurs.
* Lethal 100 / 30 leaves body `-2.3333334922790527`, sets original body/shield
  flags `000C`, preserves shield fraction .75 and clears `+F4` from .5 to zero.
* Stun `.15f * 30` first rounds to float 4.5, then nearest-even produces 4
  ticks, including overwriting a prior `7FFF`. A double product or `round()`
  can produce a different result.
* Damage 8 with two .5 armor modifiers leaves remainder 6 and requested body
  damage approximately .06666667. Repeating the same source group applies both
  modifiers again, leaving remainder 7.5. A negative armor product does not heal
  body; its negative post-material damage can increase the remaining damage.

For a native differential, compare body float bits, signed stun, preserved and
updated flags, normalized requested damage, remaining damage, reporting byte,
section callback arguments, depletion/effect requests and callback order.
Separate supplied callbacks from executed original routines in the report.
Passing this boundary does not establish real hit-region selection, full
region responses, complete death behavior, splash, weapon timing, or campaign
combat parity.
