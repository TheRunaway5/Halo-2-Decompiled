# Original 1.1 startup RNG and audio-controller seed producers

This independently frozen extension executes the original game RNG constructor
`1479B0`, then the original sound constructor `126B20` through its actual
controller constructor `21BC50`. Original heap/CRT instructions allocate and
initialize the required CRT record. Real host storage and clocks are recorded
provider inputs. **The seed writers complete; audio initialization still stops
before `2205C0` device construction, with source audio `+78=0`.**

No seed, CRT record, heap receiver, initialized flag, device result or audio
completion is written to admit the path. Complete Xbox/title startup, normal
allocation order, ordinary title-thread TLS and audible playback are not proved.
Earlier startup, cache, adapter, sound, first-person and world freezes remain
unchanged. This study is separate from the earlier heap failure in
[the audio startup freeze](SMG_AUDIO_STARTUP_1_1_RESEARCH.md).

## Pins and reproduction

| Artifact | SHA-256 |
| --- | --- |
| `tools/oracle_smg_audio_rng_startup_1_1.py` | `c67ddaa11d18fb50a67eea73e213dd926b37f343dee1845d15f37ba702ef9cc6` |
| `tools/audit_smg_audio_rng_startup_1_1.py` | `6eb81b808fd48a039ac5a7beb88757fe12701270ba5d602fd4f3cafbb133f6cf` |
| `analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen/suite.json` | `830913b3064076209284c4772f1531254e7a25a1f4ac852bec39f4f13d35b146` |
| `analysis/multiplayer/map-pack/smg-audio-rng-startup/source-fixture.json` | `d2f3ccd5468e56d522b24ddc54329e79318870df5ad1c70c3054c4018b2297cc` |
| `analysis/multiplayer/map-pack/smg-audio-rng-startup/audit.json` | `3b3f1e47c77490a8dd88a6c0e296dd3d40a546caa99f986a6cb35beb408eff02` |
| Measured original report | `43cf5c78f8fd6b2af8fd6e9de8c65ac249564ecda13463a340a1a333599b2075` |
| Independent replay report | `d965210c3b753ae23e02f9df57d9d3a1b61f6e9eba804aef8ddfe2f6e6ddb133` |

Original authority is update `default.xbe.03`, full-image SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
The inherited real Lockout cache SHA-256 is
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The audit verifies 25 instruction listings, 3,746 original instruction rows,
including every one of 1,655 actually executed original addresses; it checks
all listing bytes against the unchanged executable. Original source outputs
match in an independent execution of the exact recorded clock/context inputs.
[Fixture](../analysis/multiplayer/map-pack/smg-audio-rng-startup/source-fixture.json),
[audit](../analysis/multiplayer/map-pack/smg-audio-rng-startup/audit.json).

For another execution, write a fresh directory:

```sh
.tools/python/bin/python tools/oracle_smg_audio_rng_startup_1_1.py \
  --output analysis/multiplayer/map-pack/smg-audio-rng-startup/check/report.json \
  --replay analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/report.json \
  --context-replay analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/raw-host-context.json
```

Omitting replay options launches the pinned existing Wine32 compatibility
generator, keeps its real worker alive and records new actual host clocks. It
never rebuilds or modifies that generator. Replaying its complete recorded
context is not a second live host-thread measurement. Guest pointers/source
bytes are compared independently of actual host mmap addresses, which do not
enter the source's address space.

## Prerequisites actually executed

1. Execute the actual XAPI prologue `2D7F8E..2D7FA5`, then actual heap-writer
   fragment `2D7FB0..2D7FE3`; the intervening shutdown/hardware/event calls
   remain excluded. Literal XBE reserve100000, commit1000 and flags80000005
   come from the original header. Original `2DA83A` creates heap metadata and
   synchronization; the original caller publishes heap global `566F08`.
2. Execute original runtime lock initializer `32CFFC`, then original C
   on-exit-array initializer `3274E4`. Both return. Their actual table/caller
   ordering was recovered in the separate fixed-memory study. Other CRT/TLS,
   C and C++ initialization remains outside this bounded preparation.
3. Execute original physical-arena root `12CB70`, actual game-arena prefix
   `125280..1252BA` and tail `1252BF..125339`, preserving the original frame
   across the excluded `217090` savegame/thread-I/O call. Source produces arena
   `82D47000` and cursor128C. No savegame success or object setup is supplied.
4. Invoke actual constructor `1479B0` with that produced arena. Its 8-byte
   allocation is `82D4828C`; source advances cursor to1294.
5. Provide explicitly declared raw static sound-arena backing
   `0A000000/23000`, initial used0, then invoke unchanged full `126B20`.
   Original cache, source-pool, controller and other preceding sound constructors
   run. The source invokes actual `21BC50`; it returns1. Execution stops before
   original device constructor `2205C0` begins, at caller `126C90`'s request.

The heap constructor executes pinned primary compatibility critical-section
code on the **whole** produced KPCR/ETHREAD/host-stack/static-process context.
Uncontended lock semantics are the prior pinned compatibility implementation,
not retail Xbox kernel instructions. The context's host branch produces
TLSNULL, IRQL0 and its complete records; no selected field or pointer is patched.
[CRT prerequisite authority](MP_FIXED_MEMORY_WORLD_RESEARCH.md),
[whole context authority](MP_COMPLETE_COMPAT_KPCR_RESEARCH.md),
[original heap producer](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/002da83a.asm),
[exact retained stages](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/report.json).

## Actual raw storage services

The new provider performs actual `mmap(PROT_NONE)` for requested reserve100000,
and actual `mprotect(RW)` for the first1000 committed bytes. Source-visible
`08000000` is an explicit compatibility alias for that backing. NTSTATUS0 is
the compatibility translation of those **measured host successes**, not an
independently observed retail kernel result. Only base/access services are
provided; original code creates every heap and CRT field.

The physical request is unchanged kernel ordinal166 with arguments
`[030E4000,00061000,03144FFF,0,4]`, returned guest alias`80061000`.
Actual host mmap supplies raw storage and preserves the inherited literal cache
overlays byte-for-byte. It does not establish original RAM topology. Actual
host mprotect satisfies original ordinal182 requests for game arena
`82D47000/3FE000` and cache buffer`82A47000/300000`. The sound cache's two
original VirtualAlloc requests are `[0,208C,101000,4]` and
`[0,30FC,101000,4]`; actual host mappings return guest aliases
`0A100000` and `0A103000`. Original `21A6E0` and its pool/page constructors
write the cache state. No page-ready or I/O-completion field is supplied.
[All seven raw requests and real results](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/report.json),
[original cache constructor](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/0021a6e0.asm).

The static audio-arena binding remains an explicit provider input, matching the
earlier cache study's raw arena protocol. Its source startup reservation
producer and complete title allocation order are not asserted. Current-player,
tag, resource and other inherited fixture boundaries remain listed and pinned;
none of those unrelated fixtures is reclassified as full title initialization.

## Original seed constructor and ordering

The real callback table at `447428` has 68 rows of36 bytes. **Row3's word
`447494` points to `1479B0`**, before sound row44 points to `126B20`.
`139390` invokes that table in ascending row order. The Ghidra function export
omits entry1479B0; original disassembly of its distinct aligned prologue and
actual table word establish the entry. `147790` is a different preceding
routine and must not be used as this constructor's address.
[Original caller loop](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/00139390.asm),
[original entry](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/001479b0.asm),
[original full table bytes](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/report.json).

| Actual instruction | Produced value / operation |
| --- | --- |
| `1479C4` | Advance source game-arena cursor by8 |
| `1479E6` | Publish allocated RNG pointer at global`4EE9D8` |
| `1479EC` | Shared RNG word at pointer`+0 = 000078A8` |
| `1479F2` | `time(0)` through original`32803E` |
| `1479FC` | Original CRT `rand328505` |
| `147A03` | Actual host-backed `GetTickCount337A40` |
| `147A0E`, `147A10`, `147A13` | Store `rand ^ tick ^ time` into RNG pointer`+4` |

Original helper `147A20` independently returns `time(0) ^ rand() ^ tick`;
it is listed but **not executed** in the freeze. Shared RNG, gameplay local RNG,
CRT rand state and sound-controller RNG are separate fields; none should be
collapsed into one initialized value.
[Separate helper](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/00147a20.asm).

The inherited fixture initially has supplied gameplay shared12345678/local0.
That holder is explicitly recorded before preparation. **Original1479B0
replaces the pointer and writes a new8-byte block**; it does not preserve or
use that supplied local0. The measured output is shared`000078A8`,
local`68FF312E`. These numeric seed outputs belong to this recorded measurement,
not constants for native startup.

The table establishes relative row3-before-row44 ordering. This trace invokes
selected constructors directly and omits the other table rows and broader CRT
startup. It therefore does **not prove that every normal boot reaches row3 with
an untouched CRT rand stream**. The two rand outputs below are verified for
this explicitly cold record, not a license to hard-code normal boot values.

## Cold CRT record, exact RNG updates and thread distinction

Original `32C609` first checks `FS:[24] < 2`. With the whole produced
compatibility context, `FS:[28] -> KTHREAD+28` isNULL, selecting fallback
global `56B920`. The source finds that pointerNULL, calls original
`calloc32E813(1,88)`, which requests original `HeapAlloc` with zero-memory
flag8. Source returns actual heap allocation`08000720`; it is not provided
as a constructed CRT record.
[Original TLS/fallback selection](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/0032c609.asm),
[actual calloc](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/0032e813.asm).

The source then publishes fallback pointer (`32C661`), stores literal locale
`469C18` at record`+54` (`32C663`), **seed1 at+14** (`32C66A`), flagsNONE
at+4 (`32C676`) and the actual original GetCurrentThreadID result at+0
(`32C67A`). Getter `2D8674` reads **`FS:[28] +12C`**. In this full compatibility
host-branch record, that field is0; the separately recorded real Wine worker
ID is320. The source correctly returns0 from its actual record. Neither
value is substituted for the other. This is compatibility-produced context,
not proof of an ordinary Xbox/title-thread ID or TLS producer.
[Actual getter](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/002d8674.asm),
[whole captured records](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/raw-host-context.json).

Every `328505` invocation executes, in order:

```text
record = original32C609()
u32 s = record[+14]
s = (s * 000343FD + 00269EC3) modulo2^32
record[+14] = s                      // original328519
return (s >> 16) & 00007FFF
```

The measured trace stores cold1, then original LCG states`0029E2C0` and
`C823F683`, returning41 and18467. One original calloc occurs; the second
`32C609` reuses the same produced record. The independent auditor checks the
full before/after words and ordered arithmetic, rather than deriving expected
results from a native sound implementation.
[Original rand instructions](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/00328505.asm),
[all ordered source seed stores](../analysis/multiplayer/map-pack/smg-audio-rng-startup/source-fixture.json).

## Controller producer and exact handoff

Actual full `126B20` allocates/zeros source20C, sets original policy byte
`+79=1`, runs `21A6E0`, initializes source384-by-BC pool and1200 backing, then
**`126C52` calls `21BC50`**. The unchanged controller creates its64-by-1C
pool and root1C and invokes `16D220`. Its seed sequence is:

| Site | Actual operation |
| --- | --- |
| `21BCCA` | New `time(0)` call, not a reused earlier time |
| `21BCD4` | New original CRT rand invocation, consuming the second LCG state |
| `21BCDB` | New `GetTickCount` call, not a reused boot tick |
| `21BCE0`, `21BCF4`, `21BCF6` | Store `rand ^ tick ^ time` at controller root`+8` |
| `21BCFF` | Root`+4 = NONE` |
| `21BD06`, `21BD09` | Root+2 highbit derives from sound+7F; root+3=80 |
| `21BD12..21BD1A` | Clear root+0C,+10,+14,+18 |

The root's lower7 bits of+2 are retained in the source bit operation. In this
source-zeroed case they are0 and sound+7F is0, producing+2=80. The actual
resulting root1C is
`00008080ffffffffef7bff6800000000000000000000000000000000`, seed`68FF7BEF`.
Original `21BC50` returns1. Full `126B20` subsequently executes original
`21C1D0`, `21F5A0`, `21FEB0` and reaches its actual driver request at126C90.
The fixture supplies **no result** for2205C0. The later initialized store
`126D35` is never reached.
[Executed controller](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/0021bc50.asm),
[full source sound caller](../analysis/multiplayer/map-pack/smg-audio-rng-startup/frozen-measurement/instructions/00126b20.asm).

The four clock requests use actual host UTC FILETIME and actual boot-time
milliseconds. Original `32803E` performs the original integer epoch conversion;
the auditor confirms returned seconds equal each measured UTC input divided
by10^9. Each resulting seed uses its **own** recorded time/tick pair and actual
rand result. These providers satisfy the recorded API shape; Linux clock
granularity/epoch and elapsed constructor costs are not asserted to equal Xbox.
[Clock API provenance and earlier measured clock kernels](SMG_AUDIO_STARTUP_1_1_RESEARCH.md).

## Supported consumer contract and remaining gates

A bounded native consumer may preserve the original u32 arithmetic, separate
shared/local/CRT/controller state, individual time/rand/tick call order and
controller initialization stores. It must retain the caller's real/recovered
CRT stream and actual provider clocks. The measured seeds above are fixture
outputs. Complete original prior rand consumption, normal title-thread/TLS
startup and full constructor-table allocation order remain unresolved.

The next audio gate is actual device/effect/voice/stream ownership after2205C0.
The genuine paused SDL open from the earlier study supplies no original
DirectSound caps, effects, pointers or packet consumption. Source driver
admission, normal map/cache lifecycle, actual scheduler dispatch and audible
playback remain unresolved; no old resource-ready flag is promoted. This
extension improves original initialization evidence without claiming whole
startup or complete audio. `native_gameplay_update_applied:false` is retained.
