# Original sound-cache constructor in native C++

The new `source_sound_cache_startup` implements the complete CPU owner `21A6E0`, its `13E8E0` page constructor, and their `16D070/16D0D0/16D220` pool construction/reset. It requires real raw allocation, retained memory and protection services. Source code writes every pool/page field and reserves the physical span in original order. No provider writes a cache-valid/read-completed flag or fabricates an initialized device.

This closes the fixture-only cache callback in the earlier [portable startup study](MP_SOUND_STARTUP_NATIVE_INTEGRATION.md). The new normal and sanitizer composition executes `source_sound_startup` with this actual cache constructor and matches both source-owned arenas/globals. It still stops at the genuine platform constructor request. No ordinary map activation, asynchronous cache reads, admitted voice, launcher connection or audible parity is claimed.

## Authority and scope

The user-owned disc1.1 executable is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`. The authenticated user-owned1.5 executable is `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

Eight complete bodies and all290 paired instruction rows match normalized operands, calls and internal branches. Instructions and immutable operands are checked against both original images. Actual owning execution is disc1.1; no supplied1.5 world is promoted as another original execution.

| Disc1.1 | Authenticated1.5 | Scope |
|---|---|---|
| `21A6E0` | `21EFE0` | Complete cache CPU constructor |
| `13E8E0` | `13FC50` | Complete page allocator constructor |
| `16D070` | `16E450` | Raw pool layout |
| `16D0D0` | `16E4B0` | Raw pool header |
| `16D220` | `16E600` | Pool reset |
| `21AAF0` | `21F3F0` | Complete delete callback body correspondence; retained identity, not invoked by this constructor |
| `21AAB0` | `21F3B0` | Complete busy callback body correspondence; retained identity, not invoked by this constructor |
| `11E150` | `11F3F0` | Complete allocator wrapper correspondence; actual raw OS backing is a named platform boundary |

The paired provider packet is [providers.json](../analysis/multiplayer/integration/sound-cache-startup-native/paired/providers.json), SHA-256 `648d78324c1c0863b6fc22116834863583dc3db417861a352cf981ed327462dd`. The production constants struct contains two32-bit fields: delete callback identity, then busy callback identity. Constructor names are independently verified immutable ASCII `xbox sound` and `xbox sound cache`; source `strncpy32` padding is retained. These identities are not successful callback returns, host device capabilities or cache readiness.

## Original order and retained records

`21A6E0` reads the real general allocator identity and requests `208C` bytes. A nonzero result receives a512-entry pool, stride16, alignment shift0; source then adds flag bit2. Pool valid byte`29` remains0. The pool pointer is published before the second raw allocation.

The second request is `30FC` bytes. Original `13E8E0` unconditionally constructs its embedded pool at allocation`+70`:512 entries, stride24, static allocator identity from original`46FBFC`. The source sets this embedded pool valid and resets it, then clears the first`70` bytes and writes the page metadata. The caller sets allocation policy2. It does not mark a page loaded or start a worker.

| Page allocator offset | Source-produced field |
|---|---|
| `00..1F` | Original name, padded to32 bytes, final byte forced0 |
| `20`, `24` | Imported delete and busy callback identities |
| `28` | Constructor value0 |
| `2C` | Policy2, written by the owning caller after the constructor |
| `30` | Available page count`C0` =192 |
| `34` | Page shift`E`,16 KiB |
| `38` | Initial frame1 |
| `3C`, `40` | List head/tailNONE |
| `44..60` | Eight `7FFFFFFF` request-cost sentinels |
| `64` | Real embedded pool address, allocation`+70` |
| `68` | Source cookie`77656565` |
| `6C` | Captured general allocator identity |

The full source allocation extents are `208C` and `30FC`; actual raw OS backing is rounded to`3000` and`4000`. The source initializes headers, bitmaps and record salt words; untouched raw bytes remain actual zero-filled allocation bytes. Both complete rounded buffers are compared. `512` metadata records and`192` physical pages are separate counts.

The caller then reads the existing physical-arena index and corresponding upper/lower limits. `candidate = upper - 300000` uses32-bit wrap. Admission compares **signed** candidate and lower. On admission it stores the candidate upper bound before protection. Candidate0 publishes physical buffer0 through the separate source return path. Otherwise it ORs bit31 and requests `(buffer,300000,4)` from the real protection provider. The physical buffer is published only after that request returns. A missing protection service leaves the completed CPU records and changed arena bound at that exact boundary; it cannot publish a fabricated final buffer.

`SoundCacheStartupState` retains both allocator identities, the three cache globals, physical index and five lower/upper slots. The raw memory provider must return actual owned storage; `raw_allocate(owner,size)` must own its output backing, and `raw_protect` must protect the already owned physical span. Native bounds failures are unavailable backing, not manufactured source false/success returns. The source's second-null allocation dereference is preserved as a backing stop; general allocator failure branches have static authority but this freeze does not force an OS failure.

## Measured original cases and native comparison

The source oracle invokes unchanged full`126B20` on the prior independently produced heap/CRT/physical/game-arena state and whole recorded compatibility context. Its source calls complete`21A6E0`, then continue until actual`2205C0`; no device return is supplied. Actual `mmap` backs the two requested raw blocks and actual `mprotect` performs each requested protection. Deterministic32-bit aliases are explicit platform inputs, not retail Xbox addresses. Nondeterministic raw host addresses remain separate evidence files and are not called byte-identical repeats.

The first case uses genuine earlier produced arena inputs. Five further cases explicitly change only constructor physical-index/limit inputs before entry; they are source branch controls, not ordinary startup outcomes.

| Case | Original result |
|---|---|
| Original produced arena | Complete pools; reservation and actual protection |
| Candidate equals lower | Equality admits; actual protection |
| Insufficient arena | Complete pools; no upper change/protection; buffer0 |
| Candidate0 | Upper becomes0; no protection; distinct buffer0 return path |
| Signed-negative candidate with lower0 | Signed comparison rejects; no protection |
| Second arena slot | Uses actual declared slot1 inputs; slot0 preserved; actual protection |

Across six cases, original records contain4,246 ordered writes; native comparisons match4,090 represented instruction writes and complete raw buffers/globals. The remaining C `strncpy` primitive stores are represented with instruction identity0, rather than assigned invented original PCs; their full output bytes are checked. Three actual protection requests execute in each normal/sanitizer comparison. Missing allocation/protection providers stop without success. The combined portable startup/cache test matches the full`0x23000`-byte startup arena and complete cache blocks at the exact platform request, with original controller clock/CRT outputs replayed as declared inputs.

The source record is [measured/report.json](../analysis/multiplayer/integration/sound-cache-startup-native/measured/report.json), SHA-256 `dcec5a5f112f44cf262af78e119b6163a643d1c1f2b09fffc694e003bb58d436`. Native fixtures are [fixtures.tsv](../analysis/multiplayer/integration/sound-cache-startup-native/fixtures.tsv), SHA-256 `48dba7560ccbc1e90a66b5bb39c4904a14b6347828c2b97271bc92863b84e6af`. Independent source/pair/export repeats, raw host providers, fresh standalone source snapshots, exact compiler/commands/environment/stdout and the final audit are retained in this separate directory. Earlier freezes remain unchanged. Fresh standalone binaries do not establish that every current shared archive was rebuilt from a matching whole tree.

The [audit](../analysis/multiplayer/integration/sound-cache-startup-native/audit.json), SHA-256 `26f4f659d3a77a78d0e1c165a467836ec2f6b688b773bbcc2b71f07d5a55141a`, verifies23 independently byte-equal files,580 original paired instruction rows and169 previous pins. Raw host addresses are checked as actual allocation evidence, retained independently, and excluded from byte-equality claims. Both fresh normal and GCC ASan/UBSan runs pass, including real host `mmap/mprotect`, complete output bytes, source write order, missing-service stops and the full portable CPU startup/cache composition.

## Reproduction and next owning gate

Run into a new ignored directory; never regenerate immutable paths:

```sh
.tools/python/bin/python tools/oracle_sound_cache_startup_native.py --output /tmp/halo2-cache-source-repeat
.tools/python/bin/python tools/research_sound_cache_startup_u5.py --output /tmp/halo2-cache-pair-repeat
.tools/python/bin/python tools/export_sound_cache_startup_fixtures.py --report /tmp/halo2-cache-source-repeat/report.json --providers /tmp/halo2-cache-pair-repeat/providers.json --output /tmp/halo2-cache-fixtures.tsv
analysis/multiplayer/integration/sound-cache-startup-native/native/sound_cache_startup_tests /tmp/halo2-cache-fixtures.tsv analysis/multiplayer/integration/sound-startup-native/fixtures.tsv
```

Ordinary map activation and source virtual-voice admission remain next. Complete`126DE0` publishes real `ugh!` from current`matg`, resets source listeners/ambient fields, calls actual clocks, `127540→2217E0` and `192CA0` environment/DSP ownership. It is **not** a cache-reset callback. The observed original map path reaches real driver/effect dependencies; setting system`+7A` directly would skip these owners. Existing native SDL device/packet completion support is a genuine separate platform service, but it does not replace the original scheduler, class fades, local player/listener/RNG owners, voice factory or map lifecycle. Cache construction alone makes none of those available.
