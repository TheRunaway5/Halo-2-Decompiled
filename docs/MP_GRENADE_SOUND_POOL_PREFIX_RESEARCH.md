# Original grenade sound-pool constructor and incomplete map reset

On the same independently reproduced [admitted grenade owner](MP_GRENADE_EFFECT_POOL_BEGIN_RESEARCH.md), complete original `18A200` creates the missing looping-sound pool and its state storage. Original `18A2A0` then resets those records but stops at the first write through the absent audio driver. This establishes real pool construction, not completed sound-map activation or a returned grenade projectile constructor.

The previous [partial projectile freeze](MP_GRENADE_OWNED_PREPARATION_RESEARCH.md) remains unchanged: its `B8A30` call has not returned. This experiment starts from the admitted begin before that separate continuation, explicitly invokes only the two sound-pool owning entries, and does not resume the partial allocator or allocate a second grenade. The bounded entry order is a new declared composition, not execution of the complete startup/map dispatcher.

## Authority and unchanged context

`tools/oracle_mp_grenade_sound_pool_1_1.py` executes untouched disc-update 1.1 instructions from `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout remains `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. No 1.5 equivalence or full match behavior is inferred from this 1.1 composition.

Before adding the two original owners, each run exactly reproduces all 433 calls, 78 states and 28,965 ordered stores of the immutable admitted begin. The same original source-created player `EC700000`, Spartan `E26F0000`, SMG `E2700001`, action component, marker/attachment state, negative local PVS, fixed clock and RNG bytes are retained. The existing whole Wine/Cxbx compatibility thread context, real host storage and inherited unavailable service boundaries remain explicit. No new gameplay field, callback return, initialized flag, capability, sound admission, voice or playback result is supplied. These two entries invoke no new host service or boot/DSP/title-heap chain.

## Executed owning stores

The actual allocator pointer is `82D48288`, backed by the existing original arena at `82D47000`. These recorded addresses belong to this fixture; native code must not copy them as constants.

| Original owner | Measured result |
| --- | --- |
| `18A200 → allocator virtual method → 16D070` | Allocates `60CC` bytes, initializes name `object looping sounds`, capacity 1024, stride `18`, alignment exponent 0 and real allocator |
| `18A233` / `18A243` | Sets pool flag bit2 and publishes `[4F485C]=82EF5548` |
| `18A251 → 1656A0 → 18A272` | Advances the real arena by `244` state bytes, updates its checksum, publishes `[4F4858]=82EFB614` |
| `18A27A` | Actual constructor return; ESP delta4 |
| `18A2B5 → 16D220` | Original pool-valid byte0→1 and complete pool reset; no assigned validity |

The full `60CC` pool and `244` state buffers are retained before and after reset. The actual constructor leaves pool-valid byte `+29=0`, data pointer pool+`4C`, bitmap pointer pool+`604C`, empty 128-byte bitmap and source-zeroed state storage. The reset writes its valid byte, salt/reset fields, every slot salt and bitmap through original `16D220`.

Original `18A2A0` state stores are:

- `+000..+01F`: eight DWORD `FFFFFFFF`; `+020/+024`: zero.
- `+028..+127`: 64 zero DWORDs; `+128..+227`: 64 DWORD `FFFFFFFF`.
- `+228/+22C/+230/+234`: literal source float bits `3F800000`, read from `465030`.
- `+238`: `1F40`; `+23C`: zero. `+240` retains the earlier original arena-clear output; this owner does not write it or establish its meaning.

Arena used/checksum move from `001AE548` / `08010D56` to `001B4858` / `B8FF067C` during the constructor. Reset does not allocate further storage. Original action remains phase1, counter1, release8, bit22, counts `[2,0]`, heldNONE/type-1; object records, clock and seed are unchanged by both sound entries.

## Genuine stop

At `18A35C`, original `2230E0` consumes state+`228`. Its actual clamped first gain has bits `3F800000`. `223106` loads global `[526474]=NULL`; `22310C` attempts `MOVSS [ESI+2ABC],XMM3`, producing an invalid four-byte write to address `00002ABC`. No store is completed and neither `2230E0` nor `18A2A0` returns.

The later `18A366 → 18A790` preference tail is not reached. An original-written pool-valid byte is not evidence of completed activation when its owning reset remains unreturned. Device construction, saved audio settings, driver capabilities, admission, playback and real consumption remain unavailable as documented by the prior audio freezes. Supplying a driver pointer or skipping this gain store would change the boundary and is not part of this experiment.

## Independent evidence and native limit

Canonical pair: `analysis/multiplayer/integration/grenade-sound-pool/measured/report.json` and `repeat/report.json`. They are byte-identical (45,183,552 bytes), SHA-256 `9c587bcde2735fcccdb74baba633fa8104bad1a49b7fe6d5a4530b4a8ebb1f36`. The auditor compares full buffers and all captured source calls/states/reads/stores/points, replays ordered reset writes into the complete constructor buffers, verifies every retained instruction byte against the original XBE and checks all prior pins. Separate whole-entry listings preserve the blocked instruction and unreachable tails as static evidence; those tails are not claimed executed.

Reproduce into fresh destinations, preserving the frozen pair:

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_mp_grenade_sound_pool_1_1.py --output analysis/multiplayer/integration/grenade-sound-pool/measured/report.json
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_mp_grenade_sound_pool_1_1.py --output analysis/multiplayer/integration/grenade-sound-pool/repeat/report.json
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/audit_mp_grenade_sound_pool_1_1.py --output analysis/multiplayer/integration/grenade-sound-pool/audit/audit.json
```

The narrow constructor/layout and reset-prefix contract can be consumed within these declared inputs. It does not authorize successful source sound-map admission, resuming `B8A30` past an unreturned reset, held grenade phase2, release/fuse/explosion or claiming live Q grenade delivery. No production, renderer, shader, release or prior immutable file is modified.
