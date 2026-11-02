# Native TU1.5 post-integration listener cohorts

The native semantic `322820` and `322950` owners are now wired into world
integration. They preserve reverse notification, fresh list reads, signed
count/index behavior, stable null compaction and source store order. World
notification reads the actual `3161F0` world+C0 listener after the cohort
callbacks and compaction. No guest registers, instruction lifting or supplied
callback outcomes implement these three owners.

The original authority is English TU1.5 SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The bodies contain55/49/2 sites respectively; actual08FDB0 RET8 is the genuine
no-op listener. Other listener services remain explicit genuine dependencies.

`build/test-results/native-world-step-listeners-20261101/report.json` passes
210 physical original cases with GCC and Clang:114 complete calls,7 genuine
custom-owner entry stops and89 read/store/observer prefixes. It compares3,675
ordered reads,332 stores, all retained bytes and FP state through12,890 unchanged
original instructions. Signed empty counts, mixed/null lists, final listeners,
preserved x87 depths0/1/3/8, FP modes and three private mutations are covered.
Custom callback return composition and unmasked platform exceptions remain open.

`build/test-results/native-world-step-listeners-binding-20261101/report.json`
passes110 original world cases,96 native force compositions, the binding model
and its negative control. The model executes distinct custom listener services,
changes the world's final listener during a callback, verifies that it is read
late, and requires stable compaction before that final notification. It confirms
neither322820 nor322950 executes as a residual owner. Remaining callbacks retain
the real receiver, arguments, stack discipline and preserved x87 values.

The live build records separate world/island notification counts. The required
`HALO2_REQUIRE_NATIVE_WORLD_STEP_LISTENERS` gate admits only positive activity
from both owners. A fresh actual-camera multiplayer cohort and strict original
whole-frame comparison are pending. These component/model results do not
establish full native physics or production delivery.

Both physical compiler build trees and their outer scratch owner are absent;
58,269,696 allocated temporary bytes were measured before outer cleanup. The
world binding owner was also removed; its freed byte count was not measured.
Only small reports, commands and source snapshots remain. Source pins for the
new live cohort are in
`analysis/physics/native-world-step-listeners-live-binding-20261101/source-pins.json`.
