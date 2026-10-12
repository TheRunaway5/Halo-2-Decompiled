# Original texture owner before canister particle birth

The separate original canister composition now completes actual texture-cache
constructor `12D840` and reset `12D950`, followed by the previously recovered
decal constructor/reset. The required decal requests real texture IO and stops
at the absent async owner's NULL mutex. No IO completion, GPU readiness, effect
part or particle birth is supplied. Production and earlier freezes are unchanged.
[New tool](../tools/oracle_canister_texture_cache_20261010.py),
[measured report](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/report.json),
[preceding vertex-cache closure](MP_CANISTER_VERTEX_CACHE_RESEARCH.md).

## Genuine constructor and reset outputs

The bounded original world/canister factory, player join/spawn and PVS reproduce
the preceding source preparation. Inherited return-only services are removed
before the new roots. Actual `12D840` uses its original allocator and executes
`11E150`, the unchanged VirtualAlloc wrapper `33785C`, `16D070` and `13E8E0`.
The only raw allocation boundary is original ordinal184. It uses the same
declared Linux shared-storage protocol as the preceding vertex experiment,
accepting only the four actually requested sizes with the literal
`(NULL,size,MEM_COMMIT|MEM_TOP_DOWN,PAGE_READWRITE)` arguments. Real mappings are
64KiB aligned, page rounded, zeroed by Linux, shared directly with Unicorn and
checked against host bytes. Actual failed allocations are not replaced by a
Source success/failure value.
[Complete texture constructor](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0012d840.asm),
[raw allocator](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0011e150.asm),
[wrapper](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0033785c.asm).

| Original request bytes | Actual page extent | Actual shared base | Source owner |
| --- | --- | --- | --- |
| `508C` | `6000` | `7E420000` | `4EDA24`, texture pool |
| `6A8` | `1000` | `7E410000` | `4EDA28`, predicted-texture pool |
| `30FC` | `4000` | `7E400000` | `4EDA34`, texture page allocator |
| `C` | `1000` | `7E3F0000` | `510C34`, later decal vertex object |

`12D950` derives capacity from the existing source physical-arena fields. It
requests `MmSetAddressProtect(80061000,02CE6000,404)` through original thunk417BA8,
return site12D9C3. The previously frozen compatibility library is loaded
read-only; its original conversion and host-facing protection bodies execute.
The derived PTE mask is6B, patched Xbox/converted Windows flags440, host access40.
The supplied transport **actually applies** Unicorn permissions7 to the existing
47,079,424-byte range and verifies complete before/after contents have the same
SHA256 `67ed78a75d11fd86425364d78a84ef8e3bd7b571fee3548ea80e51f63debf891`.
It preserves general registers and supplies the original void ABI with no
fabricated status. Xbox hardware cache/write-combine/PTE behavior is excluded.
[Existing primary/protocol proof](MP_LOCAL_MAP_PROTECTION_RESEARCH.md),
[read-only compiled conversion bodies](../analysis/multiplayer/integration/spawn-source/local-map-protection/compiled-compatibility-access.cpp),
[actual new protection call](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/report.json).

The unchanged reset then owns `4EDA30=80061000`, calls `13F020` with source page
count2CE6, marks both pools valid and executes their actual `16D220` resets.
No capacity, validity bit, descriptor, allocation identity or texture readiness
is assigned by the harness. Every stage retains the complete source allocations
508C/6A8/30FC, full rounded host/guest backing, texture globals, complete decal
and vertex state, effect pools and world records.
[Complete reset](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0012d950.asm),
[capacity owner](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0013f020.asm).

## Exact next owner

After the unchanged effect/mask/decal constructors and resets, actual canister
death/update produces effectE6650000 and its location. Source event1's required
decalE70B0596 proceeds through `180980` to its texture request. `12ED10` requests
`13EAE0(7E400000,B00,5)` and receives actual page datumE2780000. It creates the
texture entry at7E42004C, destination80061000, then calls `19D8C0` with encoded
location890D4600 and six stack words `C00,80061000,0,7E42004F,3,2`. This rounds
the authoredB00 transfer toC00 through original instructions.
[Texture request](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0012ed10.asm),
[original file request dispatcher](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/0019d8c0.asm).

Original `19D8C0` decodes the location, selects its actual file-table token
(currently0) and calls `1A4870`; `1A4870` reaches `121FD0`. Original wait wrapper
`2DC8B2` calls ordinal234 at2DC8CF, returning at2DC8D5, with actual NULL handle,
UserMode1, alertable0 and NULL timeout. Execution stops before a wait outcome.
Original async queue/synchronization constructor `1221B0` has not executed in
this composition. The file token/path owner is also required: encoded location
alone cannot be promoted to an opened native file handle. No queued IO, worker
completion, successful texture creation or Source done byte is manufactured.
[Async request](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/001a4870.asm),
[queue acquisition](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/instructions/00121fd0.asm),
[retained stopped frame](../analysis/multiplayer/integration/canister-texture-cache-20261010/measured/report.json).

The next integration needs the separately recovered genuine async constructor,
real synchronization/worker services and actual file token/path binding, while
preserving the source callback's failed-seek/short-read semantics. It must retain
this required decal; skipping it would change the whole authored effect. All
particle-system/emitter/location/particle pools remain empty. Collider1, class8,
whole canister birth, natural lifetime and rendering remain separate gates.

## Reproduction and correction

```sh
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_texture_cache_20261010.py --output analysis/multiplayer/integration/canister-texture-cache-20261010/measured/report.json
PYTHONPATH=tools .tools/python/bin/python tools/oracle_canister_texture_cache_20261010.py --output analysis/multiplayer/integration/canister-texture-cache-20261010/repeated/report.json
.tools/python/bin/python tools/freeze_canister_texture_cache_20261010.py
```

The source sequence has18 roots,17 returns and one exact unresolved wait,
68 observed calls,19,896 non-stack stores and65 instruction files. Reports,
complete source buffers, real platform outcomes and listings are independently
repeated. The new freeze records exact commands/dependencies and audits prior
pins. The inherited recorded compatibility context, bounded factory/resource/
variant/clock/preparation boundaries remain; this is not complete title/frame
scheduling, a natural bullet kill, Xbox hardware, native gameplay or GPU proof.

The separately published
[particle-pool label correction](MP_PARTICLE_POOL_LABEL_CORRECTION.md) supersedes
the old canister prose inversion:526518 is emitter256×76,52651C is location256×52.
Literal original headers and native layouts were already correct; frozen old
files are preserved.
