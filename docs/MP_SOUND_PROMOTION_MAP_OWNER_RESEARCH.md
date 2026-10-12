# Retained mutable map sound state

`SourceMutableTagStore` supplies one persistent mutable copy of each explicitly
selected resident tag payload. It copies the actual owned Cache bytes and
preserves source addresses and every serialized field. It never resets promotion
counters, assigns clock state, activates a map/BSP, creates a raw tag table or
supplies availability. Its providers share the same mutable backing across calls
and retain the original Cache after both facade and caller Cache references end.
The owning source update thread must serialize access.

The completed promotion integration requires this retained ownership: original
`128610` mutates `ugh!` row+10/+14/+18 and the pointed counter arrays. Recreating
read-only metadata on every submit would discard those writes. The new store
exposes only bounded memory and actual datum/table-slot addresses, avoiding a
second stale immutable definition view of the mutable state. Nonempty accesses
must fit one uniquely owned payload. An indexed interval search retains explicit
ambiguity detection and rejects overflow, out-of-group and out-of-range requests.
`sbsp` and `ltmp` require other mappings and are explicitly rejected.

The independent original expectations are the immutable complete promotion/clock
fixtures, SHA `36f685b59311b87ec0a7e305513c083ff512456740b751a77ab4c09147479f30`,
from `sound-promotion-native/final-manifest.json`, SHA
`eb053b6680b4917f9d03eec27b078c725932a9b4e4cb369e13ebbc2a7c11940e`.
The production owner and immutable operands were separately frozen in
`sound-promotion-production-20261009/frozen/manifest.json`, SHA
`a7ca3a07efe83429451090080344d26e247eec442c35516ed7192a530bd39df1`.
Those files and original input pins remain unchanged.

The new composition test reads each actual owned map through production Cache
and imports its real `ugh!` and `snd!` payloads. Their initial bytes must match
the original experiment before any Source call. The genuine source-created
system arena/clock origin and literal raw tag table remain explicit original
fixture inputs; this test does not reconstruct them from semantic defaults.
Every source call then executes the production promotion or clock owner against
the persistent Cache-owned mutable payloads. Expected bytes and ordered writes
come exclusively from the original executable experiment.

Fresh optimized and GCC ASan/UBSan comparisons pass:

- 46 image/map banks:23 maps, each with update1.1 and1.5.
- 74,318 complete source calls and249,740 ordered writes.
- 19,211,104 complete final region bytes.
- 9,173,672 bytes imported directly from actual map payloads.
- Genuine persisted counters/cooldowns, provider lifetime and bounds checks.
- Original Cache bytes remain unchanged after all mutable Source writes.

Initial compilation identified missing complete-type includes in the new module
and then the additional original-Cache verification; both failed logs remain in
validation. The includes were corrected before final comparisons. Measured
linear interval lookup was replaced with an indexed search; the complete original
outputs remain identical. Final normal/sanitizer logs are under
`sound-promotion-map-owner-20261009/validation/*-indexed-final/`.

This closes actual mutable map backing for the promotion CPU owner. It does not
claim Source admission, current BSP/local listener production, complete sound
creation/cache loading, a device/voice, audible reload playback or a changed
launcher. Those dependencies remain explicit and unfinished.
