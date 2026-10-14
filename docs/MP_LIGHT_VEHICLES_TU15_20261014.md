# TU1.5 light vehicle construction and live integration

The reference is the owned English retail TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The complete player, vehicle and object physics goal remains active.

## Original choice evidence

`tools/oracle_light_vehicle_choices_tu15.py` executes unchanged `15CEF0` and
its actual `15CB60` callee. The original dispatcher decrements the authored
classification byte, reads Primary Light at game-options+209 for class1 and
Secondary Light at+20A for class2, and makes the authenticated calls at
`15CF0D`/`15CF25`. The selector reads the original matg/mulg reference bank.
There are no replaced calls, supplied selector returns or physics outputs.

| Choice | Original result |
| --- | --- |
|0 |Retain the collection definition and variant |
|1 |Reference+04, variant0 |
|2 |Reference+04, variant05000694 |
|3 |Reference+0C, variant0 |
|4 |Reference+34, variant0; absent from the Primary Light UI |
|5 |Reference+2C, variant0 |
|7 |NONE; no template or body birth |

Choice6 contains the original per-birth random loop and is outside this fixed
fixture. Cache loading explicitly rejects it on affected rows until its actual
random choice and changing-definition template owners are integrated.

Evidence under `analysis/multiplayer/physics-live-tu15-20261012/`:
`light-choices-original-v239.json` covers91 cases from owned Coagulation,
Headlong and Zanzibar, executing76 of138 sites in the dispatcher/selector CFGs.
The native test matches all91 definition/variant/placement results and checks45
authored child templates. It also checks matching variant admission and rejects
source-free authoritative stepping before changing the world clock.

## Wiring

`Cache::load_content` resolves both fixed light choices into cold netgame
templates before loading the authored model children and interaction metadata.
`LevelContent::light_vehicle_options` retains those configuration inputs, not
body or solver state. The original netgame factory still owns each birth.
`validate_slayer_map_options` rejects a primary option/template mismatch;
frontend validation now loads the selected variant instead of Map Default.
Clients can retain the same templates without a local source physics world;
authority stepping still requires that owner on affected option rows. Inert
saved options on maps without affected rows retain their earlier behavior.

`light-vehicle-ctest-v239.log` passes both original light choices and the actual
saved-profile/Lockout lobby regression. An earlier CTest invocation was made
before the lobby target had finished building and reported it Not Run; the
completed-target run supersedes that availability result.

## Recorded Spectre

`live-light-spectre-v238/lan-vehicle` passes a visible UDP session on the actual
authored Coagulation class1 spawn, selected through ordinary parameter76=5.
The original factory creates the Spectre and authored turret child. The driver
approaches the real entry marker, boards, drives, turns and exits through SDL
input. It travels17.249096 world units, turns0.725755 radians, submits449 actual
original vehicle-camera frames, turns that camera0.600000 radians with mouse
aim, and submits510 player-camera frames after exit. All902 UDP publications
compare exactly. `window.mp4` is the unaltered X11 recording; its10-second frame
is retained as `vehicle-camera-frame.png`. Both peers use the selected variant.

## Gauss failure and build consistency

`live-light-gauss-v240` selects the authenticated parameter76=2 Warthog variant
05000694 but crashes before the first frame. Its frozen ELF has SHA-256
`ebf89905b3da071a93748daefa3623610a0d5d4ce9e09be1f6fe911a8d77d501`.
`gauss-crash-gdb-v240.log` locates the fault in actual authority snapshot
attachment validation, before any driving test completes.

The shared attachment header changed during compilation. GDB on the compiled
objects reports `sizeof(SourceAttachedEffect)==28` in `multiplayer.cpp.o` and
`multiplayer_network.cpp.o`, but56 in `source_attached_effect.cpp.o`. The new
record adds its actual location vector. A successful link did not prove those
objects agreed on the C++ layout. This is retained as a failed live case,
without changing Gauss forces, contacts, admission or solver output.

The header's dependents are forced to rebuild in `light-vehicle-build-v241.log`.
`headers-before-build-v241.json` and `coherent-build-v241-result.json` record
whether public/internal headers remain unchanged across that rebuild. Fresh
live testing is required after the consistent build; the cold choice checks
alone do not resolve the failed Gauss run.

The rebuilt candidate's header-stability audit then detects a concurrent
`simulation.hpp` edit (`coherent-build-v241-result.json`). That candidate is
not treated as verified. Source snapshot `.local/physics-port-v242` preserves
6291 files with `source-snapshot-v242.json`; its clean build exits0 and all three
attachment types report56 bytes. The snapshot's C++ sources/headers remain
unchanged through the build. Its first Gauss run fails cleanly in tag loading.

`gauss-tag-gdb-v242.log` identifies the first-person attachment preload. The
owned Coagulation mortar MP weapon `F0210EAC` legally shares two attachment
rows at`80526B1C` with its base mortar weapon, before the MP tag's own payload.
`shared-weapon-attachments-original-v243.json` retains the original bytes and
their actual containing tag. The root runtime already resolves that owner
through `query_tag(.contains=...)`; the isolated candidate adopts that same
fix, keeping strict per-tag bounds. This is not an invented or skipped bank.

`live-light-gauss-v243/lan-vehicle` then passes:32.557823 world units of actual
travel,0.982938 radians of vehicle turn,0.600000 camera response,539 seated
original camera frames,594 camera frames after exit and901 exact UDP states.
The actual original variant/model constructor supplies the Gauss turret child.
`source-snapshot-v243.json` records the deliberate isolated source update.

## Independent client preferences

`v244` adds a read-only check of actual vehicle geometry submissions in both
views and compares each submitted definition with the authority's configured
placement definition. It cannot select a model or supply a physical result.
`source-snapshot-v244.json` records this acceptance-only update.

`live-light-client-default-v244` declares host Primary Light5 and client Map
Default0 through ordinary configuration inputs. The join is admitted, then
fails with `posed model world bank count changed`: the remote source Spectre
hierarchy is applied to the client's locally prepared Warthog hierarchy.
This records the configuration/bootstrap gap before its fix. Protocol76's
complete host configuration discovery now runs before LAN client world/model
preparation. The different-preference Spectre case and Gauss/Wraith counterparts
pass, including a different saved starting weapon. See [session configuration
fix and evidence](MP_SESSION_CONFIGURATION_TU15_20261014.md). Actual ONLINE
loading adoption remains open; matching-configuration sequences alone cannot
establish that path.

## Remaining coverage

All seats, turret operation, secondary-option live cases, random births and
changing definitions, independent client preferences/host configuration
adoption, full retail trajectories and default-launcher delivery remain open.
This evidence proves the stated original choice paths and recorded Spectre
sequence; it does not complete all vehicle or shared physics parity.

## Latest isolated live checkpoint

`v245` retains ordinary observer retreat input until it has actually cleared
0.75 world units or its finite120-sample budget expires. The travel requirement
is unchanged. `v244` Gauss had correct geometry in both views and completed
driver/camera controls, but its short observer input moved0.490835 units and
failed the0.5-unit clearance requirement. No body state or physics value was
changed to resolve that fixture limitation.

| Vehicle | Exact UDP states | Travel | Vehicle turn | Camera turn | Seated camera frames | Geometry submissions in each view |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Gauss Warthog, `live-light-gauss-v245` |902 |34.480911 |0.975394 |0.600000 |603 |1184 |
| Spectre, `live-light-spectre-v245` |902 |18.908596 |0.717764 |0.600000 |495 |1045 |

Both pass boarding, forward drive, turning, exit, original camera submission
and response, restoration after exit, actual submitted vehicle definitions
and geometry in both client views. Their observer retreats1.032776/2.637114
units. `light-vehicle-suite-v245.json` records executed ELF/video hashes and
verifies C++ sources/headers against the versioned isolated source manifest.
`window.mp4` and an unchanged10-second `vehicle-camera-frame.png` remain beside
each report.

The isolated `v244` runtime also passes recorded movement/lift/objects with
181/181/182 exact UDP states. The prop explosion again matches all512 actual
motion bytes for its two point impulses against unchanged TU1.5. This does
not resolve the independent client-preference failure described above.
