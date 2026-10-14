# Normal launcher world-feature delivery

## Current map and peer delivery, October 14

Both root launchers now select `.local/map-world-20261014/halo2_native`,
SHA-256 `4f0caddf049367c06cd5fc2a215a1f948180398b266a9bf49656029e3a1e0c8d`.
Both installed `--launcher-info` outputs confirm that path. Their current
MCC-only import/readiness policy is preserved: native MCC gameplay remains
unavailable, and `--launch-game` rejects that unresolved state. The direct
original-XBE captures below do not establish MCC launcher gameplay acceptance.

The combined runtime uses one frozen current source set, preserving peer
networking and combat changes, plus the authenticated 100-body / 9,422-instruction
map module. Source, overlay, compiler/link inputs and original registration
receipts are under `analysis/map-world-features/coherent-peer-20261014/`.
The final original banks pass **1,744 bounded cases**. Native physical byte
protection, the actual decal cache export and real encrypted direct UDP also
pass. These are scoped tests, not complete game or Xbox visual parity.

`captures/ordinary-verification.json` records **15 ordinary rendering cases**:
all 13 maps that complete the first source tick, plus four-view Lockout and
Foundation. Six additional captures establish Lockout snow (**20,078 changed
pixels**) and Foundation steam (**38,829**), with byte-identical enabled
repeats and zero Vulkan validation errors. Foundation's steam camera/root
capture is diagnostic; its ordinary startup/rendering is checked separately.

The runtime's shader-directory literal is relocated to a frozen packaged set
using an equal-length path, preserving compiled literal extents. Every other
allocated ELF section is unchanged; original XBE and native instructions are
unchanged. `.local/map-world-20261014/package.json` records that relocation,
shader hashes, accepted scope and prior launcher hashes. `installed.json`
records both installed launchers. The former launchers remain in the package's
`prior-launchers/`; the earlier 98-body candidate remains in `versions/98/`.

The 21-map first-source-tick bank now passes 13 maps, including Terminal, with
no regressions against the coherent 98-body baseline. Eight still fail: seven
at the absent native decal command owner, and Gemini at separate missing
backing. Terminal's longer attempt fails at step198 on missing physics callback
`2FA6F0`; its short rendering check does not accept complete train behavior.
Zanzibar's earlier computer/barrier receipts remain historical and do not
close the current graphics owner. The full all-map goal remains active.

An earlier copied-input relink was rejected before installation: baseline and
candidate both crashed before world startup in `NativeMeleeAnimation`.
`.local/map-peer-20261013/package.json` is explicitly marked unaccepted. The
coherent rebuild eliminated that failure; it did not replace the current MCC
launcher policy with an older one.

## Historical October 13 map attachment delivery

At this historical receipt, both root launchers selected `.local/map/halo2_native`, SHA-256
`14eadcc50f0522fd05c3d7f6644d715c8ca4ecb495ffa5879e5c710138e47efe`.
Both recorded `--launcher-info` outputs confirmed that exact path. This is a **partial**
[map attachment continuation](SOURCE_MAP_EFFECT_ATTACHMENTS.md), preserving the
installed death/spectating libraries with the explicit source overlays. The
original decal constructor/reset and 82 authenticated map-support bodies are
included; the experimental unsubmitted graphics adapter is excluded.

`analysis/map-world-features/launcher-attachments-20261013/verification.json`
records four ordinary Lockout/Foundation one/four-view cases and six effect
captures through the packaged launcher. All ten pass with zero Vulkan
validation errors. Ordinary 60-tick Lockout snow changes **20,077 pixels**;
Foundation's declared-root/camera effect capture changes **38,778 pixels**.
Both enabled repeats are byte-identical. Foundation's diagnostic capture does
not replace its separate ordinary source-world startup/render checks.

The package retains its tested shaders in `.local/map/s`. Its own native ELF
read-only shader-directory literal was relocated to that frozen directory;
allocated-section verification before and after stripping finds only that
explicit `.rodata` change. No original executable or native instructions were
patched. `.local/map/package.json` records the exact relocation and hashes.
The previous launchers remain in `.local/map/prior-launchers/`.

The later 96-body first-source-tick comparison has no regressions against the prior
installed death/spectating runtime and seven improvements. **Ten maps still fail** that bounded
check, including Zanzibar's absent graphics binding. Earlier successful native
gate/traversal/script receipts below do not establish acceptance of this newer
automatic-original-world path. The full all-map goal remains active.

## Historical 2026-10-12 delivery

At the historical receipt, both root launchers selected
`analysis/map-world-features/normal-world-snapshot-20261012/build/halo2_native`.
The three world helpers forward to the ordinary root launcher:

```sh
tools/run_lockout_snow_test.sh
tools/run_lockout_snow_test.sh --local-players 4
tools/run_rain_test.sh warlock
tools/run_rain_test.sh backwash
tools/run_zanzibar_world_test.sh
```

At that historical receipt, the executable SHA-256 was
`2e108f4aa286e292ba372522e7722bbc09ee00996856ad4af248e53bbfe096d5`.
Its 12,734-file frozen input manifest SHA-256 is
`ba87bf2b7e7e3b731c89201fddec4f9b8f0aeca7ec9920743e957833b8051d70`.
The source freeze retains the combined menus/HUD, online features, campaign
inventory and world-feature code. `phase3-freeze.json` records the three-file
change from the preserved phase-two source: the missing control-marker count
and its two focused fixtures. Only one runtime source file changes in this
phase. Independent later changes in the shared checkout were not overwritten.

## Authored controls and the Relic correction

The native adapter previously reported one marker when a named `use` marker
was absent. Original `B8D30` writes node zero for that fallback but returns
**zero** for a named query. `1078F0` applies its directional gate only when
one marker was returned. A marker-name-zero query instead reports one.
Keeping the fallback matrix while manufacturing a count of one incorrectly
rejected Relic's valid exterior approach.

`analysis/map-world-features/control-marker-20261012/verification.json`
executes unchanged `B8D30`, `B8CA0`, `1D8F00`, `1D8F50`, `BADC0` and `1078F0`
with the supplied four control definitions/render-model marker banks. Eight
marker calls and twelve side calls pass, with no hooked routines. The current
native node matrices are explicit inputs; this does not certify their original
construction or animation clocks.

`normal-world-live/marker-fix-controls/verification.json` records all **eight**
ordinary `MultiplayerMatch` input cases: Zanzibar's computer, both Containment
switches and Relic's switch, each with one and four native players. Each checks
the actual idle prompt/message, rejection of an unheld input, an admitted held
`Controls` command, the retained shared group, gradual linked machine motion,
its endpoint and repeated input. Containment observes 302 intermediate machine
states across each linked pair; Relic observes 502 across its four machines.
These counts are observations across machines, not per-machine durations.
Placement is diagnostic and collision-free, including airborne starts for
elevated switches. These tests do not establish a walking route or SDL input
for Containment/Relic, or complete original unit-center/spatial-query producers.

All **25** selected CTests pass in `phase3-focused-tests.log`, including both
control catalogs, the eight native input cases, shared device/network state,
weather, prop/machine scenes, script/list fixtures, LAN startup, campaign
checkpoints and pause preferences.

## Script and prop integration

The ordinary authoritative match starts authored startup/continuous roots
once its host services are installed. Script execution precedes object/device
updates; observing clients do not independently advance those roots. The
original 48-header/128-reference salted list bank and bounded static-volume
collector now feed production multiplayer dispatch. Named NeverPlaced props
are prepared before their scripts create them, and rendering follows their
actual full-handle births/deaths.

`normal-world-live/marker-fix-scripts/` runs all **23** supplied maps for
**1,200 ticks** with ordinary construction and zero VM faults. Colossus creates
cans, Elongation creates crates, and Terminal/Triplicate removes and recreates
trains. These are script lifetimes; they do not prove falling/conveyor motion,
crushing, full prop lifetime replication/reset, constructor RNG chronology or
all attached effects. See [script evidence and service boundaries](SOURCE_MAP_SCRIPT_PARITY.md).

The earlier list-bank comparison retains 213 operations / 115,659 output words
against original routines with real datum pools, with zero mismatches. The
volume collector retains 84 cases / 45,612 words with explicit spatial services.
Its current native adapter uses authored static volumes, actual scene centers
and native placement order, rather than original cluster-registration order.
Attached volumes remain explicitly unsupported.

## Launcher and live acceptance

`normal-world-live/final-weather/verification.json` preserves the complete
**25-case** bank on phase-two executable
`af882d09546c8afe83092ba13fdf7d64680eb86d73ba2cf68cb8180a339b5f62`.
All cases pass with zero Vulkan validation errors, including ordinary one/four
views, real four-view UDP joins, odd 641x481 four-view rendering, Backwash with
no background plate and a deliberate unsubmitted-frame abort. Finite enabled
repeats are byte-identical. Snow changes 2,450 Lockout pixels; rain changes
2,754 Warlock and 531 Backwash pixels. Plates change 512,612 Lockout and 69,959
Warlock pixels. Backwash's ordinary authored weather script is now active.

The same phase-two normal launcher renders all **21 Xbox maps** without Vulkan
validation errors (`normal-world-live/final-maps/`). Needle and Street Sweeper
fail explicitly because their exclusive Vista lightmap textures have no Xbox
donor. All 23 maps passing CPU script checks does not remove that GPU blocker.
The preserved phase-two source is `source-v2/` with `input-manifest-v2.json`;
its binaries are in `phase2-binaries/`. The current runtime changes only the
control marker count from that accepted rendering base.

The corrected executable's normal-launcher snow on/off/repeat and affected-map
one/four-view checks are retained in `normal-world-live/marker-fix-launcher/`.
The SDL device replay is retained in `normal-world-live/marker-fix-devices-v3/`.
Their final receipts, including launcher/runtime/shader pins, are referenced by
`normal-world-live/verification.json`.

All nine corrected-executable launcher cases pass with zero Vulkan validation
errors. Lockout snow again changes 2,450 pixels and its enabled repeat is
byte-identical. Zanzibar, Containment and Relic each pass one/four-view startup.
All four final X11 device/audio cases pass too: one/four-view SDL hold and
walking traversal, ordinary UDP device PCM and moving four-view late-join PCM.
The local tests record `Computer test SDL driver=x11`, retain 147 intermediate
gate ticks and keep the one-use group after repeated input. Their observed
holds are 248 ms and 333 ms, rather than original timing constants.

The phase-two SDL device bank passes actual Zanzibar computer hold/one-use
rejection, closed/open walking traversal and three authored PCM requests in
one/four views, plus real UDP audio and a moving four-view late join. The gate
retains 147 intermediate simulation ticks. Zanzibar has no authored portal
associations, so its unchanged portal/acoustic mask is expected. LAN actions
in the audio fixture enter through already-admitted host service calls.

The first corrected-build SDL replay failed before gameplay because its window
never received focus. That failure is preserved in `marker-fix-devices/`. The
second harness passed one view but lost focus during the four-view hold; that
failure remains in `marker-fix-devices-v2/`. Direct SDL initialization under
the Xvfb wrapper confirmed that the inherited Wayland environment still
selected the user's desktop Wayland driver. The final runner explicitly
selects X11, clears `WAYLAND_DISPLAY` and verifies the actual local test driver.
The harness retries an actual window focus request only before its gameplay
sequence; it does not synthesize focus events or relax later focus-loss checks.
Its separately compiled source and unchanged runtime object/library hashes are
in `marker-fix-devices-v3/harness-provenance.json`. The exact earlier desktop
focus-loss trigger is not identified. Earlier accepted wrapper-based tests
remain native SDL/Vulkan evidence but do not establish display isolation.

Earlier concurrent-launcher replacement failures and incomplete banks remain
preserved. The accepted base bank pins launchers, runtime and shaders around
each case. A snapshot-copy error was stopped before source edits; its partial
copy was removed, and the original phase-two source/binaries were preserved.

The full all-map goal remains **unfinished**. Remaining work includes attached
effect/function/marker lifecycles, original scheduling/resource/RNG producers,
automatic doors, falling/conveyor/riding/crushing physics, full prop/control
network/reset acceptance, Vista-only texture import and rendered original parity.

## Subsequent shared-build state on 2026-10-13

The root launcher pair now selects `build/halo2_native`, following concurrent
work. The frozen phase3 runtime above remains preserved evidence for its exact
build; it is no longer the current launcher selection. The retained event-clock
continuation passes 5,917 original comparisons and 21-map native contact/effect
regressions. Current launcher snow on/off/repeat captures retain 2,450 changed
pixels and identical repeats. Ordinary Zanzibar startup initially failed at
10F960; its complete original scenery-lightmap lookup is now added to source
and passes 512 exact comparisons. A later isolated runtime aborts at the shared
startup's missing 1888A0 input/camera constructor before scene construction.
Thus live factory acceptance and the full current normal-launcher bank are
still unproven. See `SOURCE_WORLD_EFFECT_EVENTS.md` and
`SOURCE_SCENERY_LIGHTMAP_LOOKUP_TU15.md`; do not treat the earlier phase3 receipt
as acceptance of these later source/runtime changes.
