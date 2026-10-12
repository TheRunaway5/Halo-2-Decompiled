# Original first-person sound-owner selection and the nonlocal registration gate

The normal first-person sound caller selects the **registered hand weapon**.
It does not substitute the player unit for the sound owner. The preceding
[valid-owner cleanup experiment](SMG_AUDIO_VALID_OWNER_1_1_RESEARCH.md) explicitly
composed the source-allocated weapon into a direct `18AA90` call; the owning
first-person caller did not execute in that experiment. Its successful
unvoiced cleanup remains frozen with that distinction.

This new independent slice executes original first-person storage allocation,
reset and the normal four-view observer on the genuinely joined, spawned,
equipped **nonlocal** player. All three roots return. Original `14F5B0`
returns NONE for each of the four views, so the observer does not bind a unit
or weapon, advance a first-person clock, produce an animation event or call
sound. No local user, view, slot, event, ownership identity, callback or audio
admission is assigned to force those later branches.

The exact ordinary local-binding producer remains open. The empty observer
result is a completed original return under these nonlocal inputs, not a
platform exception or successful local registration. All 106 earlier pinned
dependencies and every production file remain unchanged. The report retains
`native_gameplay_update_applied:false`.

## Executable authority and independent execution

The source is the user's disc 1.1 update XBE, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Lockout SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
No later update is inferred. The new
[oracle](../tools/oracle_smg_audio_fp_owner_1_1.py) imports the unchanged
`ValidOwnerTrace` raw-provider composition and unchanged `PlayerTrace`
preparation/join/loadout helpers. It does not run the older direct sound case.

The whole compatibility context and two genuine measured startup clock inputs
are replayed verbatim from the preceding immutable valid-owner measurement.
Original `1479B0` still owns its allocation and RNG stores; no seed or TLS
field is supplied. This is a replay of measured Wine32 compatibility context,
not new retail Xbox context or a new real-time clock measurement. Actual host
raw storage providers retain their declared compatibility scope.

The [measured source execution](../analysis/multiplayer/map-pack/smg-audio-fp-owner/measured/report.json)
and [independent fresh-process repetition](../analysis/multiplayer/map-pack/smg-audio-fp-owner/reexecution/report.json)
retain full player/object records, entire 8330-byte view storage, entire
8000-byte scratch allocation, original calls/stores and instruction bytes.
The [auditor](../tools/audit_smg_audio_fp_owner_1_1.py) compares source outputs
without omitting binding or owner fields. Its only comparison exclusions are
the inherited explicit provider-mode labels and actual host allocation
addresses. The complete instruction files are checked against the original
XBE in both executions.

The frozen listings contain 25 files and 32,654 instruction rows, including
30,564 unique executed original addresses. Both executions record 9,306
first-person-phase stores. Exact artifacts, dependency hashes and verification
counts are retained in [the suite](../analysis/multiplayer/map-pack/smg-audio-fp-owner/frozen/suite.json)
and [audit](../analysis/multiplayer/map-pack/smg-audio-fp-owner/audit.json).

## Source-owned records and actual observed gate

Unchanged original heap/CRT/game-arena, player, object/body, world and loadout
preparation produces player `EC700000`, biped `E26F0000`, and carried
SMG `E2700001`. The actual player current unit and unit inventory/weapon
parent are source stores. The source object-header pool has capacity 2048,
stride 12 and high-water/count 2. Whole records and the inherited declared
services are retained in the report. No older header registry is copied.

The creation identity is the inherited zero-key **nonlocal** input. Source
player reset/join leaves `[4F01F0]+C+4*view` equal to NONE for all four
local views. Source-generated game options and published host property bytes
are delivered inputs. Sign-in, lobby, membership and complete session
production remain unproved; the nonlocal input must not be relabeled a local
controller binding. The separate control/view/camera storage constructors
also do not establish that binding.

| Original root | Source-owned output / return |
| --- | --- |
| `1677C3 → 125490` | Requests `8330` bytes and publishes original FP view storage at `4F1198` |
| `1677C3 → 1254D0` | Requests `8000` bytes with alignment exponent 4 and publishes original scratch at `4F1194` |
| `1677E5` | Clears/resets all four views and both hands per view; unit, selector and weapon fields become NONE through original stores |
| `1678C1 → 14F5B0` | Queries source local views 0, 1, 2, 3; each returns `FFFFFFFF` |

In this backing/layout, view storage is `82EBA058` and aligned scratch is
`82EC2390`. Those are recorded source outputs, not constants to assign to a
native constructor. The observer performs no view-storage write and leaves
the full initialized state unchanged. It never calls `169D8A`, `169965`,
`CD1A0`, `168866`, `1687C2` or `18AA90`. No later branch is made to run
by setting a slot or player mapping.

## Exact owner-selection contract from original instructions

These downstream functions are retained as **inspection evidence** in this
new suite. They are not marked executed by the nonlocal observer. Their
bounded execution in the older [registration](SMG_FP_REGISTRATION_1_1_RESEARCH.md)
and [FP sound](SMG_FP_SOUND_1_1_RESEARCH.md) suites retains its older supplied
local/world/clock context; it is not promoted into a genuine local join here.

1. `1678C1` resolves a local-view player through `14F5B0`. If non-NONE,
   it reads that player's current unit `+2C` and representation byte `+88`.
   Changed view binding goes through `169D8A`.
2. For each bound hand it reads the unit's **current** signed hand index
   `+212/+213`, then original `CD1A0` obtains the weapon from
   `unit+218+4*index`. Requested indices `+216/+217` are not this input.
3. Changed weapons go through `169965`. Original `169E15` writes the
   accepted weapon into the hand at `169E4B`, literal bytes `897E04`
   (`MOV [ESI+4],EDI`). Model/graph/node-name checks and effect registration
   retain their original predicates. The hand's unit and weapon remain distinct.
4. The ordinary FP clock/event path forwards a produced type-1 callback to
   `1687C2`. It computes
   `H = [4F1198] + view*20CC + C + hand*1010`.
   Sound datum must be non-NONE and the original camera/reference-flag gate
   must admit it. At `16881F`, bytes `FF7604`, it pushes **`[H+4]`**
   into original `18AA90` as owner. This is the registered weapon.
5. Original static caller loads EAX from `46FC48` and ECX from `46FC28`,
   pushes marker NONE and gain `3F800000`, and calls `18AA90` at
   `168822`. The returned sound identifier goes to `H+1008` at
   `168827`; the current activity's low word goes to `H+100C` at
   `168834`, including after an unavailable return.
6. The source request chain is
   `18AA90 → 18ACD0/18AA40 → 18AD70 → 18EF20 → 18DDE0 → 18BA10 → 21F220`.
   Original `18EF20` can change type 1 to 0 only after a source local-player,
   admitted camera mode, current-unit and parent-chain relationship. The
   nonlocal composition does not satisfy or force that gate.

The older FP sound suite executes that caller/request chain with its original
registered SMG `10002` and declared composed local-view context. The new
valid-owner suite executes the request chain from direct `18AA90` with the
genuine source-allocated `E2700001` but no ordinary local FP caller. These
are two separate bounded results. There is no new end-to-end assertion that a
genuine ordinary local player/event produced the `E2700001` request.

The mask-3 cleanup result is consistent with this weapon owner selection:
`BBCB0` accepts its index and signed salt, then rejects type 2. Changing the
owner to a biped would test a different caller contract and is not done.
The unexecuted `D2270` unit-slot path remains separate. Driver, thread,
saved settings, voice, successful admission and playback remain unavailable.

## Reproduction and next producer boundary

Use fresh output directories; the existing freezes are immutable:

```sh
audio_fp_owner_repro=$(mktemp -d)
.tools/python/bin/python tools/oracle_smg_audio_fp_owner_1_1.py \
  --output "$audio_fp_owner_repro/measured/report.json"
.tools/python/bin/python tools/oracle_smg_audio_fp_owner_1_1.py \
  --output "$audio_fp_owner_repro/reexecution/report.json"
.tools/python/bin/python tools/audit_smg_audio_fp_owner_1_1.py
```

The final auditor checks the retained frozen pair without rewriting either
execution. The next owning producer must establish the original local
view→player mapping through the genuine membership/lifecycle path. Storage
allocation alone, a native renderer's chosen view or a manually populated
player slot does not close that source boundary. Until it closes, the original
ordinary local FP clock-to-sound chain and the host driver/playback contract
remain separate from the verified allocation, nonlocal observer and unvoiced
owner cleanup.
