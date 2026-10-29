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
JSON report to pass with at least one actual frame before publication. Missing,
failed and zero-frame reports reject promotion and preserve the existing
selections. Synthetic policy tests prove that admission rule separately from
the actual menu/runtime check.

Evidence is under `build/test-results/splitscreen-shadow-startup-20261028`:
the original menu reproduction, permanent regression failures, current source
pins and the delivery owner's reports. Bounded installed-binary disassembly is
in `build/test-results/splitscreen-shadow-crash-20261028/installed-binary-proof.json`.
Runtime delivery remains pending until the new candidate completes qualification.
