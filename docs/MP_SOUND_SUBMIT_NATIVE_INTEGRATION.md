# Raw sound submit and controller CPU ownership

The new `sound_submit.hpp/.cpp` implements the complete original `21F220` CPU submit path and its metadata, effect, controller and raw-pool control helpers. It requires complete real admission, promotion, source-factory, effect-update and controller-seed providers when their original branches are reached. It supplies no availability flag, device, voice, listener, permutation, RNG seed or platform success. The existing semantic first-person request cannot be padded into the original request: callers must retain its complete 160-byte backing and source address.

This checkpoint establishes the cold submit return and a complete optional-effect allocation/binding/rejection/removal cycle. It does not connect launcher reload events to playback. The genuine source system is still unavailable at the previously measured `2205C0` device request.

## Original authority and actual execution

Evidence is in `analysis/multiplayer/integration/sound-submit-native/`. Fifteen full bodies have unique normalized branch/call/operand correspondence in the authenticated user-owned 1.1 and u5 executables. All 1,510 instruction rows are independently checked against original image bytes:

| CPU owner | 1.1 | u5 |
|---|---:|---:|
| Whole submit | `21F220` | `223B20` |
| Controller request | `21FE40` | `224740` |
| Custom/default metadata | `18EAC0`, `18EBA0`, `21B5B0` | `18FE80`, `18FF60`, `21FEB0` |
| Bind/remove effect | `21FD70`, `21FE00` | `224670`, `224700` |
| Raw pool allocate/release/scan | `16D2C0`, `16D4C0`, `16D680` | `16E6A0`, `16E8A0`, `16EA60` |
| Find/retain/create/release controller | `21BA70`, `21BB40`, `21BBA0`, `21BD90` | `220370`, `220440`, `2204A0`, `220690` |
| Controller initialization | `21BFA0` | `2208A0` |

The oracle composes unchanged whole `21F220` with the complete prior source-created 143,360-byte `126B20` constructor arena at its actual unresolved device request. The original system byte `+78` remains zero. Literal Lockout tag-table bytes and current matg/ugh/scenario roots are explicit raw ABI inputs. They do not establish the full 88-callback map activation owner. The u5 execution consumes the same declared prior-1.1 startup layout and caller backing; ordinary whole u5 startup is not claimed.

The complete raw request is retained from the independently frozen original `18BA10` output in `smg-audio-sample-boundary/source-fixture.json` (SHA `222245ded354b337a2e624beef2dbaf2609aa1721cecd383d0ef70b1756495d6`). It contains original unowned stack bytes and now obsolete source pointers. Supplying that whole record to this separate ABI experiment is explicit composition, not an actual newly produced live first-person event or current world transform. No omitted field is initialized to a guessed zero.

There are 2,574 source cases: all 1,279 resident authored Lockout sound datums per image, plus eight declared interface/flag controls per image. All return the original `NONE`. The ordinary records have no mutations. Four interface controls per image execute a genuine optional-effect allocation, request binding, cold-controller rejection and pool release, producing 196 exact ordered writes across eight cases. No allocation or controller return is supplied. Both source pools and their identities come from their unchanged prior constructors.

The module matches every return, all 196 ordered writes, the complete arena and the complete caller record in normal and GCC ASan/UBSan builds and independent reruns. The test uses real `NativeSoundStorage` OS mappings and 4,086,272 bytes of literal serialized Lockout metadata/table backing; it has no fixture allocation, native-device, admission or source-factory callback. Source reports/listings, complete-body correspondence and exported fixtures repeat byte-identically. The earlier seven-body exploration and overly strict “returned means no stores” diagnostic remain separately preserved. The latter exposed the genuine optional-effect cycle; it is not the final expected result.

## Raw request and metadata contracts

`SoundSubmitState` retains explicit system, tag table, current matg, current ugh, current scenario, effect pool, controller pool and controller-root addresses. These are mutable owner inputs, not source-image defaults. `memory(address,size)` must resolve their real backing and exact extent. Missing storage fails at that phase; no synthetic null object or success is returned.

`source_sound_custom_playback` preserves the whole `18EAC0` interface dispatch. `NONE` and interface kind 0 use the sound definition's signed byte `+0D` and actual ugh `+34` pointer, with 0x34-byte stride; kind 0 retains the source group-check behavior without adding a sound-NONE check. Kind 1 requires matg soundGlobals count/pointer and its `+0C` datum. Kind 2 reads scenario `+324`. The latter two resolve the selected definition's `+4` block pointer plus 4 and low30 interface index times 0x38. Kind 3 returns null. Group lookup uses the original signed low16 index, while definition lookup uses unsigned low16. Host bounds are explicit storage failures, not new original selection gates.

Actual Lockout SMG reload definitions have custom-playback index `-1`. Its two authored ugh custom rows also have `+2C=0`; ordinary submit therefore takes the non-controller path. A declared kind-1 interface on the reload request instead selects the literal matg-referenced effect, whose real count/pointer gates allow the effect cycle before availability rejection. This is not an ordinary reload's selected interface.

`source_sound_controller_request` preserves `21FE40` exactly, including its original ESI re-read after the EAX fallback selection. It only writes caller output on the original pointer branch. The source null/fault behavior is not repaired by substituting the fallback pointer. Counts are signed DWORDs. No controller/effect is created merely by resolving metadata.

The original raw `21F220` record is 0xA0 bytes. Whole `18BA10` independently retained in the prior producer freeze writes flags `+0`, priority `+4`, copies 17 DWORDs (68 bytes) to `+0C`, owner `+50`, callbacks `+58`, optional 40-byte marker to `+5C`, 16-bit data size `+8C`, and optional interface `+98`; other bytes retain caller backing. Existing semantic location48/marker40 alone cannot reproduce the 68-byte copy or all untouched bytes. The new API accepts an explicit source-address alias whose complete 160 bytes are actually backed, rather than allocating a zero-filled semantic replacement.

## Effect, controller and scheduler ordering

`21F220` resolves the optional controller metadata before testing audio availability. A genuine positive request allocates from the existing raw effect pool through complete `16D2C0`, writes the resolved request at effect `+10` and `NONE` at `+8`, preserves the source optional playback flag update at raw request `+0D`, then binds through `21FD70`.

Binding writes raw data size `0x30`; retains the prior callback at raw `+88` when flag `0x20` is set, otherwise marks effect `+4` bit 3 and stores zero there; stores the actual allocated effect datum at raw `+84`; installs the imported callback identity at `+58`; and sets raw flag `0x20`. It calls actual retain/create controller ownership according to raw flag `0x10`. Those complete wrappers first consume the genuine system `+78/+79/+7A` gates. In the measured cold context they naturally return `NONE` without a seed, clock or callback reply.

Raw pool allocation/release preserve original signed cursor/high-water/capacity comparisons, bitmap writes, full zero/poison stores, salt wrapping, counters and release order. They do not assign a salt or identity. Controller scanning retains bitmap-selected records and the original 0x1C-byte lookup. Reference counts wrap in their original seven bits. Genuine first-reference initialization requires the complete `147A20` seed producer (original ordered time/CRT-random/tick consumers) and retains `21BFA0` field/write order. That positive branch is statically ported, not executed or seeded by this checkpoint.

After binding, submit checks system `+78/+79/+7A`, calls complete `128380` admission with its source-owned local DWORD, then complete `128610` promotion with the sound reference and complete `127750` creation. A genuine successful source writes effect state 1, source datum and low16 priority, then requires complete `21F740` phase 0. A rejected source calls `21FE00`, releasing a retained controller when present, clearing effect `+8`, and releasing the actual pool datum through `16D4C0`. The non-controller branch retains the same availability/admission/promotion/factory order. No missing provider is interpreted as rejection or success.

The only new immutable operand is `SoundSubmitConstants.effect_callbacks`: source `451318` / u5 `455C18`, from the actual `21FDBE` immediate. `paired-owners/providers.json` retains each full 32-byte callback table. It is an original callback identity requiring the owning implementation; it is not a host voice, plugin, playback result or default callback.

## Validation and next live gate

Test CLI: `sound_submit_tests <fixtures.tsv> <literal-lockout-regions.bin>`. `native/commands.json` and `native-repeat/commands.json` retain exact compiler/run commands, environment, outputs and directly observed source/binary pins. The production storage dependency matches its separate frozen source. This is a fresh standalone source build; no shared archive freshness or launcher integration is asserted. No shared CMake/runtime/Simulation/renderer/audio/OriginalData files were edited.

The next live dependencies are complete `128380` admission and its actual listener/audibility/local-RNG inputs, `128610` promotion, `127750` factory/cache/permutation/voice owners, and the normal current-map/raw-caller producers. The real SDL packet backend and decoded sample coverage are available separate platform/resource evidence; they do not satisfy these source ownership gates by themselves. Whole `21F220` returning `NONE` on the genuine unavailable state cannot be reported as audible reload parity.
