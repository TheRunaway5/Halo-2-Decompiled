# Original grenade begin after a bounded unit/weapon phase

This independent experiment executes the original grenade-animation request on the genuinely allocated Spartan and starting SMG. It reaches a real throw channel and writes the initial grenade phase, then stops at the unavailable effect pool. It does **not** establish a returned/admitted complete action, created grenade, count debit, release, trajectory, fuse, explosion, continuous world clock, graphics readiness or native gameplay integration.

The complete-world negative request and texture-constructor result remain immutable in [the earlier document](MP_GRENADE_ACTION_SOURCE_RESEARCH.md) and `analysis/multiplayer/integration/grenade-action/manifest.json` (`cb72c6ec1bfc4e5045c6bb537dc6f5e4fc91123616b18abbcfb5e5b0277493bf`). This experiment neither resumes nor bypasses that complete-world stop. No production files change.

## Authority and actual ownership

The dynamically executed image is the user-owned disc1.1 update `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. Lockout SHA-256 is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. The older source-created command composition is reproduced byte-identically before restoring its complete original guest memory and CPU context.

Original allocation/join/loadout/registry/PVS/world/physical producers own player `EC700000`, Spartan `E26F0000` and starting SMG `E2700001`. The actual source inventory slot at unit+218 contains that weapon; no datum, header, high-water, body, inventory identity, ready state, action component, pose or resource default is assigned. The Wine/Cxbx whole-KPCR record remains compatibility-produced context, not ordinary Xbox hardware/thread authority. The inherited local membership, session, platform, resource, raw storage and feedback boundaries remain recorded.

## Explicit composition and original clock limits

`tools/oracle_mp_grenade_bounded_phases_1_1.py` first runs original `CD310` against source counts and obtains selector0. It executes original `14E780`, declares only command+0 flags and signed-short+22 selector, then composes original `852A0` submission, `69350` assembly and `14EAE0` player delivery.

One neutral continuation explicitly composes:

1. Original unit `1D0ED0` blend, only when the actual signed unit+12A manager offset and manager+68 pass the original `BD360` gates.
2. Original biped table callbacks `C7E90` then `DE960`, with their real unit datum. `DE960` builds its own animation request; no fabricated `112E10` request is supplied.
3. The same original blend gate for the source-owned weapon, followed by actual `1021D0` own update.
4. Original `BDF80` local pose update for the unit and weapon, followed by original `1019F0` projectile postpass with no declared fire request.

This is an explicitly bounded composition of actual roots, **not** original recursive `BD360` traversal or a complete `B7E00`/world tick. It excludes the separately blocked attachment texture path instead of providing a success return for that path. The original game clock record is unchanged in every captured state: pointer `01016000`, complete24h bytes `00001e008988083d00000000000000000000000000000000000000000000000000000000` (dt bits `3D088889`, tick0). No tick, frame, duration or clock field is written. Consequently a phase index here is not a recovered naturally advancing match tick or an authored animation-duration claim.

During this neutral phase, the actual SMG own state+178 changes9→0. The actual writer is original `109B82`, reached by `1021D0`→`1099B0`→`109A60`; the complete body and ordered stores are retained. `1099B0` itself excludes own states7/8/10 and calls `109A60(weapon,0,1)` for the executed state9. This demonstrates the producer in this exact context rather than assigning an idle value or assuming a15-tick initial delay.

## Actual Q prefix and first stop

The next declared command has flags `04000000` and selector0. Actual `150F00` builds the original type22 request. Original `E95E0` consumes it:

- `1055F0` returns AL0 with actual own state0; `CF440` returns the actual count2. Existing constructor counts remain `[2,0]`.
- The source follows its ordinary channel branch at `E9873` and calls actual `115650` with unit `E26F0000`, request identifier `0D000021`, raw scalar argument `3E08B439` and a real stack output. Only the inherited negative `115650` service is removed **after unchanged preparation**. No channel return or animation selection is supplied.
- Actual `115650` returns AL1 and produces channel pointer `82D89512`. Its original record contains graph `E9180793`, identifier-1 and index219. The unchanged authored graph record identifies clip219 as `combat:rifle:throw_grenade:var0`; that metadata is pinned in the earlier Spartan pose report.
- Original `E9896` writes action component+0C phase1. `E989A` clears counter+0E, `E989E` clears release timer+0F, and `E98C6` writes the actual original `1CA8D0`→`148160` result8 to+0F. This is a captured source result, not a chosen throw delay. The call operand consumed by `148160` has bits `3E888889`.
- Original `C9A40` executes the source scope-reset path. The retained original `EFE30` feedback boundary remains explicitly inherited and is not newly closed.
- `E990E` requests actual effect `E7A60621` through `1781F0`. The original chain reaches `179B90`, whose `179BD8` loads pool global `4F1F0C`, then passes that NULL pointer to `16D2C0`. The first unavailable instruction is `16D2C3`, reading `00000034`. The request is recorded; no pool pointer, allocation result, callback, effect success or throw success is supplied.

At that stop, complete action44h bytes are `00000000000000000000000001000008ffffffffffff0000ffffffff00000000000000000000000000000000ffffffff0000000000000000000000000000000000000000`. Current/requested selector0, counts `[2,0]`, active words+4/+8 zero, phase1, counter0, release timer8, held objectNONE and held type-1 are genuine source outputs. No `E8D60`, grenade allocator or retained `E99B0` update executes after this request. Source `E992E..E993B` would set active-word bit22 only after the unavailable effect request returns; **that store is not executed here**. Therefore the partial phase must not be promoted to an admitted action or a working Q handler.

## Return observation and frozen reproduction

Recursive `E7F30` dispatch reuses return address `E7F49`. The new observer checks both the return address and actual ESP delta against the callee's original RET ABI. It does not mistake a nested action return for completion of outer `E95E0`. The stopped outer callback remains unreturned in the canonical report. Earlier immutable tools are untouched; preliminary22-phase runs under the older observer are exploratory and are not the canonical freeze.

Canonical outputs are `analysis/multiplayer/integration/grenade-bounded-phases/measured-complete/` and `repeat-complete/`. Reproduce both with separate invocations:

```sh
.tools/python/bin/python tools/oracle_mp_grenade_bounded_phases_1_1.py --output analysis/multiplayer/integration/grenade-bounded-phases/measured-complete/report.json --neutral-phases 1
.tools/python/bin/python tools/oracle_mp_grenade_bounded_phases_1_1.py --output analysis/multiplayer/integration/grenade-bounded-phases/repeat-complete/report.json --neutral-phases 1
.tools/python/bin/python tools/audit_mp_grenade_bounded_phases_1_1.py --output analysis/multiplayer/integration/grenade-bounded-phases/audit-frozen/audit.json
.tools/python/bin/python tools/freeze_mp_grenade_bounded_phases_1_1.py --output analysis/multiplayer/integration/grenade-bounded-phases/manifest.json
```

Report, full object/action/clock buffers, ordered reads/stores, source requests, return-stack checks and instruction listings must match independently. The audit verifies each listed instruction against the pinned original image and all earlier manifest pins. Original1.1 execution is separate from the previously frozen complete1.5 correspondence of grenade owners; no complete1.5 world composition or native throw integration is claimed.

The next genuine dependency is the original effect-pool owner/lifecycle, followed by a normally returning begin callback and actual retained grenade update/creation/release. This task stops at the measured pool request; it does not extend graphics/cache work or substitute a projectile trajectory.
