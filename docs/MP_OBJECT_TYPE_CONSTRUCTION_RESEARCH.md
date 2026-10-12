# Original object-type construction dispatch

Complete native `10CA30`, `10CBD0` and `10CC20` now preserve original group classification and preparation/construction callback order over actual retained object storage. The comparison passes 17 original roots, 10 required lower callback boundaries, 213 ordered metadata reads and 532,345 complete output bytes. These dispatch owners are a necessary part of the generic `B8A30` Unit constructor; they do not yet construct a native live Unit or make Q throw a grenade.

The isolated scope is `analysis/multiplayer/integration/spawn-source/object-type-construction`. New code is `object_type_construction.hpp/.cpp`, its test and three research/export tools. CMake gains only one Systems source and one test. Unit, Simulation, runtime, renderer, frontend, shader, release and prior frozen files are unchanged by this slice. Literal original buffers and table payloads remain in ignored analysis.

## Complete owner contract

`source_object_definition_type` implements complete `10CA30`: sign-extend the definition datum's low16 index, read its actual tag-table group DWORD, then search all 13 original type records in ascending order for the first matching record+4 group DWORD. It returns -1 when none matches. This is group classification, not the definition's first type word. Actual Lockout tags exercise types0,1,2,3,4,5,6,10,11 and one nonobject group. Other type records are retained as literal original metadata but have no authored Lockout query fixture here.

`prepare_source_object_type` implements complete `10CBD0`. It reads the same mutable C4 creation request's definition, classifies its group, obtains the selected type record, and traverses the record+84 DWORD pointer bank through its first NULL. For each non-NULL slot it reads slot+28 and invokes each non-NULL preparation callback with that same request. It ignores callback EAX. Both the initial bank read and the repeated in-loop read are retained.

`construct_source_object_type` implements complete `10CC20`. It indexes the actual header pool using low16 of the full incoming datum and reads signed body+AA for the type record. It begins with AL1, traverses the same bank, reads slot+2C, and invokes a non-NULL constructor only while the prior result is nonzero. Callback arguments are full unit datum, same request and same mutable failure byte. Nonzero callback AL is normalized to1. A false result prevents later callbacks but still reads all remaining bank pointers and method words through NULL. No salt check is invented inside this routine; salted caller admission is a separate owner.

The provider API requires selected-image type addresses, bounded immutable byte reads and actual signed-index tag group lookup. Method addresses are opaque original identities, never executable host pointers. Every admitted method requires an actual callback owner. Missing callbacks, metadata, body or header backing throw; neither success nor NONE is supplied. Host bounds checks reject undersized C4 requests, absent failure backing, escaped 32-bit pointers and unterminated signed16 banks instead of reading unavailable host memory. These explicit host errors do not claim original malformed-memory behavior.

The original type-bank loop index is signed16. Each wrapper first reads record+84, rereads that slot in the loop, then reads the next indexed slot after increment. The native module preserves that order. No wrapper writes retained memory itself; all observed changes belong to its required lower constructors. Callback-backed body growth must keep request/header backing valid throughout the wrapper.

## Actual original preparation and comparisons

The observer reuses the immutable `PlayerTrace` preparation and unchanged source join, spawn/loadout and early-cluster prefix. Its first four stage records equal the existing positive player-visibility fixture exactly. The actual source-created biped is `E26F0000` and weapon `E2700001`. No final body, header, manager, action offset, selector, count, readiness flag, clock or resource success is supplied by this observer.

Natural `B8A30` callers execute two preparations and two constructions. Biped preparation invokes `DDAC0`; biped construction invokes `C52C0` then `DDB40`. Weapon preparation has no non-NULL method; weapon construction invokes `F9900` then `101BB0`. These original callbacks execute unchanged in the natural capture. The biped body grows from 5,302 to 5,750 bytes during the actual `C52C0` callback. Captured sizes and offsets are observations, not production inputs.

Ten additional unchanged `10CA30` roots use actual authored Lockout datums. Three separate `10CC20` controls retain the already constructed source unit and copy only a literal source-produced C4 request/failure byte into host scratch. They declare lower callback AL sequences `[0]`, `[2,0]`, and `[255,128]` to establish false short-circuit and nonzero normalization. Those declared service results are not natural successful construction fixtures.

The native test compares each complete wrapper input/output state, actual callback identities/arguments/order, same request/failure backing, and all metadata read addresses, widths and bytes. For the ten callback boundaries only, the test applies the captured complete original before/after buffers and AL. This isolates complete dispatch correctness; it does not port or validate those entire lower callback bodies. The production module contains no replay provider. Full state includes the actual 76-byte pool record, 2,048×12 header backing, 132-byte world record, every allocated body, C4 request and failure byte where present.

The first exploratory observer accidentally emitted duplicate callback rows while a callee kept its return address at ESP. Its report is preserved under `measured` and explicitly excluded in `observer-correction.json`. The accepted observer admits one row per actual return-address/entry-stack frame. `measured-v2` and `repeated-v2` reports/listings agree independently. Source instructions are unpatched and checked against original image and mapped guest bytes.

## Authenticated later-update correspondence and data seam

| Update1.1 | Authenticated u5 | Complete instructions |
| --- | --- | ---: |
| `10CA30` | `10D950` | 17 |
| `10CBD0` | `10DAF0` | 25 |
| `10CC20` | `10DB40` | 44 |

Complete instruction operands, internal branch topology and relocated direct call/data references match after independently locating the u5 bodies. This is static full-body correspondence, not a claim that u5 world startup or its lower constructors executed here.

The exact selected-image packet is `u5/metadata-packet.json`. It retains each image's 13-pointer type bank, record first8 bytes including group+4, every record+84 slot through NULL and each slot+28/+2C pair. Source banks are1.1 `46FAD0` and u5 `474B90`. Existing `OriginalData.unit_action_tables` holds separate action descriptors/base extents and does not provide these inheritance banks. A future importer must read this packet's exact addresses from the whole-fingerprint-validated selected image. There is no copyrighted payload embedded in the native module, no guessed record extent, and no implicit cross-image pointer conversion.

## Actual Q integration boundary

Current native keyboard capture retains a Q pulse, but `Controls` and the runtime simulation submission do not deliver it. Original grenade request dispatch occurs in `150F00` before `151C90→C8140`: the current primary weapon definition+12C bit12 can clear input flag04000000; the admitted branch clears eight DWORDs, writes action type22 and calls `E7F30`. A host Q adapter can be explicit, but it must call that action chain against genuinely owned Unit/action storage. `C8140` alone does not deliver the grenade action.

The existing frozen native dispatcher and `E95E0` begin body can resolve signed unit+346 action storage only if the complete constructor chain produced it. Native Scene currently retains individual Source fields and typed inventory; it has no source-complete raw Unit body/header/managers. These three new dispatch owners remove the inheritance-bank gap in that construction chain. They do not authorize copying the observed 5,750-byte final Unit, setting +12A/+33E/+346 to observed offsets, forcing grenade inventory/phase or replacing required effects/audio/world services with successful results.

The shortest remaining complete storage owner is generic `B8A30` (614 original instructions; independently matched u5 `B9B70`). It orders this preparation, actual `BD170` header/body allocation, generic field writes/component appends and real type construction before its post-construction world callbacks. Complete native `BD170` already owns exact requested-byte zeroing and allocation failure release, but its header/body storage services must own genuine allocation. The generic constructor's entire retained-store and failure/cleanup sequence, required biped preparation/construction, and current world/resource/transform owners must compose before a live Unit bridge can deliver Q. Root's independently frozen `B8820/BBCB0` request/salted lookup is separate and remains unchanged.

## Reproduction and authority

Normal GCC-O2 and Clang AddressSanitizer/UndefinedBehaviorSanitizer standalone comparisons pass. Fresh registered production normal and GCC AddressSanitizer/UndefinedBehaviorSanitizer comparisons pass (0.03/0.11 seconds for the focused test), with matching direct output and an independent native repeat. Thirteen original report/listing/packet/fixture repeat pairs are byte-identical. The selected new archive member is copied and agrees with its compiled object; other linked archive members are observations, not a claim that all were rebuilt from an audited full source snapshot.

Use the exact commands in the scope's `REPRODUCE.md`. The source authority is original `.03` SHA `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, actual Lockout SHA `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`, and authenticated external u5 SHA `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`. The inherited complete compatibility KPCR/host storage, game options/property delivery and resource boundaries remain as declared by immutable player preparation. This is not full title startup, natural local-device admission, original Xbox OS equivalence, full native world construction or live grenade gameplay parity.
