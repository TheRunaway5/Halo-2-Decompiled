# Two-player Split Screen

In Split Screen sign-in or its lobby, press A/Start on the second controller
and choose or create a separate profile. The picker belongs to that controller;
its selection preserves the host profile. Cancelling the picker preserves the
existing roster. Both players appear in the authored sign-in and lobby widgets.
The keyboard remains player one's input, so a keyboard user can also join a
second player with a controller.

The launcher now carries the actual SDL controller instance IDs alongside the
selected profiles. Gameplay resolves each view's controller by identity rather
than assigning it again by device enumeration order. The same retained menu
owner keeps those selections across game and mainmenu resource teardown.

The offline session adapter publishes separate local users through the recovered
controller/profile and reconciliation routines. Each explicitly selected user
owns an independently initialized offline identity bank; controller zero owns
the session's machine context. This is a native local-user provider extension,
not a claim that stock Vista exposed native split-screen. The supplied DLL's
original identity constructor and selected-user XUID getter remain unchanged.

World construction validates each reconciled player against its actual initialized
options and profile request. The first source tick activates and binds each local
player separately. Gameplay fade and announcer recipient handling include every
local view. Results retain both actual profiles, identities and statistics. The
native recovered ranking comparator supplies encoded ranks; original PC
469462..469495 divides those ranks by two for postgame placement. The source
executable was also executed with two occupied players for ties and either winner.
Done opens a fresh lobby with both selections, and another Start reuses them.
Leaving the browser with multiple users executes the recovered reason2 title9
route (60CE70 ->63F6B7 ->63F57F); the native title presentation retains both
profiles. The single-user path continues to allocate the original PC main menu.

`native_vista_flow_tests --two-players` runs two real-clock countdowns and actual
CPU match ticks through authored expiry, postgame, results, Done and fresh lobby.
Its optional capture directory exercises Vulkan loading in both directions,
both gameplay views, independent virtual SDL controller input and the real
results renderer. Intervening match ticks are accelerated on the CPU; this is
not a full rendered match or a physical-controller test.

`tools/test_solo_splitscreen_flow.py --players 2` additionally drives two virtual
SDL controllers through the actual title/profile/Split Screen/Start route and
renders the installed asset registry through match tick90. Evidence and source
ranking controls are under `analysis/frontend/two-player-splitscreen-20261011/`.

The delivered build passed two two-player round trips, the solo regression,
the original session/menu-service and browser-return fixtures, and all eleven
title-to-match checks with Vulkan validation enabled. Both local views received
independent virtual SDL inputs; both completed reports retained both players;
Done retained the roster for the next round. No Vulkan validation errors were
reported. `delivery.json` records the tested runtime and both installed launchers.
`settled-lobby/native-initial-lobby.png` confirms both profile names and the
two-player count after the authored entry animation has settled.

The strict GPU capture report retains `passed=false` because another concurrent
workflow changed only its `CMakeLists.txt` input pin while it ran. The captured
process exited successfully and all functional checks passed; runtime, compiled
source inputs and assets were unchanged. `functional-validation.json` records
that distinction without changing the strict report.
