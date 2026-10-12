# Solo Split Screen launch, 2026-10-11

The installed title route reproduced the reported countdown, Halo 2 loading
logo and process exit with one signed-in profile. The first failure was
`Actual selected scenario has no supplied Vista cache owner`.

The source import discarded Vista's scenario caches and shared sound owner
after exporting UI artwork. `NativeVistaOfflineFlow::world_loaded` requires
the selected scenario's original response bank and its exact shared audio
namespace. Setup now retains and validates the supplied Vista catalog, hashes
it in provenance, and commits it with the UI pack. Xbox remains the gameplay
and world-asset source. Both launchers reject the old incomplete default map
catalog before starting. The active import was rebuilt from the supplied
installer directory; all 41 imported cache headers and provenance hashes were
checked. Original inputs, the older import and user preferences were preserved.

The misleading party warning was independent. `MenuState::lobby_view` assigned
Xbox status11 during the real Vista countdown. The unchanged Xbox24FFC9 /
19A50F and English menu data bind that status to “You need to be in a party”.
Status10 binds the original countdown/cancel text and expands E413 from the
actual retained countdown stage. The source status table was left intact;
the live caller now supplies status10.

After restoring the audio catalog, the full GPU route exposed a second exit.
The debugger caught SIGSEGV in `vkAcquireNextImageKHR` on the loading worker,
immediately after the old renderer's instance/surface teardown. The existing
`desktop_loading_bridge_gpu_tests` reproduced the same crash without gameplay
or audio. Scene renderers now retain one shared Vulkan instance/surface for
the same SDL window, while each scene keeps its own device and GPU resources.
The departed scene/device still expires at handoff. Forward and reverse loading,
guarded departed CPU data, resize, fade/join, cancellation and worker exceptions
pass with Vulkan validation on the local RTX5080/Wayland path.

A separate observed `VK_INCOMPLETE` surface-format enumeration now retries the
count/fill query, following the installed Vulkan-Hpp enumeration pattern. The
SDL test probe can inject this legal platform response once to lock down the
recovery path.

Regression commands and artifacts are under
`analysis/frontend/solo-splitscreen-20261011/`. The importer test goes red on
both directory and nested archive imports that discard match owners. Launcher
tests go red when either required default cache is missing. The real-clock
roundtrip asserts the actual countdown status and exercises two authored match
expiries, player activation, retained results, Done, fresh lobby and cancellation.
`tools/test_solo_splitscreen_flow.py` uses the launcher's actual asset registry,
normal SDL sign-in/Start input, real countdown/loading, and GPU presentation
through match tick90. It deliberately supplies no frontend frame limit or
simulation limit that would bypass the source lifecycle.

The final run used audio and Vulkan validation, recovered an injected
`VK_INCOMPLETE`, and rendered 278 gameplay frames through tick90 at the resized
1100x650 viewport. All 10 launch assertions passed. The importer suite passed
7 tests, launcher verification passed 70 cases, and the current real-clock
roundtrip completed both matches. Both installed launchers were refreshed from
the tested Release build; `delivery.json` records their resolved runtime and
hashes, alongside `final-title-live/report.json` and its gameplay capture.

This verifies the reported solo launch path and its loading ownership. It does
not establish multiple-controller split-screen gameplay, full platform parity,
or original framebuffer equality. Diagnostic probes, before-fix binaries,
captures and debugger logs are retained only in the named analysis directory.
