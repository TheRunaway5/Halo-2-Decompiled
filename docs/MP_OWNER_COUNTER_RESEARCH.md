# Original incoming-owner counter and selected-credit death timer

This independent TU1.1 gate executes the complete incoming-owner counter tail
of `CBA60`, including its history-write prefix, and two real lethal writer
prefixes where the incoming attacker and credited player can differ. It closes
this counter's ordinary multiplayer enable state without inventing an AI
default. Earlier frozen suites and production files are untouched.

## Authority and reproduction

- Original TU1.1 executable:
  `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256
  `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
- Original Lockout cache: `assets/disc/maps/lockout.map`, SHA-256
  `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
- New tool: [oracle_mp_spawn_owner_counter.py](../tools/oracle_mp_spawn_owner_counter.py).
- Ignored source evidence:
  `analysis/multiplayer/integration/spawn-source/owner-counter-report.json`
  and `tu11-owner-counter-instructions.txt`.

```sh
.tools/python/bin/python tools/oracle_mp_spawn_owner_counter.py \
  --output analysis/multiplayer/integration/spawn-source/owner-counter-report-repro.json
```

The suite contains **86 cases**: 32 admission/reference cases, 14 timestamp
and arithmetic cases, 32 enabled-cap cases, three source ownership transitions,
three complete AI-state producers and two actual lethal selected-credit timer
prefixes. The report records full before/after player and object identities,
counter and timestamp fields, history, source writes, register state and source
float bits at the counter stages. The tool asserts the preceding frozen report
pins and records all imported tool hashes. No instruction patches or marker/head
poses are supplied.

## Inputs, state and initialization

`CBA60` receives the victim's full object datum in EAX, normalized amount in
XMM0, and six stack arguments. The bounded counter requests explicitly vary
these inputs; they are not all asserted to originate from actual world damage.
Actual stock SMG fatal inputs are used separately in the timer cases.

| Argument | Source interpretation |
| --- | --- |
| `+4` | Signed low 16-bit damage class; class 9 chooses a different forwarding field |
| `+8` | Low-byte lethal flag; upper bits do not make it true |
| `+C` | Incoming full salted player datum, or `NONE` |
| `+10` | Incoming signed low 16-bit membership/designator |
| `+14` | Incoming full salted source-unit/object datum, or `NONE` |
| `+18` | Low-byte cause recorded in the victim's history |

The relevant persistent fields are:

| State | Type and meaning in this routine |
| --- | --- |
| Player `+2C` | Full current-unit datum or `NONE` |
| Unit `+138` | Signed 16-bit membership used by the relationship gate |
| Unit `+13C` | Full current player owner or `NONE`; the final enabled-cap gate tests only whether it is `NONE` |
| Unit `+10A` bit 2 | Dead/depleted flag of the final resolved body |
| Unit `+248` / `+24C` | Full optional forwarding object references for class 9 / other classes |
| Unit `+2EA` | Signed 16-bit counter, incremented with 16-bit wrap |
| Unit `+2EC` | Previous counter update tick, `NONE` sentinel, otherwise wrapping 32-bit tick |
| Time global `[518444]+8` | Current game tick |
| Time global `[518444]+4` | Float32 seconds per tick |
| AI global `[4FCDA8]+1` | Byte enabling the post-increment 3/5 reset gate |

The preceding frozen constructor cases execute original `C52C0` through
`C5464`: it clears `+2EA`, stores `NONE` in `+2EC`, clears current owner and
initializes the four-entry history. The new fixtures use that same original
constructor prefix, actual original player/object pools, host membership and
complete player binding. Body storage and active header registration remain
the inherited explicit fixture. See
[MP_SMG_DAMAGE_HISTORY_RESEARCH.md](MP_SMG_DAMAGE_HISTORY_RESEARCH.md) and
[MP_DATUM_OWNERSHIP_RESEARCH.md](MP_DATUM_OWNERSHIP_RESEARCH.md).

## Complete counter order and admission

The unchanged listing `CBA60..CBDB6` is the authority for this contract;
[CBA60](../analysis/map-pack-update/functions/000cba60.c) provides supporting
decompiled control flow. Its decompiler's float typing is not substituted for
the signed integer conversion instructions.

1. Record/accumulate the incoming hit in the victim's four-entry history first.
   This is before every counter gate, including lethal, relationship and owner
   admission. A counter rejection does not undo that history write.
2. Require the lethal **low byte** to be nonzero. Require incoming designator
   **low signed short** to differ from `-1`. Call original `1E2F50` with the
   victim membership and incoming designator; require its returned byte true.
   In source options state 2 this dispatches `15F680` to the actual Slayer
   relationship callback `253F10`, which compares the two signed-short values
   for inequality. Membership is distinct from an invented simulation team.
3. Prefer the incoming player if it is not `NONE`, its low-16 index is below
   player pool high-water, its record salt is nonzero and its sign-extended
   record salt equals arithmetic `incomingPlayer >> 16`. If its current `+2C`
   is not `NONE`, index that object header and use its nonzero body pointer.
4. If that preferred path cannot produce a body, validate the incoming object
   as a fallback: not `NONE`, index below object pool high-water, nonzero record
   salt, salt equal to arithmetic `incomingObject >> 16`, nonzero body and
   header type admitted by `(1 << (type & 31)) & 3`. Ordinary types 0/1 are
   admitted; tested types 2/5 are rejected.
5. From the resolved body, choose `+248` for low-short class 9, or `+24C` for
   other classes. If the chosen reference is not `NONE`, index its header and
   replace the body and current object reference with that linked object.
6. Reject if the **final** body has `+10A` bit 2 set. There is no fallback after
   this dead-body rejection. A dead primary body forwarding to a live body is
   admitted; a live primary body forwarding to a dead body is rejected.
7. Perform the timestamp reset and wrapped increment below, then the enabled
   3/5 gate. Return normally with `ret 0x18`. This is a mutating routine; no
   Boolean success result is established or needed by this counter contract.

Salt strictness is deliberately asymmetric in the original instructions.
The incoming player lookup and fallback incoming object lookup validate salts.
The preferred player's non-`NONE` current reference and a non-`NONE` forwarding
reference are then dereferenced by **index without another salt, range, type
or body validity check**. The suite explicitly varies stale current/forwarded
salts and observes that these unchecked paths reach the index occupant. These
are demonstrations of source preconditions, not healthy state produced by an
original lifecycle. A native port should preserve valid binding invariants;
it must not falsely claim this routine performs a strict object lookup at each
link. The enabled-cap owner-presence test also performs no player salt lookup.

There is no amount-positive gate in this tail. The complete zero-amount lethal
request still increments the admitted owner. Nonlethal requests still record
history but do not update counters. Raw lethal `0x100` is false and `0x101` is
true; class `0x10009` selects class 9, while designator `0x1234FFFF` is rejected
as `-1`. These are explicit varied normalized-result inputs.

## Timestamp reset, float order and wrap

At `CBD2B..CBD6C`, let `prior` be unit `+2EC`, `tick` current game tick and
`count` signed short `+2EA`:

```text
if prior == 0xFFFFFFFF:
    count = 0
else:
    delta = signed32(wrapping32(tick - prior))
    elapsed = float32(float32(delta) * secondsPerTick)
    if ordered(elapsed > float32(4.0)):
        count = 0
count = signed16(wrapping16(count + 1))
unit.counter = count
unit.lastTick = tick
```

`CVTSI2SS` converts the signed wrapping subtraction, `MULSS` multiplies by the
stored float32 seconds, and `COMISS/JBE` treats equality and unordered NaN as
no reset. Constant `465080` is `40800000` (4.0). The oracle's inherited floating
environment is SSE MXCSR `1F80`, x87 FPCW `027F`; source instructions own the
operation order. The elapsed XMM0 bits are captured and asserted for every
non-sentinel case. No wider double multiplication or unsigned elapsed comparison
is substituted.

| Case | Source result from count 2, unless specified |
| --- | --- |
| 119 ticks at `3D088889` seconds/tick | elapsed `407DDDDF`; counter 3 |
| 120 ticks at that rate | exactly `40800000`; counter 3 |
| 121 ticks at that rate | `40811112`; reset then counter 1 |
| 120 ticks at next float32 rate `3D08888A` | `40800001`; reset then 1 |
| 120 ticks at previous rate `3D088888` | `407FFFFF`; counter 3 |
| Prior tick is `NONE`, prior count 5 | unconditional reset then 1 |
| Prior tick one step in future | negative elapsed; counter 3 |
| Zero seconds/tick | zero elapsed; counter 3 |
| Quiet NaN seconds/tick | unordered comparison; counter 3 |
| Tick 0 minus prior `80000000` | signed delta `-2147483648`; counter 3 |
| Tick `80000000` minus prior `7FFFFFFF` | wrapped delta 1; counter 3 |
| Prior count 32767, no timeout | counter `-32768` |
| Prior count -1 / -32768, no timeout | counter 0 / -32767 |

Every admitted case stores the current raw tick, including short wrap and
enabled-cap reset. Rejected cases leave both counter and timestamp unchanged.

## AI enable producer and enabled 3/5 gate

The cap gate is conditional on byte `[4FCDA8]+1`. If nonzero, the final
resolved object's `+13C != NONE` chooses threshold 5; `NONE` chooses threshold
3. If the **signed post-increment short** is at least that threshold, reset
`+2EA` to zero. It is a reset after increment, not saturation or an upper clamp;
`+2EC` remains the just-written tick. Negative wrapped values do not meet a
positive threshold. The 32 cases cross enable 0/1, owned/unowned and initial
counts 0, 1, 2, 3, 4, 5, 32767 and -1.

The complete real new-map initializer starts at **`1CB3D0`**, before Ghidra's
separate `1CB3E6` entry. Original game-system callback data at `447C58` points to
`1CB3D0`. Its multiplayer branch is:

```text
1CB3D0  eax = [4EDF18]
1CB3D5  compare dword [eax+8] with 2
1CB3D9  if different, enter 1CB3E6
1CB3DB  ecx = [4FCDA8]
1CB3E1  byte [ecx+1] = 0
1CB3E5  return
```

Thus original **multiplayer options state 2 explicitly disables this gate**.
Only other options states enter the AI initialization body which eventually
sets the byte to 1 at `1CB4FE`. The new case poisons the entire mapped AI global
with `A5`, executes complete `1CB3D0` in original state 2 and observes only the
enable byte cleared. It does not need a supplied AI result. Two further complete
cases poison that global and execute `1CB990` / `1CBA00`, which also clear the
enable byte. The original callback table bytes and full initializer/reset
instructions are pinned in this suite's listing. Complete registered world
bootstrap and non-multiplayer AI initialization are not executed here.

## Full identity across ownership transitions

Three cases execute the original lifecycle producers used in the preceding
frozen datum/lethal suites before running complete `CBA60`:

| Source transition and incoming ownership | Counter recipient |
| --- | --- |
| Attacker dies, source detach and bind new unit; incoming player is still the same full player, incoming source-unit is its retained old dead unit | New current unit; old dead unit stays unchanged |
| Player leaves and rejoins with new player salt; incoming old full player is stale, retained old source-unit still allocated and alive but unowned | Old source-unit fallback; new player's unit stays unchanged |
| That leave/rejoin plus actual `154440` player deletion callback and lower `BD1F0` release; original allocator reuses object index with new salt | No recipient; fallback old full object salt is rejected |

The last case proves lower allocator release/reuse, not complete world
deletion. None of the cases invent a free-on-death or free-on-respawn timer.
These results require preserving full source player and object identity and
current binding; they cannot be reproduced by slot-only `last_damager`.

## Incoming self with enemy selected credit

The two new timer cases generate **24 actual stock SMG hits from enemy player
0**, then a real fatal hit with either that enemy or the victim as incoming
owner. Each hit captures full projectile ownership through original `BD080`
and executes actual damage/result/attribution instructions. Original `1ED960`
selects enemy player `EC700000` from previous history in both cases and passes
it to actual `15E4D0`; victim is `EC710003`.

The self-final case therefore has incoming player `EC710003` but **selected
writer attacker `EC700000`**. With respawn seconds 0 and suicide penalty seconds
5, both real writer prefixes produce countdown `+164 = 30`, clear `+168` to
zero and reach the same current-unit marker request. The writer compares its
**selected full attacker argument** with its victim argument at
`15E59C..15E5E4`. It adds suicide penalty only when those arguments are equal;
the incoming-self flag does not substitute for selected credit there. The prior
frozen corpse suite's all-self history/fatal case selects victim as credit and
produces the positive 150-tick penalty result. See
[MP_CORPSE_LIFECYCLE_RESEARCH.md](MP_CORPSE_LIFECYCLE_RESEARCH.md).

Both new prefixes stop at the original `B9C20` call entry before any marker
transform: full victim unit `E2700001`, marker `04000095`, capacity 1, CL 0,
original return address `15E7E1`. Player ownership remains bound at this point.
No position, history timer copy, later death statistics, event, Slayer callback
or cleanup output is fabricated. The older complete lethal contract separately
proves that victim suicide statistics use incoming self even when enemy history
receives credit; this new prefix alone does not execute those later statistics.

## Frozen evidence and limits

| Artifact | SHA-256 |
| --- | --- |
| New oracle | `349577d11c85e23895946c9591e38aebad26af794240a5ede97127767d430dba` |
| New 86-case report | `e7492035911eb502c77b5154b3a35a9ad3c8decac254f3c5f18758d9a0d56dac` |
| New instruction listing | `6d31cacba708122f3a4abe43284acffb5e96b5f2c34810fecb9302bcc657c31f` |

This contract permits the supported native incoming-owner counter to run after
the original history write and before synchronous death cleanup, with full
current binding and membership intact. The ordinary multiplayer producer makes
the enabled-cap branch inactive. Exact counter wrap/timing and selected-credit
suicide timer semantics are established; this is not proof of complete engine
startup, all damage classes, valid forwarding producers, animation/Havok,
marker transforms, full world deletion or live Xbox parity. Explicit invalid
references, header-type variants, normalized counter inputs and inherited
storage/active-header boundaries remain clearly bounded in the report.
