# Original grenade begin with source-owned effect pools

The separately composed original grenade begin now returns AL1 on the genuinely allocated Spartan and starting SMG. Original `1775D0`→`175860` and `177690` construct and reset the missing effect pools. Actual `E95E0` creates an effect handle, then the original visibility/removal path retires it and returns NONE to its caller. Begin continues, sets action bit22 and returns. The subsequent ordinary retained-action update advances counter0→1. This establishes bounded begin and its first update; grenade creation, release and a live Q handler remain open.

## Authority, inputs and original producer order

The executed original disc1.1 image is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. This experiment dynamically executes original1.1, independently of the previously mapped1.5 grenade owners.

`tools/oracle_mp_grenade_effect_pool_1_1.py` first reproduces the unchanged source-created command/world fixture byte-identically. It restores that fixture's complete guest memory and CPU state. Original allocation, join, spawn, loadout, registry, PVS and physical producers retain player `EC700000`, Spartan `E26F0000`, SMG `E2700001` and its actual inventory/body/manager/marker state. The state before the new effect constructors, excluding the new observer fields, is exactly the state before the earlier [bounded grenade experiment](MP_GRENADE_BOUNDED_PHASE_SOURCE_RESEARCH.md). The declared neutral and Q command records also match that experiment exactly.

The new composition executes:

1. Complete original `1775D0`, including its tail jump to complete `175860`. The existing source-produced allocator at `51841C` backs their real allocations. `16D070` owns all pool headers and data/bitmap pointers; no pool pointer or handle is assigned by the tool.
2. Complete original `177690`, including the actual `16D220` resets. It owns pool byte+29 validity and pool lifecycle state.
3. The unchanged bounded neutral phase, which owns SMG state+1789→0 through `1021D0`→`1099B0`→`109A60`, writer `109B82`.
4. The same original `CD310` selector0 and declared incoming Q flag `04000000`, through `14E780`→`852A0`→`69350`→`14EAE0`→`150F00`→`E7F30`→`E95E0`.
5. The existing bounded blend/control/motion/weapon/pose/postpass roots. In particular, ordinary `C7E90`→`E7E60` invokes the admitted type22 `E99B0` update.

This is an explicit composition of owning original roots. It does not establish full startup callback order, resume the separate full-world texture stop or replace the inherited fixed game clock. The original clock remains tick0/dt bits `3D088889`; the shared seed remains `12345678`. No time, seed, current marker, attachment, PVS, local membership, audio flag, action, count, animation result or admission return is written by this extension.

## Exact effect result and returned action

| Original owner | Captured result |
| --- | --- |
| `1775D0` / `175860` | Six source-created pools; outer constructor RET ABI is inherited through its original tail jump, ESP+4 |
| `177690` | Complete real reset returns, ESP+4 |
| `115650` | Real unit graph `E9180793`, selected throw clip219 `combat:rifle:throw_grenade:var0` |
| `179B90` | Real effect datum `E6650000`, ESP+12 |
| `17B1A0`→`179080` | Actual empty local-PVS visibility path retires the effect; both return AL1 |
| `1781F0` | Genuine EAX=`FFFFFFFF`, ESP+20 |
| `E95E0` | EAX=`00400001` (AL1), ESP+12; source `E993B` stores active word `00400000` |
| Subsequent `E99B0` | AL1, ESP+12; phase1 counter0→1 |

Before the effect request returns, original `E9896..E98C6` has produced phase1, counter0 and release timer8 through the actual channel-duration owner `1CA8D0`/`148160`. The actual count remains `[2,0]`, selected/requested type0 and held object NONE. After `1781F0` returns NONE, source `E9913` restores the component pointer, consumes the optional original definition callback, then `A99F0` returns and `E992E..E993B` sets bit22. This branch does not require a positive effect handle or invented effect-success reply.

The effect is not skipped. Original allocation, world marker lookup, location attachment, visibility decision and removal all execute. The report retains each full effect/location record at the observed boundaries, all ordered guest stores, complete final pool storage/bitmap bytes and the unchanged owner bodies. All six pools finish empty.

Lockout's actual begin effect `E7A60621` is `sound\\weapons\\frag_grenade\\frag_throw`: root flags2, one location, one event, one part referencing original sound `E7A70622`, and zero particle systems. Plasma begin effect `E7B0062B` similarly contains sound `E7B1062C` and zero particle systems; its metadata is recorded, but this run throws only the actual available frag. With the genuine inherited nonlocal view/PVS state, original `17B1A0` reaches `11D620` and retires the frag effect. Neither `18AA90` nor `18AD70` executes. This is a measured nonlocal negative visibility result, not evidence of local throw sound playback or an audio/hardware capability.

The source-produced pools are:

| Global | Captured pointer | Capacity | Stride |
| --- | --- | --- | --- |
| `4F1F0C` | `82EBA050` | 256 | `190` |
| `4F1F08` | `82ED30BC` | 512 | `3C` |
| `518464` | `82EDA948` | 128 | `54` |
| `526514` | `82EDD3A4` | 1024 | `40` |
| `526518` | `82EED470` | 256 | `4C` |
| `52651C` | `82EF20DC` | 256 | `34` |

These are captured source outputs, not addresses a native caller should install. An effect provider must own its actual storage/lifecycle and preserve the request/result contract.

## Repeatability and limits for native integration

Canonical independent runs are `analysis/multiplayer/integration/grenade-effect-pool/measured/` and `repeat/`. Both use the new tool without changing prior frozen tools, reports or production files. The independent audit checks identical complete report/listing bytes, the unchanged owner/input comparison, original instruction bytes, returned ABIs, actual bit store, effect allocation/removal order, fixed clock/seed and all earlier frozen source pins. `exploratory-01/` retains an initial observer-only failure: it reached the positive guest outputs but did not recognize the constructor tail-return. It is excluded from the canonical evidence.

```sh
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_mp_grenade_effect_pool_1_1.py --output analysis/multiplayer/integration/grenade-effect-pool/measured/report.json
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/oracle_mp_grenade_effect_pool_1_1.py --output analysis/multiplayer/integration/grenade-effect-pool/repeat/report.json
PYTHONDONTWRITEBYTECODE=1 .tools/python/bin/python tools/audit_mp_grenade_effect_pool_1_1.py --output analysis/multiplayer/integration/grenade-effect-pool/audit/audit.json
```

The inherited compatibility-produced Wine/Cxbx KPCR, raw storage/file/resource/physics services and optional feedback boundary remain explicit. Only the inherited negative `115650` animation service is removed after unchanged preparation, allowing real channel selection; no new source return is supplied. In particular, inherited `EFE30` unavailable feedback remains a declared leaf and is not promoted to real audio delivery.

A native begin consumer can use this exact returned action once the retained44h component, real unit-manager channel selection/duration, original gates, scope reset, owning callbacks and effect provider are connected. Its effect provider must not universally return NONE from this nonlocal case. The existing lower `unit_grenade` module still describes the earlier partial-begin evidence and is not modified here.

The begin path also executes original cancellation `E9A50`→`E8F30` against actual heldNONE before starting the new phase1. That empty branch writes phase3 and returns; it does not release an allocated grenade. The next retained update reaches preparation at counter2. Genuine `E8D60` count debit and a constructor request were separately measured in the [callback experiment](MP_GRENADE_NATIVE_CALLBACK_RESEARCH.md), but not yet through this admitted action's next ordinary update. Actual grenade allocation/attachment, `E8F30` held release, launch velocities, fuse/bounce/explosion and custom native LAN replication still require their owning chains. A completed begin/first update alone does not make Q work in the native simulation.
