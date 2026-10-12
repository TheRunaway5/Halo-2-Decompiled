# Original held kill-trigger fatal continuation

The complete original TU1.1 `DEFC0` call now returns on the genuine allocated,
moving Spartan after a separately selected sound-storage/current-ugh prefix.
Its final object velocity is unchanged. This closes the prior fatal-tail stop
for this bounded composition; it does **not** complete natural map startup,
audio playback, a naturally falling trajectory, forced landing-dead animation,
or ragdoll solving/presentation.

## Authority and retained inputs

The unchanged executable is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`
(SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`).
Actual Lockout cache is SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The new oracle first reproduces **all** stages, calls, reads, stores, service
records and final backing from the frozen
[earlier kill-trigger experiment](MP_SPARTAN_KILL_TRIGGER_RESEARCH.md)
(report `e43e73718e3a327842ad1b3e30dfb3101bafd59492e71997fc1a76861b0dcb5d`).
It retains that experiment's source movement, authored trigger-volume diagnostic
placement, tick/dt, selected source player/world construction and explicit
platform/service boundaries. The placement remains a caller test input, not
source-produced falling motion. The original kill check consumes object+30
current bounds center, distinct from object+64 placement root.

New source execution/tool:
`tools/oracle_mp_spartan_kill_trigger_sound_1_1.py`.
Evidence root:
`analysis/multiplayer/integration/spartan-kill-trigger-sound-20261010/`.

## Selected initialization, with the natural ordering still blocked

Original callback record45 at `447A7C` contains
`{20E8B0,20E920,20E960,...}`. Whole constructor `20E8B0` consumes the
already source-created `125280` game arena (`4ED650/4ED654/4ED65C`), increments
its cursor by `1880`, executes `1656A0` CRC update, and publishes its slice at
`526280`. It makes the exact `33785C` raw request
`VirtualAlloc(NULL,1880,101000,4)`, then stores the returned allocation at
`526284` and `52627C`. The new adapter uses actual anonymous private
read/write libc `mmap` backing, rounded to `2000`, guest alias `0C100000`.
It supplies raw OS storage only; the original constructor produces every sound
field. It does not execute the original XAPI VirtualAlloc body or establish
retail Xbox address/status parity. Actual host allocation addresses are saved
separately from the repeatable report.

Whole selected `20E960` executes its literal `rep stosd`, clearing `620` words
(`1880` bytes) in the actual allocated bit bank. Neither a preset sound record,
voice/permutation selection nor successful audio callback is supplied.

Whole selected `126DE0` reads the actual current matg/tag table and publishes
current ugh pointer `804EEE90` at `126E1C`. It then stops at unchanged `126E22`,
attempting to write `[system+1E4]` while `4ED950==NULL`. The owning `126B20`
sound-system constructor has **not** run on this fixture. No earlier audio
arena is copied in and no system pointer is fabricated. This exact cold stop
is distinct from prior audio studies which had an earlier source-created
sound arena and reached the later driver dependency.

These are explicitly selected roots. In the original ascending map reset,
callback44=`126DE0` precedes reset45=`20E960`; the selected reset cannot be
presented as naturally reached after the still-blocked callback44. This study
retains the actual prefix outputs while keeping that natural startup boundary
open.

## Held CPU/frame continuation and complete fatal return

The original fatal call was held at `20EFC2`, after its preceding `20EFBC`
`mov ecx,[526464]` had cached NULL. Selected roots execute on external caller
stack `0C01F000`; the held original CPU context and every byte of its stack
region are preserved. Direct retry of `20EFC2` still faults at NULL+24. This
negative is retained. The next explicitly declared retry begins at the
immediately preceding unchanged `20EFBC` global load in the same frame, so
original instructions read the newly produced ugh. No fatal admission is
re-run, no guest gameplay field is assigned, and no return is patched.

The actual `20EF90→21B540` sound lookup returns the real class-selected first
index `21h`; the source bit-bank test returns AL0 on the actual zeroed bank.
Execution continues through the original fatal tail:

`DEFC0→D9180→DAC40→CB410→CBDC0→E6050→E6DC0→1C70D0`,
then unwinds to the original caller return. Complete `DEFC0` returns EAX1.
Two inherited `170060` playback boundaries return unavailable0. This proves a
complete fatal return with unavailable playback, **not** successful sound
submission. No new successful audio, animation, physics, or admission result
is supplied.

At complete return:

| Owner field | Before | After |
| --- | --- | --- |
| Full unit datum | E26F0000 | E26F0000 |
| Object+88 velocity words | 3C33EFD9 / 3EA3BE55 / BE6F9DB2 | identical |
| Object+13C current player | EC700000 | FFFFFFFF |
| Object+140 previous player | FFFFFFFF | EC700000 |
| Player+2C current unit | E26F0000 | FFFFFFFF |
| Player+30 retained old unit | FFFFFFFF | E26F0000 |
| Unit+3DC physical state | 1 | 3 |
| Unit+34B | 0 | 0 |
| Physical body count | 1 | 1 |

There are **no stores** overlapping object+88..+93 during the resumed complete
fatal tail. `CBDC0` receives fourth argument0; `E6050` therefore does not request
its forced landing-dead branch. `C5190`, `EFF50` and `1D9330` are not executed.
The actual one-body controller rebuild/world path executes; this remains
separate from the previously measured direct ten-body constructor and its
still-open natural +34B/contact/ragdoll admission.

## Independent verification and authenticated 1.5 correspondence

Measured/repeated report:
`measured/report.json`, `repeat/report.json`.
The independent auditor checks repeated report/listings, every listed byte
against the original XBE, all reads/stores against executed instructions, the
entire prior prefix, exact raw OS request, final ownership/momentum and complete
captured before/after buffers by replaying ordered original stores.

The complete represented new roots are independently paired against the
authenticated u5English executable SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`:

| TU1.1 | Authenticated 1.5 | Owning correspondence |
| --- | --- | --- |
| 20E8B0 | 2131B0 | complete constructor |
| 20E920 | 213220 | complete teardown, listed only |
| 20E960 | 213260 | actual callback record45, unique paired ctor/dispose record |
| 126DE0 | 1281A0 | complete current-ugh/map owner |
| 20EF90 | 213890 | complete sound bit-bank consumer |
| 21B540 | 21FE40 | actual direct call from paired20EF90 |
| 13FB30 | 140EA0 | complete helper |
| 33785C | 33B3AC | original raw allocation entry, adapter boundary |
| 1656A0 | 166A80 | complete arena CRC helper |

All nine complete instruction/internal-branch shapes match. The two ambiguous
short-prefix mappings are derived from actual paired owner calls/callback
record bytes before complete-body comparison. Annotation semantics and fixed
image offsets are not mapping authority. Original 1.1 dynamic execution is not
whole 1.5 execution or Xbox hardware parity. Previous death/kill body mappings
remain pinned in the earlier independent freeze.

## Native integration consequence and remaining limits

The native kill-trigger branch's unconditional velocity clear is unsupported
by this complete original fatal return and can be removed narrowly. This does
not authorize changing kill-volume scheduling/predicate to a guessed bounds
center: exact source one-row-per-tick selection, disabled-bit ownership and
current bounds producer remain separate live integration requirements.
Nor does momentum retention authorize dead-body motion, a standing corpse,
a guessed death clip/timer or ragdoll constraints. No shared runtime, header,
CMake, renderer or release file is changed by this evidence extension.

Measured and independent repeat report SHA-256:
`5515ce2e5950eb864b3074f45ffe9e0f855089d4cb299a04b1f8d03a6b6e6858`.
All 555 repeatable report/instruction files are byte-identical. The audit covers
2,664 original calls, 59,975 reads, 32,118 raw stores, 28,441 executed addresses,
and 76,219 listed instruction rows. It independently replays 204 complete
captured intervals, including 5,743 overlapping reads and 4,262 stores (1,536
changed bytes). `audit.json` and `audit-repeat.json` are identical. Separate
host context/allocation records retain actual platform provenance and are not
claimed process-address-identical. All nine complete correspondence bodies
and both correspondence reports repeat identically.

Immutable manifest: `analysis/multiplayer/integration/spartan-kill-trigger-sound-20261010/frozen/manifest.json`.
The earlier cached-pointer negative probe and guard-failure log are retained
as separate diagnostic artifacts; only the measured/repeat pair is the final
independent proof suite.
