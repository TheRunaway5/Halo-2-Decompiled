# Original replication allocator prerequisite

Research snapshot: 2026-10-12. The original replication allocator can construct and allocate a private King state bank in all three owned Xbox executables, without supplying an allocation return. A bounded original boot suffix initializes the genuine page provider, executes the complete concrete allocator constructor, and publishes the original wrapper global. The declared arena and wrapper/object backing make this an API composition; the earlier physical-memory allocator and `operator new` do not run. This establishes a usable original allocator for a subsequent positive `entry_update_decode` oracle, not a complete world boot or native-session result. [Frozen tool](../tools/research_replication_allocator.py), [original execution manifest](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

## Executed evidence and title-update authority

All three images independently execute three real boot suffixes/full constructors and **162 complete allocator virtual calls**. Each image observes 3,224 unique original instructions, all checked against its own pinned executable after execution. The separate static correspondence records **124/124 equal complete owner pairs**, including normalized operands, original branch/switch topology and raw relocated references. Nine literal constructor tables are retained; the corresponding three table byte sequences are equal across retail, TU1.1 and TU1.5. The report retains 202 complete instruction listings, including the larger boot owners whose earlier instructions were not executed. A containing-owner listing is not a claim that every branch in that owner executed. [Manifest and counts](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

| Source contract | Retail | TU1.1 | Xbox TU1.5 |
|---|---|---|---|
| Complete boot owner, statically retained | `81780` | `82190` | `85370` |
| Executed suffix, end exclusive | `81E99..81F49` | `828A9..82959` | `85A89..85B39` |
| Complete concrete constructor | `32A850` | `330DE0` | `334950` |
| Concrete allocator vtable | `454980` | `45BB30` | `460A78` |
| Arena page-provider vtable | `454970` | `45BB1C` | `460A64` |
| Wrapper global | `4D87F8` | `4DFDC0` | `4E5458` |
| Earlier arena-base global | `4D87FC` | `4DFDC4` | `4E545C` |
| Earlier arena-size global | `4D87F4` | `4DFDBC` | `4E5454` |
| Allocate, vtable `+14`, `retC` | `32A2C0` | `330850` | `3343C0` |
| Block info, vtable `+04`, `ret8` | `329F70` | `330500` | `334070` |
| Release, vtable `+00`, `ret8` | `329E40` | `3303D0` | `333F40` |
| Compact, vtable `+28`, `ret4` | `32A510` | `330AA0` | `334610` |
| Page allocation, provider `+08`, `retC` | `1A1A90` | `1A53F0` | `1AB6A0` |

These bindings come from the actual boot instructions and literal vtable bytes, then execute independently in each image. They do not assume the retail address applies to an update. The previously frozen orchestration report supplies the complete boot-owner provenance and changed retail/TU world-layout boundary. [Three-image allocator bindings](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json), [complete boot context](MP_KING_REPLICATION_ORCHESTRATION_SOURCE_RESEARCH.md).

## Declared backing and exact boot-prefix ABI

The reusable `setup_allocator(trace, name, arena_base, arena_size, context_base, stack, fs_exception_head)` operates on an existing original `Source`/`KingTrace`. Defaults deliberately avoid `KingTrace`'s `04000000..04040000` region: the arena is `05000000`, size `01000000` (16 MiB), and a fresh context page is `06000000`. Wrapper storage is context+0, allocator object context+100, diagnostic info context+600, and optional size output context+700. Arena/context bytes initially contain `CD`/`A5`. Every mapping and initial byte pattern is recorded. Different addresses are allowed only with fresh, nonoverlapping mapped backing. [Reusable function and checks](../tools/research_replication_allocator.py), [recorded setup inputs](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

At the first executed suffix instruction, ESI is the declared arena, EBX the declared `34`-byte object, EBP the declared `10`-byte wrapper, ESP the mapped caller frame, and `[ESP+14]` the declared arena size. EFLAGS=2 explicitly clears DF for the original `REP STOS`. The stack's first two saved-register words are0; the suffix consumes those through its original `POP EDI`/`POP ESI`. The flat `FS:[0]` exception-chain cell is backed and initialized to `FFFFFFFF`, restored by the original complete constructors. The tool does not fabricate `FS:[28]` thread identity. It stops before the next boot `POP EBP`, after the actual wrapper-global store. These are bounded entry/exit points inside the original boot owner, with real calls and returns between them. [Exact registers, stack bytes and constructor observations](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json), [retail boot instructions](../analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/retail-distribution_allocation-00081780.asm).

The suffix **does not read or write the earlier arena-base/size globals**; it consumes the register/frame values above. Their before/after bytes are recorded unchanged. The arena and object backing are therefore declared constructor API inputs, not replacements for the return of the earlier physical allocation or `operator new`. Every metadata pointer and subsequent block pointer is produced by the actual provider/allocator instructions. The wrapper global is written by the real suffix. A caller requiring complete physical boot must still execute those earlier producers and their platform services. [Suffix instructions](../analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/retail-distribution_allocation-00081780.asm), [setup assertions and scope](../tools/research_replication_allocator.py).

## Provider and constructor contracts

The original suffix calls the real tiny provider constructor, then writes provider+4=arena+1000, provider+8=arena+size, provider+FFC=wrapper+C, the real provider vtable, provider+C=`floor(size/4096)-1`, and wrapper+C=0. It zeroes precisely **`3FB` dwords (`FEC` bytes) at arena+10 through arena+FFB**. It does not zero through the counter pointer at+FFC. The page provider's complete allocation owner accepts alignment masks `FFF`, `1FFF`, `3FFF`, `7FFF`, or `FFFF`; it scans/updates the real bitmap, returns arena-backed pages, and increments the counter through provider+FFC. Its whole release owner clears the corresponding bits and subtracts pages through the same counter. [Retail boot initialization](../analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/retail-distribution_allocation-00081780.asm), [whole page allocator](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-001a1a90.asm), [whole page release](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-001a1a00.asm).

The complete concrete constructor receives ECX=object and 13 ordered stack arguments, then returns with `ret34`. The tool observes the actual entry arguments pushed by the original boot instructions and the immediate original return, rather than constructing a guessed call signature. [Complete constructor](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032a850.asm), [observed stack frames](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

| Argument | Actual retail value | Other-image contract |
|---|---|---|
| 1 | `46E490` | First literal16-byte-row table, 31 nonzero rows |
| 2 | `46E690` | Second literal16-byte-row table, 16 nonzero rows |
| 3 | `800` | Unchanged literal |
| 4 | `0` | Unchanged literal |
| 5 | `400` | Unchanged literal |
| 6 | `100000` | Unchanged literal |
| 7 | `46E7A0` | Four nonzero dwords, then0 |
| 8 | Declared arena | Actual real page provider input |
| 9 | `1` | Unchanged literal; distinct from argument10 |
| 10 | `0` | Stored at object+24; skips shared-global branch |
| 11 | `4` | Unchanged literal |
| 12 | `400` | Unchanged literal |
| 13 | `0` | Metadata lock's ownership-control argument |

The first/second/dword table addresses are `475978/475B78/475C88` in TU1.1 and `47B150/47B350/47B460` in TU1.5. Their raw table bytes and terminal records are equal. Their hashes are `da7a99dc2344c48641811989fd8e6d1baf379fa3745e1ba4bd98b5b66b961d17`, `1230dfb129d2c94e4f326d7030806a895ff2f5efa5ecbf3404a5f7219962a896`, and `b8e54678543f6b62fb08b1a252539f27a604590b279a19c1bb622ff926727e77`. [Raw tables](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

Exact stack accounting matters: after the constructor's prologue ESP is entryESP−28, so `[ESP+50]` is **argument10**, not argument9. The authentic0 value skips the shared-global allocator/CRT registration branch. The metadata lock is constructed with argument13=0; its thread-ownership checks are disabled. An earlier provisional interpretation of argument9 as object+24 was incorrect and is superseded by the observed entry frames and complete listing. No successful CRT, thread lookup, semaphore or lock-wait return is supplied by this proof. [Constructor argument loads and gates](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032a850.asm), [lock construction](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032ec10.asm), [observed frames](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

Construction stores the genuine object vtable, page-provider pointer at+1C, metadata at+20, object+24=0 and magic `DEADBEEF` at+28. It records47 pool rows at+2C and metadata byte request `1AFC` at+30. All metadata and auxiliary pool allocations execute through the real page provider. The declared 16 MiB cases finish construction with metadata `05001000` and **10 consumed pages**. Wrapper publication writes wrapper+0=object, wrapper+4=0, wrapper+8 byte=0, and the actual wrapper global. Padding bytes remain the original declared sentinel; they are not assumed initialized. [Complete constructor](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032a850.asm), [raw post-construction object/wrapper/provider page](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

## Actual allocation, info, release and compact results

The public virtual allocation owner receives ECX=object and `(signed_size, actual_size_out_or_zero, clear_byte)`. It checks the magic, returns0 for negative size, maps requested0 to1, and invokes the real size-class allocator. The production replication call uses `(state_size,0,0)`. Its 40-byte path goes through retail `32A2C0→32C110→32BE80→32E500→32FBD0`, with real page allocation and pool setup when required. The original info owner receives `(block,info_out*)`, and in these cases writes the actual byte size to its first dword only. Remaining info sentinel bytes stay unchanged. Release receives `(block,FFFFFFFF)` and executes the real free owners. [Public allocate](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032a2c0.asm), [size-class selection](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032c110.asm), [info](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-00329f70.asm), [release](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-00329e40.asm), [actual replication caller](../analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/retail-entry_update_decode-0008aac0.asm).

Each independent image performs the same declared sequence, keeping positive blocks live until reverse-order release: sizes `−1,0,1,4,39,40,40,40,41,64,127,128,1024,2048,4096,8192,65536`. Negative1 returns0 without increasing the10 constructor pages. Requests0/1/4 report actual4,39/40 report40,41 reports48,127/128 report128, and the larger listed requests report their listed sizes. The three repeated40-byte requests return `05010228`, `05010250`, `05010278` after the preceding39-byte request. All positive info/release ALs are1; pointer, size, page-count and payload-head results match across all three executables. These are source-generated outputs for the declared sequence, not fixed addresses a caller should manufacture. [Raw calls and results](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

After reverse releases, the real compact owner with argument0 returns AL=1. A new40-byte allocation returns source pointer `05010000`; a subsequent `(40,actual_size_out*,1)` allocation reports actual40 and has40 zero bytes. Both are freed through the real release owner. The final provider consumes28 pages: release/compact do not imply the entire arena has returned to its initial bitmap state. No allocation failure due to exhaustion, failed compact/retry sequence, invalid free, stale pointer query or concurrent wait is claimed. [Raw final state, clear output and complete call list](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json), [whole compact owner](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/retail-owner-0032a510.asm).

The virtual methods themselves do **not** update wrapper+4. The replication caller increments that live-block count after successful allocation and decrements it after freeing; this probe calls the methods directly, so wrapper+4 remains0. An entry-decoder oracle must let the original entry owner perform those operations and retain its actual cleanup behavior. [Original update decoder](../analysis/multiplayer/game-variants/king-replication-orchestration-source-20261012/complete/retail-entry_update_decode-0008aac0.asm), [raw wrapper results](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).

## Reproduction and limits

The [frozen tool](../tools/research_replication_allocator.py) pins all three executable SHA256 values, their function inventories and the frozen orchestration report. Reproduce into a new directory using:

```sh
.tools/python/bin/python tools/research_replication_allocator.py \
  --output analysis/multiplayer/game-variants/replication-allocator-source-20261012/reproduction-new
```

The saved [reproduction comparison](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/reproduction.json) checks every generated instruction listing and report byte. Diagnostic1 preserves the first retail constructor probe with a different declared base; diagnostic2/3 preserve schema/ABI-observation development. They are not substituted fixtures or packet executions. The final report/tool are independent of all existing native/oracle/fixture work. [Complete manifest](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json), [reproduction record](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/reproduction.json).

This work resolves the **positive private-bank allocator boundary** for original replication decoder research. It does not prove original physical-memory boot, arena-size policy, low-memory retry behavior, aggregate packet acceptance, occupied-world callbacks, native transport, normal King admission, all39 variants, or framebuffer/hardware behavior. No native source, CMake, test, fixture, progress or admission file is changed. [Recorded scope and empty patch/return inventories](../analysis/multiplayer/game-variants/replication-allocator-source-20261012/complete/report.json).
