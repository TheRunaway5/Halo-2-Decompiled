# OriginalData import correspondence in the owned 1.5 executable

The three CPU data tables required by `OriginalData::load_xbe` occur **once each, with complete byte equality**, in the authenticated English 1.5 executable. The loader can select its actual table addresses using whole-file FNV-1a `08AEEC2701BCA325`. This fingerprint is an import selector; the separate [executable authentication](MP_TU_1_5_EXECUTABLE_IMPORT.md) establishes source authenticity.

## Source and frozen evidence

The 1.1 source is `assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03`, SHA-256 `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, whole-file FNV-1a `598BF549DD8FB828`. The owned 1.5 source is `/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe`, SHA-256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`, whole-file FNV-1a `08AEEC2701BCA325`.

[`research_mp_original_data_u5.py`](../tools/research_mp_original_data_u5.py) finds complete table bytes in the original new file and verifies the complete normalized instructions and internal branch topology of representative consumers. Original executable instructions, raw table bytes and section mappings are the authority. The fresh unseeded Ghidra inventory supplies candidate function starts only; attached annotation names and comments are unused. External helper equivalence is not inferred from masked call targets.

The independent [`audit`](../tools/research_mp_original_data_u5_audit.py) parses raw section headers independently, verifies all 434 original instruction listing rows and 11 paired static image-memory operand values, and checks actual instruction operands for the 19 balance constants. The static operand audit includes LEA address operands and a zero-filled runtime global; it does not claim executed memory reads or live global equality. A fresh-process repeat reproduces the full report, audit, six instruction listings, three binary tables and balance manifest byte-identically. Outputs are ignored under `analysis/multiplayer/integration/inventory-source/original-data-u5/`; `reproduction/` retains the second run.

## Exact importer locations and representative consumers

| Table | Extent | 1.1 VA | 1.5 VA / raw file offset | Section | Table FNV-1a |
|---|---:|---|---|---|---|
| Weapon directions | 1,026 × 12 bytes | `447E40` | `44B8E8` / `4472E8` | `.rdata` | `6778693F5DB53615` |
| Fade curve 5 | 1,024 bytes | `4745E8` | `4796A8` / `475FC8` | `.data` | `E2570A57F6E2DC5E` |
| Collision projection | 6 × 3 signed 16-bit words | `4471E4` | `44AC8C` / `44668C` | `.rdata` | `ED2DF01A26BD8B1D` |

The corresponding table SHA-256 values are `9581ebb544c806b2c88cdc2a094df6a0ac07978c9bb07dcb9f68fe2962fd2feb`, `ed6ff8c88811bc9aa7bb610401d9d6005a35463d8be0bca9c68e117874c68ec9`, and `afaf7ee3fe87b6989541d81f79a6a737b1e7cadede1ab3ed6737c6a29b43c5db`, respectively. All extents are fully raw-backed in their single source section.

- Directions: complete 20-instruction consumer `147A50` → `148DE0`; actual `148E02` bytes `8D148DE8B84400` form `ECX*4 + 44B8E8` after the original RNG/index arithmetic. The subsequent original stores copy all three vector words. [Original listing](../analysis/multiplayer/integration/inventory-source/original-data-u5/instructions/weapon_directions-u5.asm).
- Fade: complete 60-instruction consumer `17E480` → `17F8A0`. Original `17F8CD` sign-extends AX, `17F8D6` shifts by ten, and `17F8DD` adds actual family base `4782A8` (1.1 `4731E8`). Hence curve 5 is `4782A8 + 5*400 = 4796A8`; a direct curve-5 base reference is not required. This proves the table selector arithmetic without executing the caller or external rounding helper. [Original listing](../analysis/multiplayer/integration/inventory-source/original-data-u5/instructions/fade_curve5-u5.asm).
- Projection: complete 137-instruction consumer `14BAC0` → `14CEA0`. Original `14CFAB` bytes `0FBF888CAC4400` read the signed word at `EAX+44AC8C`; `14CFB2` reads the adjacent word at `EAX+44AC8E`. The complete paired original listings retain their actual external call targets and runtime-global references. [Original listing](../analysis/multiplayer/integration/inventory-source/original-data-u5/instructions/collision_projection-u5.asm).

## Source balance word import

The separately frozen [combat postprocessor](MP_COMBAT_TAG_POSTPROCESS_UPDATES_RESEARCH.md) executes complete original `164750` / `165B30` against raw Lockout cache bytes and records their identical ordered outputs. Its `paired_image_memory_operands` supplies **19 unique four-byte constants in first-encounter order** after excluding only the runtime pointer globals `4E7920/4E791C/4EB114` → `4ECC20/4ECC1C/4F0414`. The base disc receives no optional patch. The importer reads each word from the selected, whole-file fingerprint-checked executable and retains its raw bits; no tuning values are generated or hardcoded in gameplay.

| Index | 1.1 VA | 1.5 VA | Original word bits |
|---:|---|---|---|
| 0 | `4652D4` | `46A374` | `3AE4C388` |
| 1 | `4652D0` | `46A370` | `42B40000` |
| 2 | `451190` | `455A90` | `42F00000` |
| 3 | `4652CC` | `46A36C` | `425C0000` |
| 4 | `4652C8` | `46A368` | `42960000` |
| 5 | `46502C` | `46A0CC` | `3F000000` |
| 6 | `4652C4` | `46A364` | `3FE00000` |
| 7 | `465104` | `46A1A4` | `42480000` |
| 8 | `446548` | `449FD4` | `3FC00000` |
| 9 | `46509C` | `46A13C` | `3F400000` |
| 10 | `4652C0` | `46A360` | `43200000` |
| 11 | `4652BC` | `46A35C` | `3FA66666` |
| 12 | `4652B8` | `46A358` | `43480000` |
| 13 | `4650F4` | `46A194` | `3F666666` |
| 14 | `4652B4` | `46A354` | `40B00000` |
| 15 | `46518C` | `46A22C` | `3F333333` |
| 16 | `4652B0` | `46A350` | `40940000` |
| 17 | `46516C` | `46A20C` | `41F00000` |
| 18 | `451194` | `455A94` | `42700000` |

The 76 concatenated little-endian bytes have SHA-256 `822331d6ca293d3e78bcb3ed60634fbdaec1b4a61b85d05def38e15240767cae` and FNV-1a `488CD915E8C2E81F`. The exact constant addresses, first original read sites, bits and source pins are retained in `balance-constants.json`; every constant is bit-identical between the owned updates. This closes a raw-import omission, without claiming a new 1.5-only combat change.

## Reproduction and limits

Run `.tools/python/bin/python tools/research_mp_original_data_u5.py --output <fresh-directory>/report.json`, then `.tools/python/bin/python tools/research_mp_original_data_u5_audit.py --output-dir <fresh-directory>`. Compare all artifacts against the frozen directory.

Tool SHA-256: `009b2d7836b63036ea80bfc7cf67b6edd6c7f4b8182fac54bd8e7ee1fc0978e7`; auditor `be36d09a92419128520a975ee5b0bf023f4d3fb52f740c8cfae17c3d32191d03`; report `e400e5d4605a86bcdf27cb057437b4193a0aeebb7730a5cadbf189c4c094501f`; balance manifest `85a87efcbdf9a6f27455f3851dd590bd2e245d34a8bf69fa11aa7124773a1f3a`. The audit and final evidence manifest provide the remaining pins.

This table evidence is static original-source correspondence. It does not newly execute direction/fade/projection callers, prove updated RNG streams, produce live animation/world/contact state, or establish complete title-update simulation or hardware parity. The 19-word source import is separate from the combat postprocessor and live content integration. No shader, rendering, matrix, executable, cache file or earlier frozen evidence is changed by this research.
