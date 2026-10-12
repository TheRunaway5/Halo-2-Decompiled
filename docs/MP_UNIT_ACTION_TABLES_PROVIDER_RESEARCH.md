# Retained original unit action tables

`OriginalData::unit_action_tables` now owns the selected executable's complete
60-entry action bank and the three verified biped, weapon and projectile
allocation records. Native action callbacks can retain `OriginalData` and read
the literal 16-byte descriptors through a bounded span. Original callback
addresses are identities only. No host code pointer is constructed from them.

The importer follows each original bank pointer to its record, including the
signed base extent at object-type record+8. It embeds source addresses and
verified type indices, without embedding the authored record bytes or replacing
an unknown type's allocation size. The base executable has no verified table
mapping and leaves the optional provider absent.

| Bank | Update 1.1 | Authenticated update 1.5 |
| --- | --- | --- |
| Action pointers | `0046EC68` | `00473D28` |
| Object-type pointers | `0046FAD0` | `00474B90` |

The authority is the agent's independently measured and frozen packet
`analysis/multiplayer/integration/spawn-source/unit-action-dispatch/original-data-owned.json`,
SHA-256 `657123ffd0c414bc1080a4e5bf028865eda5f2f533054fbcc72f0c5bf9e30a75`.
The exporter authenticates that packet and both owned executables before writing
expected records. Independent exports have identical fixture SHA-256
`2a4b78444ab6a1339ef697a750385d5abd9473fd0de4c0e5293a993b3145fa4a`
and report SHA-256
`9ef75310297cfbbb81a8c7117b92a0dcac01bafac380ab9304f1b06f42ce75f9`.

Production-library CTest `original_unit_action_tables` passes in normal 0.02
seconds and GCC AddressSanitizer/UndefinedBehaviorSanitizer 0.12 seconds. It
compares all 120 full action descriptors and six full allocation records
(2,016 bytes), source bank/record identities and signed extents. It checks
provider lifetime after facade release, release after the last callback,
unverified base/type absence, and out-of-bank rejection.

This is an immutable source-data provider. Actual object/player constructors,
local-user assignment, retained unit components, grenade action callbacks,
projectile creation, attachment/release, world insertion and fuse/damage owners
still have to compose in the live match. These checks do not establish a thrown
grenade or a corrected launcher. The canonical launcher is unchanged.

Sources, binaries, logs, independent outputs and earlier evidence are preserved
in `analysis/multiplayer/integration/unit-action-tables-provider-20261008/frozen/manifest.json`.
The CMake snapshot observes the shared file; this change owns only the table
test registration and does not certify another chat's rendering changes.
