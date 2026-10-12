# Native controller hotplug ownership and original mapping boundary

The current native controller list changes a surviving controller's local-player ownership when an earlier device disconnects. A CPU-only actual-SDL reproducer repeats this failure: controller instance2 keeps forward1/reload1, but removing instance0 changes its recipient from local view3 to view2. No controller value or identity changed. The faulty routing is the combination of `Platform::refresh` compacting its handle vector and the input caller selecting a pad by the current vector index.

This is read-only research and a proposed native adapter seam. The production runtime/frontend and all previous frozen evidence remain untouched. No GPU, window, game unit, pose, vitality, contact, readiness, player/controller table or gameplay result was supplied.

[Exact runtime source snapshot](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/runtime-source.cpp),
[unchanged refresh/axis and literal selector extraction](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/runtime_refresh_extract.hpp),
[complete report and individual pins](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/report.json),
[actual first reproduction](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/reproduction1.stdout),
[actual failing assertion](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/reproduction1.stderr).

## Existing native assignment

Current runtime explicitly combines keyboard/mouse and first-pad input for a single local view. With multiple views, keyboard/mouse owns view0 and pad indices0/1/2 feed views1/2/3. It keeps a vector of live controller handles and deduplicates them by `SDL_JoystickInstanceID`. It retains no independent seat owner. These are native host input conventions, also described in the controller paragraph of [MULTIPLAYER_PROGRESS.md](MULTIPLAYER_PROGRESS.md); that paragraph's older key bindings are not used as current keyboard authority.

Both `SDL_CONTROLLERDEVICEADDED` and `SDL_CONTROLLERDEVICEREMOVED` call `refresh`. Erasing an earlier handle moves every subsequent vector element down one index, while the view-to-index formula stays the same. The bug affects movement and button actions: the reproducer observes the same held raw reload request move along with forward input. The earlier [natural PvP report](MP_NATURAL_PVP_DAMAGE_RESEARCH.md) records the same ownership shift through the actual full SDL runtime, independently of this minimal test.

## Original source evidence

The original owns independent fixed local-view and controller tables. This supports separating native discovery from native ownership; it does not establish an Xbox port enumeration or reconnect-seat policy.

| Owner | Original1.1 | Authenticated u5 | Exact fields |
|---|---|---|---|
| Local-key/member owner | `14DFF0` | `14F3D0` | Matches signed16 player+1A to player-global+98; passes signed16 player+1C to the view binder and DWORD player+20 to the controller binder. |
| View binder | `1508B0` | `151C90` | Writes signed16 player+28; full player datum in player-global+0C+4*view; increments/decrements signed16 count+8. |
| Controller binder | `150950` | `151D30` | Writes DWORD player+24; full player datum in player-global+1C+4*controller; increments/decrements signed16 count+A. |

Source global owner slots are `[4F01F0]` in1.1 and `[4F54F0]` in u5. The player pool owner is separately `[4F01F4]`/`[4F54F4]`. The binders take stack `(full_player_datum,new_index)` and return with `RET8`. A same-index request returns without remapping. A changed binding first releases the old binding. Unbinding writes NONE to the selected table entry and corresponding player field and decrements its count; their owning instructions do not compact the remaining entries. View binding additionally calls its actual control/first-person owner callbacks. Native input must not manufacture those original fields merely to obtain a successful source scheduling gate.

Complete1.1 dispatch `188AD0` reads the four fixed player-global view entries, skips NONE and preserves the associated output-slot/mask index. `1884E0` reads player+28 for its per-view control record and player+24 for processed controller input. Thus controller and view are distinct identities; a device's current discovery ordinal does not establish either source field.

[Original1.1 local-key owner](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/0014dff0.asm),
[view binder](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001508b0.asm),
[controller binder](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00150950.asm),
[complete fixed-view dispatch](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/00188ad0.asm),
[controller/view command consumer](../analysis/multiplayer/integration/spawn-source/local-command-scheduling/instructions/001884e0.asm),
[mapped u5 local-key owner](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-0014f3d0.asm),
[u5 view binder](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00151c90.asm),
[u5 controller binder](../analysis/multiplayer/title-update-1.5/scoreboard-profile-bridge/measured/instructions/u5-00151d30.asm).

Those instructions were verified by the previous immutable source suites. The binding contract here is static: the source-created startup context still stops before genuine local membership/controller/view binding. The latest [map-clock freeze](MP_LOCAL_MAP_CLOCK_RESEARCH.md) stops callback23 at ordinal128 `KeQuerySystemTime`, after two source-backed compatibility ordinal156 reads. It does not execute local ownership or readiness. Earlier neutral/declared command fixtures do not close that producer boundary. No new original-engine dynamic execution is claimed by this native SDL test.

## Proposed smallest native seam

Keep live-device discovery and local-view assignment separate:

1. A registry maps the actual current `SDL_JoystickID` to its live handle. Enumeration can be compact and reordered without changing any seat.
2. A fixed four-entry native assignment table stores an optional instance identity per local view. Its small interface is explicit `assign(view,instance)`, `detach(instance)` and `resolve(view)`. Ordinary input polling resolves the existing assignment rather than indexing the discovery list.
3. Initial assignment may preserve the exact current native single/multiple-view convention. Label that as the existing host adapter policy. A detach clears only assignments and held/pending pad input owned by that instance; keyboard input in view0 and every surviving assigned device remain independent. Missing devices provide neutral pad input and never transfer another participant's commands.
4. New device discovery alone does not prove a previous seat. The primary installed SDL header defines an instance ID as unique during connection/application lifetime and gives a reconnected device a new identity. A caller may explicitly assign a fresh device to an available view; any automatic reconnect/fill policy needs an explicit native policy decision or additional original evidence. Matching only GUID or vector ordinal is insufficient, especially for identical controllers.
5. Runtime delivers the resolved pad requests through existing `Controls` for the actual native local view/player. This adapter creates no source membership, player+24/+28, readiness or state-four bytes. Original local-session producer parity remains unresolved.

SDL's primary identity contract is `/usr/include/SDL2/SDL_joystick.h` (`SDL_JoystickID`, `SDL_JoystickGetDeviceInstanceID`, `SDL_JoystickInstanceID`), whose exact hash and installed2.32.74 version are retained in the report. No physical-port identity is inferred from it.

The corresponding production regression should exercise the real extracted module/API after root integrates it: three simultaneously connected devices, remove an earlier assigned device, retain the untouched device's view/actions; repeated hotplug notifications; later unused-device removal; two identical device GUIDs with distinct instance IDs; explicit fresh-device reassignment; and single-view keyboard input with an idle/disconnected pad. Assert that removed-device held/pending input cannot leak into another view. Automatic reconnect behavior should be tested only after its policy is explicitly selected.

## Reproducer and freeze

[hotplug_probe.cpp](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/hotplug_probe.cpp) uses actual SDL virtual controllers, actual raw forward/reload inputs and the actual add/remove event queue. Its wrapper copies the unchanged runtime `refresh` and `axis` bodies and literal view selector; only the window-dependent constructor is omitted. Three owned VID/PID0 devices share the normal virtual GUID, so identity deduplication already has a same-GUID case. A test-only SDL whitelist requires zero recognized preexisting physical controllers; detach/cleanup only targets this probe's owned virtual devices. It neither alters nor detaches user hardware.

From the repo root:

```bash
analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/hotplug_probe
```

Expected current result is exit1 with `FAIL unchanged connected controller changed local view after another device detached`. Two fresh executions produced byte-identical stdout and stderr. Provider failure is separately exit2; it is not a hotplug ownership verdict. This frozen extracted reproducer demonstrates the current bug and cannot by itself verify a subsequently edited production adapter: that regression must import the new real seam or regenerate into a fresh evidence directory.

[Freeze manifest](../analysis/multiplayer/integration/spawn-source/controller-hotplug-20261008/manifest.json) pins this note, runtime snapshot, extraction, report, binary/source and both executions. This work leaves the bug present for root's implementation; it does not claim Xbox hardware hotplug behavior, full session binding parity or new gameplay effects.
