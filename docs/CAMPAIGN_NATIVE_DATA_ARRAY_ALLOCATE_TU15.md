# Native TU1.5 data-array allocation

Semantic native `16E6A0` selects a free actual signed-indexed slot, publishes occupancy/count/cursors, clears the real record, and issues its original salt. All accesses use caller-owned memory; no original executable is loaded by production. The implementation retains fresh header reads after potentially aliasing stores, exact DWORD/byte clear order, wrapped16-bit salt increment and theFFFF→8000 rule. Allocation refusal returnsFFFFFFFF.

The typed API is `native_tu15_data_array_allocate(memory,pool)`. Its ordinary DF-clear caller profile matches the original ABI; there is no numeric-register transport claim. `NativeTu15DataArrayAllocate.cmake` adds this source once to the existing shared pool-helper library, alongside deletion.

[Independent proof](../analysis/campaign-native-data-array-allocate-20261028/proof.json) covers238 unchanged physical original cases:162 complete calls and76 read/store prefixes, all78 sites. Signed indices, capacity/refusal, word boundaries, zero/odd clear strides, header/bitmap/record aliases and salt boundaries are included. GCC andClang pass, real ASan/UBSan instruments the shared helper/test, and four compiled occupancy/salt/cached-clear/tail mutations are rejected. The independent original repeat is byte-exact.

The observer corrects Capstone’s missing read metadata for authenticated originalTEST852C8B and suppresses nonexistentREP operands when realECX iszero. These are offline observation fixes; original code/results are never patched or supplied.

The217,326-byte compressed fixture is required independent reference data. Scratch binaries, raw memory and builds were removed; compact reports, commands and hashes remain. Full query initialization, resource/action ownership, all mission behavior and physicalXbox acceptance remain open.
