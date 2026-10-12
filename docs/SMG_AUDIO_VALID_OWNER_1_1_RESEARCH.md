# Original 1.1 sound cleanup with a source-allocated equipped SMG

This independent composition produces the player, biped, carried SMG, audio
owner pool, sound request, cache entry and unvoiced source through their owning
original instructions. Complete original sound removal returns for both a
pending ready resource and a loaded reload resource. No object header,
high-water mark, datum, ownership link, pool-validity flag, callback return,
driver, voice or successful admission is supplied to these consumers.

The actual weapon **passes the index and signed-salt checks**. Callback
`18E2F0` then requests mask 3, which accepts object types 0 and 1; the equipped
weapon is type 2. `BBCB0` therefore returns NULL after type validation.
`D2270` does not execute. This closes the valid-weapon lookup branch and the
complete unvoiced removal around it, while unit sound-slot clearing remains
unproved. The preceding [invalid-index experiment](SMG_AUDIO_OWNER_POOL_1_1_RESEARCH.md)
is unchanged and retains its distinct boundary.

Ordinary audio admission still rejects the request because the original
initialized flag is zero. The direct source-creation call is an explicit
bounded kernel input after that genuine rejection. No complete first-person
clock, local binding, sound-map activation, asynchronous thread, driver,
playback or audible output is proved. Production and all 98 prior frozen pins
remain unchanged; every new report retains `native_gameplay_update_applied:false`.

## Authority and independent execution

The source is the user's map-pack-disc 1.1 executable
`assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout is `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`;
shared.map is `084c817fa82bb9bc416d9ba847cec05be431abc419e3a1087cdf211fbf47f5af`.
This is disc 1.1 evidence; a later update is not inferred.

The independent [oracle](../tools/oracle_smg_audio_valid_owner_1_1.py) composes
the unchanged `OwnerTrace` raw audio providers with the unchanged `PlayerTrace`
owning preparation. It does not invoke the old audio case preparation or copy
its registry. The [measured report](../analysis/multiplayer/map-pack/smg-audio-valid-owner/measured/report.json)
and [fresh-process replay](../analysis/multiplayer/map-pack/smg-audio-valid-owner/reexecution/report.json)
retain complete source states, calls, ordered writes, resource requests, input
boundaries and original instruction bytes. The replay uses precisely the
recorded five host clock requests per case and unchanged whole compatibility
context. The [auditor](../tools/audit_smg_audio_valid_owner_1_1.py) compares source
outputs after excluding only named provider/context mode labels and actual
host allocation addresses. It does not omit object, audio or ownership fields.

The measured listings contain 41 files and 36,693 instruction rows, including
33,410 unique executed original addresses. Each retained instruction is checked
against the original XBE and both unchanged emulator images. Dependency-stop
and replaced provider entry instructions are excluded from the executed set;
their static listings remain separate inspection evidence. Exact hashes and
verification counts are retained in [the suite](../analysis/multiplayer/map-pack/smg-audio-valid-owner/frozen/suite.json)
and [audit](../analysis/multiplayer/map-pack/smg-audio-valid-owner/audit.json).

## Owning world and loadout composition

The unchanged [player/world preparation](MP_PLAYER_VISIBILITY_RESEARCH.md)
executes the recovered heap, CRT lock/on-exit, static engine, game-arena,
player, physics, object-header/body, unit-queue, trigger and bounded AI
constructors/resets. Original `125328/125330` publish allocator `51841C`
and its original vtable before later consumers. The unavailable `217090`
savegame/thread I/O call remains excluded using the original preserved frame;
it receives no fabricated return. Original RNG constructor `1479B0` executes
after that arena producer and before player storage. Host clock measurements
and CRT initialization determine its outputs; there is no supplied seed.

The actual source-generated default variant and host-effective property bytes
are delivered as game-options/creation inputs. The creation identity is the
unchanged zero-key nonlocal input. Sign-in, lobby, transport and complete
session producers remain open. Selected source callbacks and their ordering
are inherited evidence, not a complete title startup dispatcher.

Actual `158C10`, `14D350 → 14D800 → 14D600 → 14D3D0`, and `1509C0`
return. The source spawn path executes the allocator, model/body and
`1A28A0` loadout/attachment consumers; the early `14EAE0` cluster fragment
retains its original dependency stop. This composition deliberately calls
`spawn_and_activate(..., activate=False)`; it does not add the separate PVS or
motion phases from the visibility suite.

| Source-created record | Retained result |
| --- | --- |
| Player | `EC700000`, current unit `E26F0000` |
| Spartan biped | `E26F0000`, 5750 bytes, definition `E9F2086D`, physical component `E1680000` |
| Equipped SMG | `E2700001`, 1260 bytes, definition `E57B0406`, parent `E26F0000` |
| Unit inventory `+218` | Actual allocated SMG `E2700001` |
| Object header pool | 2048 slots, stride 12, high-water/count 2 |
| Actual weapon header | `70e2aa020900ec046096d882`: salt `E270`, type 2, size `04EC`, backing `82D89660` |

The recorded addresses are outputs of this compatibility backing/layout,
not retail memory constants or native inputs to copy. The complete header
pool, bitmap, player records, biped and weapon bodies are retained before and
after sound removal. Source `14E37C` binds the player to the allocated unit;
source `FB18F` binds the weapon to it. No newer registry or selected identity
field is copied into an old fixture.

The real SMG graph/resource bytes are imported from their original files.
Used loadout leaves `178120 → NONE`, `C10E0 → NONE` and `170060 → 0`
remain explicit optional attachment, looping-sound and audio boundaries from
the unchanged player preparation. This experiment does not promote those
returns into source subsystem behavior. All inherited services are retained
verbatim in each new case.

## Audio owner production and real service boundaries

The audio slice retains the original [owner allocator/pool contract](SMG_AUDIO_OWNER_POOL_1_1_RESEARCH.md).
Actual `18A200` allocates and writes the 1024-slot, stride-24 owner pool and
244-byte state through the real source-produced game-arena allocator.
Actual `18A2A0` writes validity and reset fields itself. In this new allocation
order the pool is `82EBA3B8` and state `82EC0484`; neither pointer is assigned
as a supplied constructor result.

| Original reached stop | Genuine dependency left unavailable |
| --- | --- |
| `126B20 → 2205C0` | Device constructor |
| `126DE0 → 2217E8` | NULL driver `[526474]`, pending store at driver `+2A0C` |
| `1221B0 → 2D8606` | Asynchronous CreateThread request |
| `18A2A0 → 22310C` | NULL driver, pending gain store at driver `+2ABC` |

Those partial prefixes are individually entered in the recorded bounded
order. Their stops do not count as completed map or device activation.
Original class-fade constructor/reset `223960/2239B0` also execute.
The source cache/page/async constructors write their own allocation, queue,
pin and ready fields. Real raw storage, clock, lock, semaphore and verified
file reads are declared host providers. The provider writes file bytes and
actual read length; original `1223AC` publishes completion.

The real provider file handle is explicitly bound at `55F798`; its original
file-registry producer remains unexecuted. One original `122360` worker
iteration is invoked as a bounded root when needed; the second wait ends the
slice. This is no claim of asynchronous timing, Xbox I/O latency or a real
source-owned worker thread. The unchanged Wine32 worker stays alive during
measurement. Its whole KPCR, ETHREAD, native stack and static process output
is transferred unchanged, with no pointer relocation or selected-field patch.
That is compatibility-produced context, not retail Xbox thread/TLS authority.

Saved `XC_AUDIO`, boot AV-pack, MMIO, capabilities, DirectSound/SDL device and
voice ownership, packet consumption and playback remain the unavailable
boundaries from earlier freezes. No partial device pointer, successful
capability query or preference is inserted into this trace.

## New request produced from the real allocated owner

The authored callback input is ready `E5AC0437` or reload `E5AD0438`, each
verified against the actual reference in FP graph `E5AA0435` and its raw
20-byte `snd!` definition. Direct callback-root `18AA90` receives actual
weapon `E2700001`, marker `FFFFFFFF`, and explicit gain bits `3F800000`.
The ordinary FP event/clock itself is not executed here.

The original caller `1687C2` provides register inputs which must also be
retained: `168802` loads EAX from original pointer word `46FC48`, target
`447180` (forward bits `3F800000,00000000,00000000`); `168810` loads ECX
from `46FC28`, target `447144` (position bits all zero). These words and
target bytes are checked against the untouched XBE, not chosen marker vectors.
Actual `18ACD0` copies them into the request location. Its original owner
checks and `18DDE0` world/location producer execute for the allocated object.

The executed path is:

`18AA90 → 18ACD0/18AA40 → 18AD70 → 18EF20 → 18DDE0 → 18BA10 → 21F220`.

Original `18AA40` returns low byte zero in this nonlocal fixture, selecting
callback table `44B14C`. The `18AD70` context is ESI=`E2700001`,
EDI=`44B14C`, with stack sound datum, constant 1, location pointer, gain,
NONE, zero. Original `18DDE0` returns low byte 1. `21F220` naturally returns
`FFFFFFFF` because the original audio initialized field is zero. That result
is preserved, not overridden.

The **new** rejected A0-byte play state is then copied unchanged into raw
caller storage at `0B100000`. It is not the earlier owner-`10002` request.
Original `129450` computes listener `FFFFFFFF` in both new cases, consistent
with this unbound nonlocal context. Direct `127750(sound, listener, state,
NONE)` is an explicit source-kernel call-site input after rejection. It
creates actual source `EF730000`, owner `+10=E2700001`, callback
`+14=44B14C`, controller `+A8=FFFFFFFF`, and voice `+AC=FFFF`. The
source owner store and original cache request are captured, not supplied.

## Exact complete cleanup ABI and branch order

Both cases call original `128A70` with source datum in EAX and explicit
reason 3 on the stack. The exact reached sequence is:

1. `128A70`: the voice branch is skipped because source `+AC=FFFF`.
2. `1280B0`: selects the original authored chunk, decrements the actual cache
   pin byte 1→0 and clears source flag bit 1.
3. Callback `18E2F0(owner=E2700001, source=EF730000, reason=3)`, callee
   `RET 0C`: reads the source-produced valid owner pool and calls
   `BBCB0` with owner in EAX and mask 3 on the stack.
4. `BBCB0`, callee `RET 4`: low-16 index 1 is below high-water 2; header
   salt is nonzero. `MOVSX` of `E270` equals arithmetic `owner >> 16`.
   `BBCEB` accepts the header. Original type byte 2 yields `1 << 2 = 4`;
   mask `3 & 4` is zero. `BBD01` jumps to `BBD06`, returning NULL.
   `BBD03` does not load the object pointer.
5. `18E30C` takes the NULL branch to `18E333/18E334`. No `D2270`
   instruction executes. The callback returns normally.
6. `128AE0`: no-controller branch tail-calls `16D4C0`, freeing the original
   source record/bitmap. `128A70` returns without a supplied callback result.

The source's salt becomes zero and its pool count drops 1→0. Remaining freed
record bytes are preserved exactly rather than assumed cleared. Owner-pool
and owner-state bytes, game RNG, whole object/header/player records and the
rest of the source-pool backing are unchanged by removal. These are audited
full buffers, not only selected fields.

| Case | Resource lifetime around full source removal |
| --- | --- |
| Ready | Pending at removal; afterward original worker reads 53,760 bytes at shared.map offset 79,925,760 and original `1223AC` writes completion 0→1 |
| Reload | Original worker reads 32,768 bytes at offset 80,033,280 before removal; the selected loaded resource is then unpinned |

Both reads are 512-byte aligned and verified byte-for-byte by SHA-256 against
shared.map. Cache resource lifetime remains separate from source-record
lifetime; successful unvoiced cleanup does not establish a voiced stream.

## Reproduction and supported native boundary

Use a fresh output directory to preserve all frozen evidence:

```sh
audio_valid_repro=$(mktemp -d)
.tools/python/bin/python tools/oracle_smg_audio_valid_owner_1_1.py --case both \
  --output "$audio_valid_repro/measured/report.json"
.tools/python/bin/python tools/oracle_smg_audio_valid_owner_1_1.py --case both \
  --output "$audio_valid_repro/reexecution/report.json" \
  --replay "$audio_valid_repro/measured/report.json" \
  --context-replay "$audio_valid_repro/measured/raw-host-context.json"
.tools/python/bin/python tools/audit_smg_audio_valid_owner_1_1.py
```

The final auditor verifies the retained frozen measurement/reexecution; it
does not overwrite either run. A bounded native consumer can preserve the
verified owner datum, callback mask, signed salt validation, cache unpin before
callback, type-2 NULL branch and source finalization under these explicit
unvoiced inputs. It must not treat this as a successful scheduler request,
change the mask to clear a weapon field, call the unexecuted unit-slot branch,
or synthesize driver/voice/ready state. The next ordinary playback gate is
still the original device/driver/session producer and its real host service
contract, separately from this completed owner lifecycle.
