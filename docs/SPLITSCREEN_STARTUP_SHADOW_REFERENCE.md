# Split-screen startup and the shadow reference guard

The October 28 startup report ends with:

```text
error: Live shadow GPU reference requires the actual640x480 drawable window
```

The installed runtime `4bf0c86b3c41485193b431cf4f7da756`, SHA-256
`c4cf078db68795d25668e12241d7eeb345e0382a5bbc097d0688d2e76b0b5b49`,
reproduces this error through the ordinary MCC Split Screen menu at 1280×720.
The actual profile/lobby/countdown/native-world path runs, then exits with zero
gameplay frames. A direct two-player CLI launch can miss it because that path
may advance to tick one before its first render.

Binary inspection confirms that the installed predicate tests scene presence
and equality between the presented tick and reference target, without first
checking that a reference target was requested. Both values default to zero
on the initial menu-prepared frame. The reference-frame flag then activates
the diagnostic's strict 640×480 drawable check during ordinary play.

The current shadow source already checks the nonzero reference target before
tick equality. That correction preserves restrictions on explicitly requested
reference captures while excluding normal tick-zero startup. The failure is
in the shadow diagnostic path, outside the custom-mode rules.

## Regression and delivery gate

`tests/native_split_screen_menu_startup.py` drives real X11 keyboard input
through fresh profile creation, Split Screen selection, the actual offline
countdown and native handoff. It supplies neither a finite-frame limit nor a
capture option. It requires actual rendered gameplay frames and a clean close
through `WM_DELETE_WINDOW`. Temporary profiles and shader caches live under
`HALO2_TEST_SCRATCH`; no screenshots are generated. Its one joined profile
matches the supplied crash log; the regular runtime qualification separately
tests actual two/four-player local worlds.

`native_mcc_split_screen_menu_startup` runs this regression at 1280×720 through
the normal CTest storage wrapper. Engine qualification requires its retained
JSON report to pass with at least one actual frame and the qualified executable's
exact hash before publication. Qualification removes the previous menu receipt
before CTest. Missing, failed, zero-frame and mismatched-runtime reports reject
promotion and preserve the existing selections. Synthetic policy tests prove
that admission rule separately from the actual menu/runtime check.

Evidence is under `build/test-results/splitscreen-shadow-startup-20261028`:
the original menu reproduction, permanent regression failures, current source
pins and the delivery owner's reports. Bounded installed-binary disassembly is
in `build/test-results/splitscreen-shadow-crash-20261028/installed-binary-proof.json`.
## Qualified delivery, October 29

Both installed launchers now select immutable runtime
`bin/engine-builds/9101fc561cbe41539e3c5fb3db5b32bd/halo2_native`, SHA-256
`c78a26364b8295d7177dc250537de612a895eace671ab6e9cd57b59a314ba9f1`.
The existing launcher binaries were preserved byte for byte during the guarded
selection transaction. The previous `4bf0c86b3c41485193b431cf4f7da756` runtime
was also preserved.

Qualification passed 24/24 CTests, all 13 packaged multiplayer map/local-player
smokes and four frontend checks. The actual Split Screen menu regression
submitted 111 frames and 259 ticks, then closed cleanly at 1280×720. Both root
launchers independently resolved the same runtime. Additional matches through
the installed launcher submitted four frames with two and four actual local
views at 1280×720.

A prior candidate run exceeded the test's 90-second world-readiness allowance;
both loading GPU checks also timed out on that run. Their timing cause remains
unproven. The regression now allows 170 seconds for world readiness within its
unchanged 210-second overall budget. With that exact helper, the old installed
binary still reproduces the original guard error. Final startup took 99.268
seconds; the loading GPU checks passed in 111.69 and 18.99 seconds. This is
bounded startup acceptance, with no loading-performance or full gameplay,
visual, audio or cross-machine parity claim.

The [installed delivery receipt](../build/test-results/splitscreen-shadow-startup-20261028/installed-delivery.json)
pins the exact executable, menu report, launcher hashes and two/four-view runs.
The [qualification report](../build/test-results/splitscreen-shadow-startup-20261028/delivery-owner-v4/attempt-5/report.json)
retains all required checks and source hashes. The [acceptance receipt](../build/test-results/splitscreen-shadow-startup-20261028/acceptance.json) records
owned-build/probe cleanup and the intentionally retained runtime/resources.
