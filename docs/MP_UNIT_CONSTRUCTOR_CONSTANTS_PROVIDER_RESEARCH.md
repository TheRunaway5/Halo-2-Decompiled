# Original constructor-tail operands

`OriginalData::unit_constructor_tail_constants` now supplies the literal
activity pairs and normal/special blend operands needed by the complete native
constructor-tail owners. One/zero words reuse the already verified selected
image's animation operands; they are not inferred defaults. Both authenticated
updates import the actual bytes, while the unverified base leaves this optional
input absent.

| Operand | Update 1.1 | Authenticated update 1.5 |
| --- | --- | --- |
| Four activity pairs (32 bytes) | `0046FB24` | `00474BE4` |
| Normal blend word | `0044C6B4` | `00450170` |
| Special blend word | `00465140` | `0046A1E0` |
| One/zero words | `00465030` / `00465048` | `0046A0D0` / `0046A0E8` |

The independent complete-body correspondence packet is
`analysis/multiplayer/integration/spawn-source/unit-constructor-tail/original-data-owned.json`,
SHA-256 `e25b3c968c9fe7bf758ddd87cc58e0edf91227e219383025938c537de0bec2b4`.
The exporter authenticates both executables and compares their bytes to that
packet. Two independent exports produce identical fixture SHA-256
`dc92863253d823e8dfd7bd29912bfe4c93de9711fd58ebd0f50c80901b62fd6e`
and report SHA-256
`61db22e6e399f9e431da77bcf9ca126f239f6e279c330291259bcd6e185db8e9`.

Production CTest `original_unit_constructor_constants` passes normal 0.02
seconds and GCC AddressSanitizer/UndefinedBehaviorSanitizer 0.10 seconds. It
compares eight scalar words and eight activity pairs, all 96 bytes across the
two images, and checks unverified base absence.

These are immutable operands only. Runtime player-transition flags, actual
object/player/component storage, graph readiness, subchannel and action-cache
owners remain required. No constructor success, live grenade or new launcher
is asserted. Evidence is preserved at
`analysis/multiplayer/integration/unit-constructor-constants-provider-20261008/frozen/manifest.json`.
The shared archive copies are retained build observations for the next provider
composition, without claiming that unrelated rendering edits were reviewed.
