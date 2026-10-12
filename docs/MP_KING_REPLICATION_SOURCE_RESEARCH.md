# Original King replication difference and application contracts

Research snapshot: 2026-10-12. The registered Xbox King callbacks use a 40-byte update bank: 36 bytes of shared engine data, a signed hill identifier, and a 16-bit occupant mask. Their difference/application behavior and relevant caller bodies correspond across retail, TU 1.1, and TU 1.5. This note establishes original CPU contracts; it does not establish native transport compatibility, a populated replicated match, or admission of any King variant. [Original-byte report](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json), [prior King engine research](MP_KING_ENGINE_SOURCE_RESEARCH.md).

The [inspection tool](../tools/inspect_king_replication_contracts.py) executes only the original engine registration constructors. It then reads the real King engine and entity-definition vtables, saves complete instruction listings, and compares 20 owners between retail and each update: 60 listings and 40 complete static pairs. It does **not** execute the replication callbacks, codecs, world services, or native implementation. Root-owned replication oracle/native work is separate. No instruction patches or supplied returns occur in this inspection. [Manifest and scope](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json).

## Original authority and correspondence

| Image | SHA256 |
| --- | --- |
| [Retail XBE](../assets/disc/default.xbe) | `03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d` |
| [TU 1.1 XBE](../assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03) | `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377` |
| [Owned imported TU 1.5 XBE](</home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe>) | `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd` |

Original registrations select source engine type 4. Its object/vtable are retail `47FC88/45C878`, TU 1.1 `487190/463CE8`, and TU 1.5 `48C4A0/468D88`. The distinct 27-slot entity-definition tables are `451240`, `4583B8`, and `45D400`; each actual name getter returns `king-engine-globals`. Engine slot 39 and entity slot 0 share the literal current identifier getter, returning 3, rather than the registered source engine type 4. All three entity getters for identifier, bank size and supported mask were additionally checked as literal `MOV EAX, immediate; RET` bytes. [Registered tables and entity pointers](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json), [nine getter byte checks](../analysis/multiplayer/game-variants/king-replication-source-20261012/reproduction.json), [earlier getter execution](MP_KING_ENGINE_SOURCE_RESEARCH.md).

| Owner | Retail | TU 1.1 | TU 1.5 |
| --- | --- | --- | --- |
| Shared initialization, engine slot 41 / `+A4` | `24E320` | `2544C0` | `25C250` |
| Shared difference, slot 42 / `+A8` | `24E360` | `254500` | `25C290` |
| Shared apply, slot 43 / `+AC` | `24E490` | `254630` | `25C3C0` |
| King initialization, slot 44 / `+B0` | `2BDB40` | `2C3E80` | `2C7900` |
| King difference, slot 45 / `+B4` | `2BDB80` | `2C3EC0` | `2C7940` |
| King apply, slot 46 / `+B8` | `2BDC00` | `2C3F40` | `2C79C0` |
| Entity size owner, slot 11 | `9A2C0` | `9AE60` | `A42F0` |
| Entity write owner, slot 14 | `9A3A0` | `9AF40` | `A43D0` |
| Entity read owner, slot 15 | `9A530` | `9B0D0` | `A4560` |
| Entity initialize dispatch, slot 19 | `A45E0` | `A53B0` | `AE7F0` |
| Entity difference dispatch, slot 20 | `A4650` | `A5420` | `AE860` |
| Entity apply dispatch, slot 23 | `A4730` | `A5500` | `AE940` |
| Entity creation/application, slot 22 | `A4880` | `A5640` | `AEA90` |
| Shared update writer | `A4AB0` | `A5870` | `AECC0` |
| Shared update reader | `A4E20` | `A5BE0` | `AF030` |
| Player-property refresh | `1523C0` | `153B10` | `154EF0` |
| Retained recording refresh | `196780` | `198280` | `19B180` |
| Received phase application | `15CBA0` | `15E200` | `15F5E0` |
| Game-over begin | `1389C0` | `13A130` | `13B4A0` |
| Recording team-is-active predicate | `161E60` | `163500` | `1648E0` |

Every row above is a complete static owner comparison, including internal branch structure, relocated image operands and referenced `.rdata` prefixes. Direct calls to other mapped owners must resolve to the corresponding image's actual entry. This establishes these bodies' correspondence; it does not establish every transitively called world/network service. [Comparisons, references and 60 listing hashes](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json).

One relocation needs an explicit exception to the generic token comparator: recording refresh compares its row pointer to an immediate end address. The tool checks that only this token differs and that each literal interval contains exactly 16 rows of `0x18` bytes: retail `511A74..511BF4`, TU 1.1 `519264..5193E4`, TU 1.5 `51ED74..51EEF4`. This is an address relocation, not a behavioral update. Earlier diagnostic directories remain preserved and are superseded by `complete/`. [Explicit bound records](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json).

## Literal bank and field meanings

`MP` below means the object pointed to by retail `4E9AE8`, TU 1.1 `4F10B8`, or TU 1.5 `4F63B8`. `King` is the state pointer at `51ECC8`, `526558`, or `52D8A0`. The table separates byte-proven storage from reconstructed names. [Shared difference](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-base_difference-0024e360.asm), [King difference](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_difference-002bdb80.asm), [reconstructed globals layout](../tools/reference/halo2-decompiled/include/unknown_157450.h).

| Update bit | Bank offset/type | Live source field | Meaning established here |
| --- | --- | --- | --- |
| `01` | `00`, u16 | `MP+06` | Eight-bit encoded team-selection mask; reference name remains `field_c_2`. |
| `01` | `02`, u16 | `MP+08` | Nine-bit assignment mask selecting the nine team-designator entries. |
| `01` | `04`, u16 | `MP+0A` | Reference name `team_flags`; finer gameplay interpretation is not promoted to a new rule. |
| `01` | `06`, u16 | `MP+0C` | Reference name `playing_teams`; current connected teams in the reconstructed producer. |
| `01` | `08`, u16 | `MP+0E` | Team-is-active mask used by the complete original predicate. |
| `01` | `0A..1A`, nine i16 | `MP+10..20` | Team-designator mapping; absent entries are `FFFF`. |
| `02` | `1C`, u8 | `MP+6C`, i16 | Current phase/state, narrowed on difference and widened on apply. |
| `04` | `1D`, boolean byte | `MP+C08`, byte | Game-finished notification data; application starts the separate game-over owner. |
| `08` | `1E`, u16 | `MP+6E`, word | Current round. |
| `10` | `20`, i16 | `MP+E0`, word | Round timer, encoded with `+1` on the original bitstream. |
| none | `22`, two bytes | none | Padding cleared by initialization. |
| `20` | `24`, i16 | `King+1A0`, i32 | Selected hill, compared at full live signed width before narrowing. |
| `40` | `26`, u16 | `King+1A8`, u16 | Occupant bitmask. |

The two masks at bank `02` and `08` must not be conflated. The small update header calls bank `02` `team_mask`; the larger live globals header calls `MP+0E` `team_mask`. The actual reader uses the first to select nine mapping entries, while the actual recording predicate uses the second for teams `0..7`. Reference producer `157670` assigns designators; `158140` recomputes connected-team bits and accumulates present teams into `MP+0E`. These producer interpretations are readable retail reconstruction evidence, not newly executed or three-image compared producer owners. Preserving the literal 14-word span `00..1B` is supported without inventing stronger field names. [Update layout](../tools/reference/halo2-decompiled/include/game_engine_globals_update.h), [retail producer reconstruction](../tools/reference/halo2-decompiled/src/unknown_157450.cpp), [original shared reader](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-shared_read-000a4e20.asm), [whole predicate](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-recording_team_is_active-00161e60.asm).

## Initialization and difference ABI

Shared initialization takes one stack bank pointer and returns with `ret4`. It clears 36 bytes, then writes nine `FFFF` mapping words at `+0A..+1A`. King initialization uses ECX as the registered engine object and two stack arguments `(ignored, bank)`, `ret8`. It first clears all 40 bytes and then calls that engine's real slot 41 with the bank. Thus hill and occupant mask start at zero; hill `NONE` is not the cold initializer result. [Shared initializer](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-base_initialize-0024e320.asm), [King initializer](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_initialize-002bdb40.asm).

King difference uses ECX engine and stack `(requested_and_changed_flags_pointer, ignored, bank)`, `ret0C`. It starts a separate changed word at zero. If requested bits `1F` are nonzero, it calls actual slot 42 with `(requested & 1F, &changed, bank)`. Requested `20` compares the sign-extended bank hill against the full 32-bit live hill and writes only its low 16 bits when different. Requested `40` compares/copies the occupant word. Finally it **replaces** the caller's flags with the newly changed bits; unsupported requested bits disappear. It does not simply OR into the original request. A live hill outside signed-16 range can consequently remain dirty after truncation. [Complete King difference](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_difference-002bdb80.asm).

Shared difference takes stack `(requested_mask, changed_pointer, bank)`, `ret0C`, and ORs into the supplied changed word. Bit `01` compares all 14 words and copies the entire 28-byte span if any word differs. Bit `02` zero-extends the bank byte, compares it with the raw live word, then stores the live low byte. Bit `04` first normalizes the bank byte to 0/1, compares against the **raw** live byte, then stores the live byte normalized to 0/1. Bits `08/10` compare/copy their words. Out-of-byte phase values or noncanonical live boolean bytes can therefore repeat dirty; no extra sanitization is present in this owner. [Complete shared difference](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-base_difference-0024e360.asm).

## Application and actual service order

King application uses ECX engine and stack `(mask, ignored, bank)`, `ret0C`, with boolean AL. For `mask & 1F` it calls actual slot 43 with `(mask & 1F, bank)` and normalizes the returned AL. It still proceeds with King bits even if that base result is false. For changed bit `20` it sign-extends bank hill, writes `King+1A0` **before** calling the actual hill builder `(King, hill)` at retail `2BDC70`, TU 1.1 `2C3FB0`, or TU 1.5 `2C7A30`. Bit `40` compares/copies `King+1A8`. It returns the earlier base boolean, or true when there were no shared bits. [Complete King apply](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_apply-002bdc00.asm), [authored geometry contract](MP_KING_ENGINE_SOURCE_RESEARCH.md).

The actual shared apply takes stack `(mask, bank)`, `ret8`, and returns AL1. Its ordering is:

1. Bit `01`: copy the 28 bytes to `MP+06..21`, then call actual player-property refresh, then actual retained-recording refresh.
2. Bit `02`: compare signed-extended live `MP+6C` with the unsigned bank byte. If different, supply the new byte in **EAX** to the real phase-application owner while `MP+6C` still contains its old value. After returning, reload the globals pointer and bank byte, then store the widened new phase.
3. Bit `04`: call game-over begin whenever this bit is present, including when bank `+1D` is false. It does not directly copy the bank boolean to `MP+C08`.
4. Bits `08/10`: store the round and timer words.

These calls are part of application, not optional observers. Neither callback supplies a default success path for an unimplemented world service. King hill application also requires the genuine scenario geometry builder, rather than an identifier-only assignment. [Complete shared apply](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-base_apply-0024e490.asm), [complete King apply](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_apply-002bdc00.asm).

Player-property refresh walks the actual occupied player-pool bitmap/high-water range. For each row it constructs the full datum from the row's salt and absolute index, copies `0x90` bytes from player `+D4` to its stack, and calls retail `14BE90` with that copy in EAX and the player datum on the stack. It does not require a current live unit or skip disconnected occupied rows. The called property owner and its sanitization remain the original producer contract. [Whole refresh](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-refresh_player_properties-001523c0.asm), [player input/property research](MP_DAMAGE_PLAYER_INPUT_SOURCE_RESEARCH.md).

Retained-recording refresh returns immediately if recording-enable retail `510CA0` is zero or freeze byte `510CB1` is nonzero. TU 1.1 uses `518490/5184A1`; TU 1.5 `51DFA0/51DFB1`. Otherwise it makes 16 actual team-is-active calls, supplying indices in ECX. For a true result and a zero row byte it writes byte 0 of the corresponding `0x18`-byte recording row to 1; it never clears the byte. The predicate requires a selected engine, options `+184` bit 0, index `0..7`, and the matching `MP+0E` bit. This is not event recorder `1972A0`, and its freeze byte is not the event recorder's different `510CA1` flag. [Whole recording refresh](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-refresh_recording-00196780.asm), [whole team predicate](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-recording_team_is_active-00161e60.asm), [event-recorder distinction](MP_KING_ENGINE_SOURCE_RESEARCH.md).

Received phase 1 stores current clock `+08` into `MP+70`, calls shared retail `157A80` player preparation, then tail-dispatches selected engine slot 3 only if options `+180` equals **4**. This is an options-mode condition, not a comparison against the selected engine index. Phase 3 tail-calls retail `159E20`; other phases return. This received-state owner is distinct from the authority-side transition owner covered in the earlier callback research. [Complete received-phase owner](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-phase_change-0015cba0.asm), [authority-side transition research](MP_KING_NATIVE_CALLBACK_RESEARCH.md).

Game-over begin is service-free. If options `+1128` is already nonzero it returns without writes. Otherwise it writes that flag to 1, converts signed clock `+02` to binary32, multiplies by literal binary32 `7.0` (`0000E040` little-endian), converts through x87 `FISTP` under the actual control word, stores countdown `options+112C`, and clears `options+1129`. It does not itself set `MP+C08`. Its complete source is shared with the existing game-over oracle. [Whole original owner](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-game_over_begin-001389c0.asm), [pinned constants and prior execution tool](../tools/oracle_game_over.py).

### Honest bounded whole-execution contexts

These are source-derived input contracts for a separate oracle, not new execution results in this note:

- For bit `01`, a backed actual pool with high-water `+38 = 0` makes the real property-refresh owner return before reading its bitmap/data. Disabled recording or a nonzero real freeze byte then makes the real recording owner return. Alternatively, enabled/unfrozen recording and actual team-mode false executes all 16 real predicates without changing rows; team-mode true with backed recording rows exercises `MP+0E` activation.
- For bit `02`, a changed supplied phase other than 1 or 3 executes the complete received-phase owner through its own return. Phases 1/3 require their actual player preparation/round services. Equal phases bypass the call. Existing original callback evidence covers different authority-side phase owners and must not be relabelled as execution of this received-state owner.
- For bit `04`, back actual options and clock. Active game-over yields the owner's genuine no-op path; inactive game-over executes its actual conversion/stores. Record observed FPCW and MXCSR for numeric claims.

Empty rosters and disabled recording are declared fixture inputs, not assertions about a populated live match. No hook-supplied callback return is needed for these branches. [Player refresh listing](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-refresh_player_properties-001523c0.asm), [recording listing](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-refresh_recording-00196780.asm), [phase listing](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-phase_change-0015cba0.asm), [game-over listing](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-game_over_begin-001389c0.asm).

## Entity callers and original encoding boundaries

The actual entity-definition initialization/difference/apply dispatchers select an engine through `MP+C14`, call its slot 39, compare that result against their own entity slot 0, and dispatch engine slots 44/45/46 only when those identifiers match. Initialization and difference return true on a matching dispatch; apply propagates slot 46's AL. The leading caller argument is not forwarded to those engine callbacks. They retain distinct entity-definition `this` and registered engine `this` pointers. [Initialize caller](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-initialize_dispatch-000a45e0.asm), [difference caller](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-difference_dispatch-000a4650.asm), [apply caller](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-apply_dispatch-000a4730.asm).

The separate creation/application caller compares the creation record's signed word `+04` with the selected engine's current identifier, adopts the full record identifier at `+00` into `MP+24`, then invokes slot 46 only when its supplied update mask is nonzero. It returns true for the matching creation even if that apply result is false. A previously retained non-NONE identifier is cleared before adoption. This is original entity identity ownership; it does not assign an invented native UDP field or prove native entity registration. [Whole creation caller](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-creation_apply-000a4880.asm).

The real King entity advertises bank size `28` hexadecimal and supported mask `7F`. The actual writer first delegates shared bits `1F` with two additional reserved bits, then uses a flags writer for existence bits 5 and 6. A present hill is sign-extended, incremented, and written in four bits; a present occupant mask is written in 16 bits. The hill writer's checked-range diagnostic does not substitute a successful value. The reader subtracts 1 from the four-bit hill value and stores signed16, reads the occupant word, replaces the output flags with the observed bits, and returns `shared_valid && observed_mask != 0`. It still reads/stores King data after a false shared-valid result. [Whole King writer](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_wire_write-0009a3a0.asm), [whole King reader](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_wire_read-0009a530.asm), [readable entity definitions](../tools/reference/halo2-decompiled/src/game_engine_entity_definitions.cpp).

Shared encoding has five existence bits. When present, the five initial words use widths `8,9,8,8,8`; the nine designators selected by bank `+02` use four bits each. The reader checks subset relations `bank04 ⊆ bank00`, `bank08 ⊆ bank04`, `bank06 ⊆ bank08`, and selected designators `0..7`; omitted designators become `FFFF`. Phase uses two bits, finished one, round five, and round timer 16 after `+1` adjustment. These are source codec contracts and validation, not a new transport format. The static King size wrapper calls the shared size owner, adds two, then applies **minimum** clamps `0B` for bit `20` and `17` for bit `40`; it must not be reinterpreted as the full encoded packet size. [Shared writer](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-shared_write-000a4ab0.asm), [shared reader](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-shared_read-000a4e20.asm), [literal size wrapper](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/retail-king_wire_size-0009a2c0.asm).

Writer capacity/reservation, flags-writer begin/end, checked bitstream operations, caller scheduling, entity dirty publication, packet delivery, and received-update orchestration remain actual integration dependencies. None is supplied a successful return here. Existing original King update publishes occupant changes with `40` and moving-hill changes with `20`; that producer does not by itself prove a native entity/channel exists. Original retail dirty publisher `B58C0` is mode/context gated and updates a registered entity's dirty mask; its reconstruction is not a transport compatibility proof. [King update/publication research](MP_KING_ENGINE_SOURCE_RESEARCH.md), [retail dirty publisher reconstruction](../analysis/retail/functions/000b58c0.c).

## Reproduction and retained limits

From the repository root, choose a fresh output directory:

```sh
.tools/python/bin/python tools/inspect_king_replication_contracts.py \
  --output analysis/multiplayer/game-variants/king-replication-source-20261012/reproduction
```

The frozen [complete report](../analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json) SHA256 is `44c4599ff00ed27dc3124555d98ca3e6c9f4a10565c9dfaf03fa9eb2fa1d6e2d`. It pins all three original images, the tool and five harness dependencies, both actual tables, every saved listing, comparisons and the explicit recording-bound normalization. Reproduction metadata is saved separately beside the frozen outputs. No native module, root-owned oracle, or test expectation is imported by this tool.

A fresh independent invocation produced the same report and all 60 listings byte-for-byte. The [reproduction record](../analysis/multiplayer/game-variants/king-replication-source-20261012/reproduction.json) also verifies the nine getter bodies above and local document links. The initial standalone verification helper needed its reference-module import order corrected; source generation and the frozen outputs were unaffected.

Static correspondence plus real registration does not establish complete replication callback execution, populated player/property refresh, source network services, local HUD/render effects, a normal-menu match, or all39-variant completion. Original hill geometry, gameplay consumers, presentation and full ordinary/native transport flows remain separate requirements. [Existing goal scope and source gates](MP_KING_ENGINE_SOURCE_RESEARCH.md), [existing callback boundaries](MP_KING_NATIVE_CALLBACK_RESEARCH.md).
