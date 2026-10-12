# Original neutral command production and owned biped control

The original neutral-command initializer, command codec, packet assembler and
complete player update now execute on the source-created, equipped Lockout
biped. With its owning network configuration handler executed, the source
packet admits one command and the player update calls original `C8140` once.
The four original object motion phases then return with the real player PVS.
The original outer packet scheduler still admits no command because map
readiness and the simulation state transition remain open. This is an explicit
composition of original producers and consumers, not complete device dispatch
or a complete ordinary local FFA tick. Production and earlier freezes are
untouched.
[Independent oracle](../tools/oracle_mp_spawn_command_input.py),
[complete report](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[preceding player/world/PVS freeze](MP_PLAYER_VISIBILITY_RESEARCH.md).

## Immutable authority and repeat

Original TU1.1 XBE SHA-256 is
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`;
actual Lockout SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The complete earlier compatibility context is reused without editing its
records; its real host worker remains alive throughout each run. Source-owned
heap, object/body/world, player, animation, loadout and PVS producers retain
their preceding scope and capacity. Compatibility-produced unknown zeros are
not recovered Xbox hardware defaults.
[Preceding preparation and scope](MP_PLAYER_VISIBILITY_RESEARCH.md),
[complete compatibility producer](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md).

| Evidence | SHA-256 |
| --- | --- |
| Oracle | `de382cb5f14ccbc64fbc5609419382f2a574fc1f46ba00e371efbfa3a641f4fc` |
| Report | `1fc1b58c65fd804477ac9bbdcd3bcad1ad10bd954f98b0f5ea17348891ba0a76` |
| Instruction manifest | `24849163eb3364a4bdc8138ed548a94644b070646ba307195567912e69bc159f` |
| Executed original instruction listing | `4e0aaea1af6cbe05f7f2377c2e22b3ce4c508ddaa8a25d1189eda9291330c535` |
| Observed input store-site listing | `7403e0b8386ae9930771700a7c27182bf1a689d363049181f053ba04ee77fcdd` |
| Measured raw host clock fixture | `3789bb92e1fb3e8f18ed3b2a9471c5156f8b9af86360055952f85e625adf3326` |

Two cases audit 47,960 unique executed original addresses
against the original XBE and both emulator images. There are 296 routine
listings containing 26,222 rows, plus the complete executed listing and observed
input store-site listing. All 42 earlier/input dependencies are checked before
and after execution. No original instruction is patched. Independent execution
must reproduce the report and all 298 instruction files byte-for-byte; per-run
raw host records are separate evidence and may differ.
[Executed original bytes](../analysis/multiplayer/integration/spawn-source/command-input/instructions/executed-original-instructions.asm),
[observed original stores](../analysis/multiplayer/integration/spawn-source/command-input/instructions/observed-source-input-store-sites.asm),
[independent verification](../analysis/multiplayer/integration/spawn-source/command-input/independent-freeze-verification.json).

Reproduce from the repository root:

```sh
.tools/python/bin/python tools/oracle_mp_spawn_command_input.py \
  --output analysis/multiplayer/integration/spawn-source/command-input/reproduction/report.json
```

## Owning configuration and simulation initialization

The fixture first executes the frozen selected player/world preparation.
Before join, the positive case executes original configuration entry `663B0`,
then both cases execute complete simulation storage constructor `68040`,
empty old-map disposal `68170` and new-map callback `680F0`. These are explicitly
selected roots after the preceding preparation, not a recovered complete title
startup sequence. Original callback record 23 at `447764` contains init
`68040`, dispose `680C0`, new-map `680F0` and old-map `68170` followed by five
zero callback words. Its unchanged record bytes are retained.
[Callback record and stage order](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[simulation constructor](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00068040.asm),
[whole empty disposal](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00068170.asm),
[whole new-map callback](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000680f0.asm).

Original configuration handle `47E548` points to descriptor `46E5E4`, whose
+10 callback is `663A0`. Complete `663B0` dispatches this callback, which calls
`663F0`; original `66C9A` then calls `67A70`. Original `67AF8` loads literal
`46507C` (raw `00000040`, float 2.0) and `67B1C` stores it to `4D62CC`.
The descriptor handler flags at `47E54C` change 0→2→0A through original
`663CB` and `663D2`. No expiry value is written by the fixture.
[Descriptor bytes and actual stores](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[handler dispatcher](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000663b0.asm),
[owning configuration constructor](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000663f0.asm),
[literal expiry producer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00067a70.asm).

`68040` aliases the original BSS simulation context `54C670` (1224 bytes),
watcher `54D894` (C34 bytes), and dispatch table `54E4C8`, then executes the
real dispatch-table constructor. It sets simulation global byte `4D6D38` to 1.
Storage construction alone leaves zero BSS entries; complete owning
`68170→69030` executes original player reset `6AAB0` and vehicle reset
`6ABA0`, producing NONE sentinels. Neither record is manually initialized.
Complete `680F0→68E10` derives context kind +8=1 and state +18=2 from the
retained game-global input, binds the watcher and calls its real constructors.
The inherited game mode/context is a declared boundary; these outputs do not
establish an ordinary local host device session.
[All before/after context bytes](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[owning reset](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00069030.asm),
[context new-map producer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00068e10.asm).

The two original `337A40` calls execute their unchanged instructions and read
the imported ordinal-156 `KeTickCount` data. Original import word `417CD4`
contains `8000009C`; the fixture routes this data import to owned four-byte
backing `0B000000` holding measured host `CLOCK_BOOTTIME` milliseconds. This
import routing is a raw platform boundary, not a game-state writer. Original
`197450` calls `time(NULL)` at `32803E`, whose real conversion executes from
the returned FILETIME. A separately pinned provider intercepts wrapper `336983`
and returns measured host UTC FILETIME; underlying Xbox kernel
`KeQuerySystemTime` is not executed. The three actual measurements are replayed
verbatim for reproducibility. Original `197463` and `19746D` own its recording
time stores. This path does **not** execute `srand`, `rand`, `32C609` or cold
CRT fallback allocation; static listings for those routines are not execution
claims. A provisional pre-freeze report misidentified `32803E` as `srand`;
that wording was corrected and the provisional evidence is retained separately.
[Raw measured clock fixture](../analysis/multiplayer/integration/spawn-source/command-input/measured-host-clock-inputs.json),
[original time function](../analysis/multiplayer/integration/spawn-source/command-input/instructions/0032803e.asm),
[owning recording initializer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00197450.asm),
[separate CRT/RNG producer authority](SMG_AUDIO_RNG_STARTUP_1_1_RESEARCH.md).

## True owner, neutral command and original control writer

The frozen join/spawn helper allocates player `EC700000`, biped `E26F0000`
and SMG `E2700001`, binds them through original stores, and derives actual
cluster 4/PVS bytes `FFDF0000`. Its source current-unit binding and the exact
authored root bits `BFB9579D,3C855B09,C1235809` are preserved. The inherited
creation delivery is a nonlocal zero-key input with local-user index NONE;
source local membership, controller/view binding and physical device inputs
are not supplied. With real simulation construction enabled, original join
also executes `69700→85220→14E780` and produces the actual simulation
control record for the allocated player. Record +8 becomes 0 through source
branches in this declared context.
[Join and source simulation membership calls](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[owning join hook](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00069700.asm),
[record constructor](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00085220.asm),
[preceding ownership/PVS proof](MP_PLAYER_VISIBILITY_RESEARCH.md).

Complete original packet producer `684F0→694A0`, preplayer dispatch `685B0`
and full `14EAE0` return with a zero player-command mask and no `C8140` call.
Game+1120 stays zero and context state +18 stays 2. The separate explicit-root
composition then follows this original producer/consumer chain:

| Stage | Actual ABI and output |
| --- | --- |
| 14E780 | EDX points to a 5C-byte output; original source writes the neutral command |
| 852A0 | Stack arguments are actual simulation player-record pointer, source 5C command; original codec/submission updates the record |
| 69350 | Stack arguments are actual simulation context, original packet output; source assembler supplies the player command and mask |
| 14EAE0 | Stack packet argument; complete source player phase consumes the assembled command |
| 151C90 | EAX is full player datum; stack arguments are current unit and source command; original control constructor runs |
| C8140 | EAX is `E26F0000`, ECX points to source-produced 7C-byte control; actual writer changes the allocated biped |

Packet backing `010F0000` and command backing `010EC000` are existing owned
scratch storage. Source `684F0` initializes the complete 4048-byte packet;
source `14E780` writes the whole 5C-byte neutral command. No command payload,
mask, control vector, sequence, current-unit binding or target pose is assigned
by the fixture. The neutral initializer clears 0x17 dwords (92 bytes), then produces
FFFF at short +1E/+22/+24, FF at bytes +20/+21, and FFFFFFFF at dwords
+2C/+30/+34/+38/+3C; all other final bytes are zero. The exact literal buffer
is retained separately from its later codec and packet copies. Original
`852A0` codec output contains pitch bits `3A491C93`
at record command+8 even though the initializer output is zero there. Original
x87 trigonometry in `151C90` produces control direction bits
`3F7FFFFB,00000000,3A491C92`; those source rounding results are preserved.
[Complete command/context/packet snapshots](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[neutral source initializer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/0014e780.asm),
[actual submission and codec](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000852a0.asm),
[source assembly](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00069350.asm),
[source control construction](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00151c90.asm).

The positive full player phase enters
`150DD0→1EA490→150F00→151C90→C8250→C8140`. Actual `151E15` calls the writer
and returns at `151E1A`. `151C90` writes literal control flags `06000086`
and preserves the source default sequence/grenade selection gates.
`C8140` copies source vectors to unit+150, +15C, +180, +1A4 and +1B0,
scalar inputs to +1C0/+1C4, flags to +14C and nine dwords to +1C8.
It updates sequence +214 only if incoming short +6 matches the existing short;
incoming selection short +A=NONE skips +23D. The exact 7C input, all observed
writer stores and before/after unit bytes are retained. Each actual `85460`
call also records entry registers/stack, its full 88-byte record and 5C-byte
delivery buffer before/after, raw returned EAX, and the separately defined AL
byte at the original `RET 8` return. The upper EAX bytes are not interpreted
as the boolean result. Each validator entry retains the actual pointer at
`518444`, its literal 12-byte clock record, and four-byte expiration at
`4D62CC`; no exporter must reconstruct those inherited inputs. `C8140` itself masks
the unit datum to its low index when loading the body; its execution here is
supported by the genuine owning caller, not a claim that it validates a salt.
[Complete player tail](../analysis/multiplayer/integration/spawn-source/command-input/instructions/0014eae0.asm),
[literal constructor and call](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00151c90.asm),
[exact writer/gates](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000c8140.asm),
[7C input and unit stores](../analysis/multiplayer/integration/spawn-source/command-input/report.json).

## Two cases and first motion outputs

| Case | Source expiry | Explicit assembler mask | C8140 calls | Four object phases |
| --- | --- | --- | --- | --- |
| Configuration producer omitted | Actual BSS 0.0 | 0 | 0 | All return |
| Original configuration handler executed | Source literal 2.0 | 1 | 1 | All return |

Original `85460` requires record +8 not equal 2, record timestamp +28 not NONE,
and `float(current_tick-record_tick)*dt < [4D62CC]`. The age calculation uses
32-bit subtraction, `CVTSI2SS`, `MULSS`, then `COMISS`/`JBE`; equal age/limit
does not pass this branch. The fixture retains tick 0, dt 1/30, source submitted
timestamp 0. Omitting the owning configuration producer therefore rejects even
the age-zero command. This is a measured omitted-producer case, not ordinary
configured gameplay defaults or a general expiry boundary suite.
[Original eligibility and operation order](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00085460.asm),
[both bounded case inputs/results](../analysis/multiplayer/integration/spawn-source/command-input/report.json).

Both cases execute `B7E00→1C7660→B7F50→B8040` in the original object-phase
order. Biped header flags change 4B→4F→4F→47; equipped SMG flags finish 8A.
The exact authored root remains unchanged and final velocity bits are
`00000000,00000000,BE6F9DB2`. These are original outputs, not chosen velocity,
gravity or contact. Actual `1E9540` motion request and `E3B20` consumer retain
their complete 154-byte before/after records. Raw EAX is `FFFFFFFD` from the
void producer and `0101A001` from the consumer; unexplained request fields are
not classified as floor-contact success. No game/Havok service executes in
these phases. Inherited 30-Hz clock inputs and FPCW 027F/MXCSR 1F80 bounded
root ABI remain explicit; source whole-tick increment and `139750`'s initial
floating-point control setup are excluded.
[Exact phase states, services and motion request records](../analysis/multiplayer/integration/spawn-source/command-input/report.json),
[preceding request semantics and limitations](MP_PLAYER_VISIBILITY_RESEARCH.md).

## Remaining scheduling and service gates

Original `68460` requires enabled simulation, a game-global pointer,
Game+1120 nonzero, current BSP not NONE, byte `4D6D3A` zero, and context +18=4.
This fixture preserves source Game+1120=0 and context +18=2; the complete
outer packet scheduler emits mask 0. Original `190A30` owns readiness after
whole map reset `139410`, authored object setup `D6B60/C1870`, membership
`1395B0`, and successful original `11D780` BSP load/activation, then stores
Game+1120=1 at `190A6E`. Those full scheduling producers and the later context
state-4 transition remain unexecuted. No readiness byte or state is forced.
[Exact outer gate](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00068460.asm),
[outer packet producer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/000684f0.asm),
[actual map-ready owner](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00190a30.asm).

The source local-device chain `188AD0→1884E0→187610→69280→852A0` still needs
its owning local user/view/controller, processed device and identity-table
producers. Its instructions are included as static contract evidence but are
not executed in these cases. Older fixtures with supplied control mappings
cannot establish these producers. The explicit neutral composition is usable
as a source input/control/motion differential in its declared context; it does
not prove that ordinary device scheduling reaches it.
[Local device scheduler](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00188ad0.asm),
[player/device mapping](../analysis/multiplayer/integration/spawn-source/command-input/instructions/001884e0.asm),
[processed device consumer](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00187610.asm),
[identity lookup/submission](../analysis/multiplayer/integration/spawn-source/command-input/instructions/00069280.asm).

Inherited raw OS storage, whole compatibility context, cache/resource delivery
and loadout effect/audio boundaries retain their preceding scope. The used
loadout services are attachment `178120`→NONE, looping source `C10E0`→NONE,
and audio `170060`→0. The new FILETIME provider and imported tick-data backing
are raw host clock boundaries; no new game or physical-success return is
supplied. Complete title/session startup, local device producer, complete game
tick, camera/head-pose history and hardware contact parity remain open.
[Complete service audit and declared boundaries](../analysis/multiplayer/integration/spawn-source/command-input/report.json).
