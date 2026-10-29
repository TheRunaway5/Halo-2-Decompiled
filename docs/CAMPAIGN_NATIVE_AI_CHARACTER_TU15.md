# Shared native campaign character lookups

`src/campaign/ai/native_campaign_ai_character_tu15.cpp` implements five complete
TU1.5 semantic lookup owners. It uses retained character/tag/action/order data;
it contains no register machine, private guest stack, original-code execution,
captured return values or guessed navigation results.

| Original | Typed operation | Sites |
|---|---|---:|
| `1ECD00` | First positive inherited navigation block | 20 |
| `1ED700` | First matching inherited vehicle-specific navigation record | 47 |
| `27C6C0` | Authored order's scenario character definition | 21 |
| `27C710` | Actor action override or squad order character | 50 |
| `1ED100` | Inherited movement block and authored variant selection | 56 |

The public header is `include/halo2/native_campaign_ai_character_tu15.hpp`;
`cmake/CampaignNativeAiCharacter.cmake` supplies the library source and focused
test. `native_campaign_ai_character_movement_tu15(memory,actor)` returns the
actual movement-block address needed by `1BD310`. These are internal typed
dependencies. Numeric caller ABI/dispatch is not claimed here.

All lookups preserve the original lazy read sequence, signed counts/indices,
raw inherited data identities and missing-storage failures. No new tag bank or
literal is initialized. `27C710` reads the full DWORD at actor+30 and compares
FFFFFFFF before using its low16; a non-NONE squad with lowFFFF remains a real
squad lookup. Active action metadata and actual scenario/tag data supply the
character selection. Null action definitions continue through the original
squad fallback. A retained negative order index other than FFFF is not clamped.

`1ED100` caches its original tag-instance base but calls `27C710` afresh for
each ancestor. A nonzero signed override byte at character+2C is sign-extended
then decremented; otherwise the WORD at +24 supplies the variant. The final
signed16 index has only the original upper-bound check: negative indices can
return an address before the block. The native implementation does not sanitize
them into index zero. The inherited positive count is reread before selecting
the record. Vehicle matching preserves full32-bit character keys and the
original B4 stride with signed16 iteration.

The offline oracle is `tools/oracle_native_campaign_ai_character_tu15.py`.
It executes unchanged physical i386 original routines from the authenticated
English TU1.5 image, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
All dependency routines run for real. No instruction or result is supplied;
declared inputs are installed only before each entry. Fault prefixes stop before
actual persistent reads and are never resumed. Private stack accesses and dead
volatile register outputs are not promoted into a native semantic contract.

The required compressed fixture is
`tests/fixtures/native_campaign_ai_character_tu15.json.gz`. Its 423 cases include
258 whole calls and 165 read-fault prefixes, cover all 194 original sites and
27,917 unchanged original instructions, and were reproduced byte-identically
by a second independent original execution. Cases cover multi-level inheritance,
negative/zero/positive counts, missing blocks, first/later/no vehicle match,
action/squad fallbacks, order bounds, full datums, negative variants, signed
override bytes and all retained FP inputs. All lookups are read-only; returned
block identities and ordered reads are compared exactly, with original and host
FP preservation checked.

GCC normal and ASan/UBSan each passed 1/1 CTest. Clang passed 6/6, including
five actual mutations rejected by the original fixture: wrong vehicle stride,
low16 squad sentinel, clamped negative variant, unsigned override byte and
stopped inheritance. Source hashes, fixture size/hash, commands and verified
scratch cleanup are in
`analysis/campaign-native-ai-character-20261028/proof.json`. Only the required
bounded compressed fixture and small receipts remain; executable/reference
ELFs, compiler products and raw banks were removed.

This closes neither `2013B0` cached-position preparation nor the complete
`1C9770` selector/path query, `200D30` path update, numeric dispatch or campaign
runtime acceptance. Existing count/fraction and point-geometry sources/proofs
were preserved.
