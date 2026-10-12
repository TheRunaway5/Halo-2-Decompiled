# Bounded native first-person sound request producer

`fp_sound_request.hpp/.cpp` ports the original event/marker/request producer. It
does **not** install a live sound consumer, produce a world pose, initialize a
sound system, select a permutation, allocate a voice, or establish audible
parity. Those are required services. There is no unavailable-service success
default and no fixed sound seed. Existing original audio and animation freezes
are unchanged.

The evidence is in
`analysis/multiplayer/integration/fp-sound-request-native`. Use only
`controls-canonical`, `controls-canonical-repeat`, `u5-final`,
`u5-final-repeat`, `canonical-fixtures.tsv/.json`, and the final native
snapshots/audit. Earlier directories and failed attempt logs are exploratory
records; they are retained but are not canonical inputs.

## Original authority and independent comparisons

The owned map-pack 1.1 executable is SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Authenticated owned u5 English is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Lockout is SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.

The complete-body instruction comparison identifies these pairs, including
internal branches and relocated memory/call operands, without using annotation
comments as behavior. All 767 instruction rows match the pinned executable
bytes. The 25 correspondence/provider files match a separate process repeat.

| 1.1 | u5 | Complete body role |
| --- | --- | --- |
| 1687C2 | 169BA2 | FP event gates and retained hand stores |
| 18AA90 | 18BE50 | Marker/callback selection and request invocation |
| 18AA40 | 18BE00 | Sound flag/object type callback classification |
| 18ACD0 | 18C090 | Marker construction |
| 18AD70 | 18C130 | Audible/request initialization and owner invocation |
| 18EF20 | 1902E0 | Local relationship audibility |
| 18BA10 | 18CDD0 | Play request fields and conditional success owner |
| 18DDE0 | 18F1A0 | World/location/marker owner, required provider |
| 18EEE0 | 1902A0 | Sound metadata/relationship audibility |
| BBEA0 | BCFE0 | Parent-chain relationship |
| 156EB0 | 158290 | Actual source camera mode lookup |

The three authored positive request inputs/outputs are taken unchanged from
the independently repeated prior original sound freeze: ready at tick1,
reload at47, empty reload at124. Original `18DDE0` returns AL1 in all three;
the original unavailable sound system makes `21F220` return NONE. This is
completed rejection, not successful playback. Both reload channels request
`E5AD0438`; the ready event requests `E5AC0437`. The other resident reload sound
name is not a reason to replace this actual event tag.

The new original controls execute five declared event inputs, four complete
marker helper inputs and twelve audibility inputs on the unchanged prior
equipped/registered fixture. Events/helper arguments are explicit controls,
not authored event production. The real unbound view owns weapon NONE and
actually rejects in `18DDE0`; no success or pool state is supplied. All four
camera modes in this fixture are source0, so it does not positively execute
the nonzero-camera/reference-bit8 skip branch. That predicate is retained
from the complete original body. The fixture's terrain/BSP/resource/time and
older engine leaves remain explicit inherited boundaries; it is not a new
whole-game or ordinary map startup proof. No audio bootstrap or driver chain
is reexecuted by these new controls.

## Represented native contract

`FirstPersonSoundState` is the source hand `+1008` identifier and `+100C`
activity. Cold NONE/FFFF comes from original `1677E5` writes at
`167837/16783E`. The module changes these fields only after the complete lower
request returns, even if the real return is NONE. Activity is low16 of the
actual hand `+84`. A missing provider throws before pretending that the request
returned or updating the retained hand. Slot initialization/removal and sound
stop ownership remain separate consumers.

`1687C2` processes event type1 with a non-NONE tag. Camera mode is obtained
from the required source-mode provider. Nonzero mode plus reference bit8 skips
the event. Its source FLD1 produces scale bits `3F800000`; marker index is
NONE. The source marker helper maps this index to byte0=0, copies the two
executable-owned basis records, and sets associated owner NONE. Only sound
byte0 bit40 plus actual header type2 can choose the special callback table;
the marker association additionally requires object `+12C bit0` and non-NONE
`+154`. Header classification is repeated where the original repeats it.

`18EF20` checks four actual local-player slots, source camera-mode mask3,
actual player current-unit identity, and parent relationships in both orders.
`BBEA0` reads the starting object's parent before accepting an initial-object
match. Native code preserves that query order. A malformed cyclic parent
chain is an explicit host bounds failure; it is not given a successful source
return.

The request initializes the original owned fields before the required
`owner_request`: ushort0, byte2=0, requested audibility in both nibbles of
byte3, scale at4, zero at8, NONE at28, FFFF at2C and the **current active BSP**
word at2E. Original `18DDE0` then owns its position, compressed direction,
velocity, packed location and low audibility nibble. `owner_request` receives
the actual owner/tag/callback identity and complete marker. A false result
is the actual owner rejection; an absent function is an unresolved boundary.

The `SourceSoundPlayRequest` has every represented source field: sound,
flags, priority, owner, callback identity, platform argument NONE, defined
location48, marker40 and data size40. For these FP requests flags are2D;
priority is1 for resolved low-nibble0, otherwise3. Class `+A bit7` with clear
sound byte1 bit0 sets copied location byte1 bit3. No forced pitch/permutation
variant is supplied. The required submit provider owns complete original
admission/selection/scheduling and actual platform output. The native module
does not turn a source request into a generic first-permutation `AudioEngine`
play call. After real success, class `+8` bits1 and2 require the separate
original attention/time owner; there is no skipped-success side effect.

Caller marker padding `+2/+3/+1C..+23` and still-unwritten location bytes are
explicit backing. The original leaves these untouched. The tests compare all
40 marker bytes, initialized R48 input/output and represented play fields
using actual recorded backing. Unowned x86 stack bytes in P-A0 and temporary
32-bit stack pointers are omitted from the typed native packet; they are not
invented defaults or truncated 64-bit host pointers.

## Immutable providers and the active-BSP correction

| Record | 1.1 global / target | u5 global / target | Extent |
| --- | --- | --- | --- |
| Marker position | 46FC28 / 447144 | 474CE8 / 44ABEC | 12 bytes, zero bits |
| Marker forward | 46FC48 / 447180 | 474D08 / 44AC28 | 12 bytes, `3F800000,0,0` |
| Ordinary callback table | 44B14C | 44EBF4 | 32 bytes |
| Special weapon table | 44B16C | 44EC14 | 32 bytes |

The globals in the first two rows are **pointers**, not inline vectors. The
create entries in both callback tables are the proved `18DDE0/18F1A0` pair.
Later table callbacks are retained as identities and are not ported here.

Correction to this new study's exploratory provider naming: `46FB64` / u5
`474C24` is the runtime current active BSP word, **not an immutable location
sentinel**. Fresh image FFFF is not an active-map default. The prior original
equipped Lockout fixture supplies terrain bank0 and the complete request
records have R+2E=0000 before the owner. The module therefore requires a
separate current-BSP argument. Original BSP switch evidence is retained in
`MP_ITEM_WORLD_RESEARCH.md` (`11D8ED..11D90E`); this study does not promote the
old fixture's explicit terrain setup into a completed ordinary map-start
producer. No earlier immutable report/document was edited for this correction.

## Validation and next live gates

Pure normal and Clang ASan/UBSan tests pass three authored requests plus all
21 original controls and two native unavailable-provider stops. Normal and
GCC ASan/UBSan tests also pass using actual owned `Cache::load_sound_variation`
raw20/92 metadata for all three exercised tags. Each actual-provider run
repeats the whole request/control suite. Copied observed Cache archives and
executables are pinned; they are not described as a fresh reproducible build
of every current shared source file. The source snapshots and commands make
this new module compilation reproducible with the exact observed archives.

Root's actual local FP event hook can invoke this producer with its real hand
binding/event. Required remaining live inputs are source camera mode,
four local player/current-unit slots, actual object header/parent/association
records, current BSP at event phase, and complete current weapon world-node
pose/location/velocity for `18DDE0`. Its node input is weapon-object node0 for
the FP NONE marker; it is not a first-person presentation matrix or an
assumed unit/player position. Source validation (`BBCB0/BABD0`), `BB1F0`
location/cluster, `B9AC0` current node, `143CE0` position transform, separate
SSE rotation plus `18A800` compression, `BB0C0` inherited velocity,
type0/1 PVS location and type7 BSP-surface branches must retain their actual
owner semantics. Normal stock SMG is type2, but that does not bypass its
validity, deletion, class, world-location or pose gates.

Available native pieces are owned sound metadata/bytes/PC decoding,
`sound_variation` gain/pitch/permutation kernels with explicit supplied RNG
and per-range state, class fades, and a real SDL `AudioEngine`. They do not
constitute complete source `21F220`, listener/distance/codec admission,
source/owner pools, priority/class-cap voice arbitration or callback lifetime.
Actual platform device/handles can belong to an SDL adapter, while source
RNG/state/order/listener/admission and original cleanup remain source owners.
No new Xbox driver bootstrap experiment is needed to establish this boundary.
The original activity-stop owner (`168483 -> 127AB0`) is also required before
claiming complete request lifetime; substituting immediate SDL stop is not
proved by this module.

Reproduce into new paths, keeping the frozen inputs unchanged:

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_fp_sound_request_controls_1_1.py --output /tmp/halo2-fp-sound-controls-new
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/research_fp_sound_request_u5.py --output /tmp/halo2-fp-sound-u5-new
.tools/python/bin/python tools/export_fp_sound_request_fixtures.py /tmp/halo2-fp-sound-controls-new/report.json /tmp/halo2-fp-sound-u5-new/providers.json /tmp/halo2-fp-sound-new.tsv
```

The final audit/manifests pin the source snapshots, original inputs, repeats,
commands, compile/run outputs and validation limits. This checkpoint makes
the bounded producer reviewable; `live_first_person_sound_playback:false`
remains its status.
