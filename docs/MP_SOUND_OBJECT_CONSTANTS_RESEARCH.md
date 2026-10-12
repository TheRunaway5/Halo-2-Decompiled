# Sound world-owner immutable operands

`OriginalData::sound_object_request_constants` now imports the verified
immutable inputs to the complete source world/marker/velocity sound owner.
Both authenticated updates match the independent original bytes in normal and
GCC AddressSanitizer/UndefinedBehaviorSanitizer production-library tests. The
unverified base image leaves this optional input absent.

| Field | 1.1 address | 1.5 address | Original bytes |
| --- | --- | --- | --- |
| `one_bits` | `465030` | `46A0D0` | `0000803f` |
| `minus_one_bits` | `46503C` | `46A0DC` | `000080bf` |
| `z_scale_bits` | `465298` | `46A338` | `00c0ff43` |
| `xy_scale_bits` | `46529C` | `46A33C` | `00e07f44` |
| `zero_vector` | `46FC44 → 447144` | `474D04 → 44ABEC` | Twelve zero bytes, dereferenced |

The source authority is the complete-body original/u5 correspondence and
`analysis/multiplayer/integration/sound-object-request-native/u5/providers.json`
at SHA-256 `ed24990c4f6470e736fedb71c71e86c4402b8e6977931a53496aa658f53288c9`.
An exact copy and literal operand fixture are retained in this checkpoint,
independently of the world-owner module's later completion. Expected fixture
SHA-256 is `54aa0ad6dbd512163b2e66d40ea0af5c5b1b4bade8a63b793b60d5d9a5822d19`.
The importer reads the user's executable; these vector/scalar bytes are not
embedded as replacement game data.

CTest `original_sound_object_constants` passes in normal 0.02 seconds and
sanitizer 0.11 seconds. It compares all eight imported scalar words and 24
dereferenced vector bytes across the two images, and verifies base absence.

This supplies immutable operands only. Object/physics pools, source headers and
bodies, current node matrices, current velocity, PVS, listener relationships,
active BSP and the actual floating-point context remain required live owners.
The zero vector is an instruction-selected immutable input to its particular
source branch; it is not a default for missing live velocity or pose. No live
sound consumer or new launcher is installed.

Copied binaries/logs, source snapshots, original inputs and preserved earlier
checkpoints are frozen in
`analysis/multiplayer/integration/sound-object-constants-20261008/frozen/manifest.json`.
