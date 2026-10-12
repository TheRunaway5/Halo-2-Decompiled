# OriginalData netgame constant provider

`OriginalData::multiplayer_netgame_postprocess_constants` imports raw original words for the separately recovered netgame postprocessor. It is an `optional<MultiplayerNetgamePostprocessConstants>` from `halo2/multiplayer_netgame_postprocess.hpp`: absent for Base; present for MapPack11 and TitleUpdate15; its nested Relic optional is absent in 1.1 and present in 1.5. The full executable fingerprint selects the addresses before import. No tuning values, Base correction or 1.1 Relic targets are manufactured.

## Original authority and separate repeat

The complete unchanged source bodies are `11DF00` (1.1) and `11F060` (1.5), independently executed on Beaver Creek, Lockout and Relic in the [original netgame freeze](MP_NETGAME_POSTPROCESS_1_1_TO_1_5_RESEARCH.md). Its complete report SHA-256 is `d3a369d8e75267d1505e282f82836dbb4c3ceb48b4dc6fc3cc2314f9e05ff40c`. Source SHA-256 and complete original instruction listings remain authoritative; this provider does not repeat or alter that six-case execution or its compatibility service.

[`research_mp_netgame_original_data.py`](../tools/research_mp_netgame_original_data.py) decodes every original listing row and independently verifies each instruction byte against its pinned XBE. It records all absolute memory operands, excludes only the actual scenario pointer global `4E7920 / 4ECC20`, and extracts **seven / fourteen unique original four-byte MOVSS operands**. Every constant records its raw file offset, original word bytes, first source instruction, repeated read sites and correlation to executed addresses in the frozen original cases. Raw packet SHA-256 is `19f95f4cc962bb3f7dd9589caee6dc9b57f7ff81d18c07fa7456945d7f8afa7a` for 1.1 and `3640518afbdd47aa31d359b027dbb252e153fd0cfcb6180f3a5323c108e0d539` for 1.5; their source encounter-order FNV-1a values are `D15C8D0C31AFAE08` and `D08ABE82BA07A4BA`.

The separate [semantic mapping tool](../tools/research_mp_netgame_original_data_contract.py) maps only those actual source operands to the agreed provider fields, retains both shared Z reads and does not use native expected outputs. Both tools run in fresh processes; the whole report, both original word packets and semantic mapping repeat byte-identically under ignored `analysis/multiplayer/integration/inventory-source/netgame-original-data/reproduction/`. Previous source freezes and previous native validation outputs stay unchanged.

## Exact semantic field mapping

| Provider field | 1.1 original VA | 1.5 original VA | Original word bits |
|---|---|---|---|
| Beaver Creek target 0 / X | `465518` | `46A5C8` | `4193271E` |
| target 0 / Y | `465514` | `46A5CC` | `BD4A4350` |
| target 0 / Z | `446DF8` | `44A8A0` | `3E19999A` |
| Beaver Creek target 1 / X | `465510` | `46A5C0` | `C1838AA6` |
| target 1 / Y | `46550C` | `46A5C4` | `BD8416CB` |
| target 1 / Z | `446DF8` | `44A8A0` | `3E19999A` |
| distance squared threshold | `46504C` | `46A0EC` | `38D1B717` |
| Beaver Creek X delta | `46509C` | `46A13C` | `3F400000` |
| Relic target 0 / X | absent | `46A5BC` | `425BC1BE` |
| target 0 / Y | absent | `46A5B8` | `C33ADAA0` |
| target 0 / Z | absent | `46A5B4` | `BFABFB62` |
| Relic target 1 / X | absent | `46A5B0` | `4280C8CE` |
| target 1 / Y | absent | `46A4FC` | `C3070000` |
| target 1 / Z | absent | `46A5AC` | `C00A38B4` |
| Relic Z delta | absent | `46A178` | `3DCCCCCD` |

The targets are source comparison inputs, not coordinate override outputs. Beaver Creek preserves the authored Y/Z and adds/subtracts X delta after its Type/distance checks. The two source versions read X/Y in different orders, so the provider exposes semantic XYZ while the separate native leaf takes an explicit version enum to preserve arithmetic order. Relic uses the common threshold and source-produced zero additions for X/Y; those zeros are instruction outputs, not extra XBE constants. This provider contains no arithmetic, protection, metadata mutation, map-ID selection or enum adaptation.

Primary instruction citations: [complete 1.1 body](../analysis/multiplayer/title-update-1.5/netgame-postprocess/measured/beavercreek-disc_1_1/0011df00.asm), [complete 1.5 body](../analysis/multiplayer/title-update-1.5/netgame-postprocess/measured/beavercreek-owned_1_5/0011f060.asm). For example, `11DF43/11F0A8` load the shared Z; `11DF4B/11F0B0` load the threshold; `11DF53/11F0B8` load X delta; and `11F1C4..11F21D` load the exact Relic target/delta operands.

## Provider validation and limits

`tests/original_data_tests.cpp` keeps its legacy Base/1.1 invocation and optional fourth argc for 1.5. Tests verify the complete original word packets and semantic mapping, both shared Z fields, absent Base patch, absent 1.1 Relic data, present 1.5 Relic data, existing tables/balance words, and rejection of changed 1.5 Beaver Creek or Relic constant bytes. Whole-file identity remains the production guard; frozen source packet hashes are test expectations only.

Standalone GCC-O2 tests pass for two and three owned executables; Clang ASan/UBSan tests pass for all three. New binaries and `validation.json` are stored separately under ignored `analysis/multiplayer/integration/original-data-netgame-native/`. Only `include/halo2/original_data.hpp`, `src/original_data.cpp` and `tests/original_data_tests.cpp` are changed by the provider task. The separate native leaf/header belongs to the spawn researcher; Cache/CMake/runtime integration belongs to the parent. `Cache::original_data()` is untouched by this task.

The operand report SHA-256 is `2d3035514210f9a22158ced7dd7e7df4ef295a234e74ba734a3edcbd439af8d5`; provider contract is `36bb2fefff6099a370276ed964c14a6500858eba8bfd994eb0535355db1c2f8d`. The final manifest pins tools, document, reports and source inputs. Reproduce the operand tool with `--output <fresh-directory>/report.json`, then the mapping tool with `--output-dir <fresh-directory>`.

This extension proves source word import and its semantic contract. It does not newly execute the original netgame routines, integrate teleport traversal or prove full updated map activation, world simulation or Xbox hardware parity. No earlier frozen artifact is modified.
