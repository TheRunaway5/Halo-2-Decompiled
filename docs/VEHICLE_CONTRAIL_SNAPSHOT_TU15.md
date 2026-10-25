# Live contrail CRT policy ownership

Sustained native MCC Wraith firing reaches a path absent from the static model
capture and headless weapon checks. The [rendered failure](../analysis/vehicles-tu15-goal-20261023/current-rendered-firing-v1/wraith/report.json)
stops with `Contrail snapshot omitted mutable source bytes at4648144 size4`
(`46ECD0`). It occurs during contrail rendering; the report has no completed-run
frame count, which must not be interpreted as proof that no frames were drawn.

The [original audit](../analysis/vehicles-tu15-goal-20261023/contrail-math-policy-original-v1.json)
pins English TU1.5 SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`,
the writable `.data` word and both complete original reads at32EA98/32EB36.
Its loader initializer is2694. The native world already owns this writable CRT
math policy; the sparse contrail snapshot omitted it. A later source read or
the original loader value cannot stand in for a completed authority snapshot.

The [first integrated repair](../analysis/vehicles-tu15-goal-20261023/contrail-wraith-rendered-v1/report.json)
gets past that word and exposes the next missing live x87 error-dispatch policy
at48D4F4. Its [separate original audit](../analysis/vehicles-tu15-goal-20261023/contrail-x87-policy-original-v1.json)
authenticates the writable four-byte owner, zero loader initializer and fixed
original comparisons at32CD20/32CD84/37CE67/37CE84. The existing native world
already owns it. No hook, handler success or arithmetic result is supplied.

`capture_source_contrail_snapshot` now copies these two exact four-byte live words, preserving
the existing immutable-only fallback and refusing omitted mutable data. It does
not change geometry, math dispatch, arithmetic, FP state, admission or clocks.
The same generic snapshot serialization carries this input to remote views.

The [red regression](../analysis/vehicles-tu15-goal-20261023/contrail-live-policy-red-v3/report.json)
reproduces the exact omitted-word failure. The
[first green check](../analysis/vehicles-tu15-goal-20261023/contrail-live-policy-green-v1/report.json)
checks zero, the original initializer and all-one live words, then changes the
backing word after capture. The [second red check](../analysis/vehicles-tu15-goal-20261023/contrail-live-policy-red-v4/report.json)
reproduces omitted48D4F4. The [two-word regression](../analysis/vehicles-tu15-goal-20261023/contrail-live-policy-green-v2/report.json)
checks nine combinations of zero/nonzero/all-one source policies and subsequent
independent backing mutations. The replica must retain the captured values, capture
must leave source state unchanged, adjacent mutable bytes remain excluded and
deleting the slice must still reject mutable fallback. Existing malformed-input
and immutable-original-data controls also pass. Original instructions are not
executed by this regression or by the production snapshot path.

Two earlier fixture attempts remain explicit failures: the first lacked a C++
algorithm include, and the second gave adjacent pool bindings separate backing
regions even though the capture merges their contiguous interval. The corrected
test provides one explicit eight-byte caller-owned pool-binding region. Those
test repairs supply no game state or successful geometry result.

All focused binaries and library copies are temporary under `tools/run_test.py`.
The existing main build is borrowed read-only and pinned before/after. The
integrated [two-word Wraith rerun](../analysis/vehicles-tu15-goal-20261023/contrail-wraith-rendered-v2/report.json),
current-source cohort and launcher delivery
remain required; this focused repair does not close full vehicle/effect parity.

## Authored transition lookup input and corrected extent

The two-word Wraith run gets through the live CRT reads, then reaches missing
backing at478B38. Complete17F8A0 addresses this byte as curve2, entry144 within
the original transition reader's base4782A8. The authored enum is0..7;
type0 bypasses lookup. Only types1..7 are table inputs, covering precisely
4786A8..47A2A8 exclusive. The next byte is writable placement policy.

The [export audit](../analysis/vehicles-tu15-goal-20261023/fade-curve-original-v1.json)
incorrectly treated the fixture's sixteen-value numeric sweep as the authored
table extent. Its isolated green and negative checks did not construct a cold
world and cannot qualify that extent. The fresh full-world run rejected an
ownership conflict at47A2A8 before any runtime promotion. Those reports remain
as superseded evidence of the invalid assumption.

The corrected [seven-row audit](../analysis/vehicles-tu15-goal-20261023/fade-curve-original-v4.json)
checks all7,168 consumed bytes against both the pinned original image and the
independent `light_fade_curves_tu15_original.json` fixture. It authenticates the
pinned Falloff Function enum, the linear no-lookup branch, and the
original shift, base address, endpoint bound and three byte-load instructions.
The complete original text has one occurrence of the bank's base literal,
inside that reader. The original section is `.data`; the native provider retains
these established static table inputs as immutable data. No runtime-produced
geometry, source code or mutable CRT word is exported with them.

`native_tu15_fade_curve_data.inc` is generated by
`tools/export_native_tu15_fade_curve_data.py` and registered with the existing
byte-validating native branch-data provider. The complete compiled17F8A0 body is
unchanged. Its whole-bank [red check](../analysis/vehicles-tu15-goal-20261023/fade-curve-native-red-v1/report.json)
fails at the absent base. The [green check](../analysis/vehicles-tu15-goal-20261023/fade-curve-native-green-v1/report.json)
used the invalid extent and is superseded. The original scalar fixture remains
unchanged:9,296 cases are in the authored0..7 domain; the other numeric probes
do not authorize additional production data. The [earlier private negative control](../analysis/vehicles-tu15-goal-20261023/fade-curve-private-negative-v1/report.json)
flips one bit at478B38 in its owned copy and requires the independent comparison
to reject it; original inputs and checkout bytes remain unchanged.

The new regression checks every consumed byte in both an empty provider and
the complete cold world, preserves the writable placement initializer and
selection sentinel, mutates both live owners, then verifies repeat admission
preserves their values and ownership. It excludes unused row0 and adjacent
mutable bytes. The first cold-world fixture attempt failed compilation due to
an incorrect Bytes accessor; that report is preserved separately.

The corrected [cold-world check](../analysis/vehicles-tu15-goal-20261023/fade-curve-cold-world-green-v1/report.json)
passes. Its [private one-bit control](../analysis/vehicles-tu15-goal-20261023/fade-curve-cold-world-negative-v1/report.json)
changes only478B38 in the owned native table and fails the independent byte
comparison as required. Both temporary owners were removed; borrowed native
libraries and frozen source remained unchanged.

The next rendered run exposed a second issue: a contiguous seven-row export
split around an existing row into2,048/4,096-byte owners. The retained material
loader requires complete1,024-byte row owners and correctly rejected them.
The [direct shared-owner regression](../analysis/vehicles-tu15-goal-20261023/fade-curve-material-overlap-red-v1/report.json)
reproduces that exact failure. The exporter now registers seven separate rows,
preserving the material loader's ownership contract and all existing guards.
The regression loads the actual retained material bank, then repeats admission
through both providers while preserving live placement state.
The [shared-owner green check](../analysis/vehicles-tu15-goal-20261023/fade-curve-material-overlap-green-v1/report.json)
passes this actual retained-bank path and all existing original lookup checks.
Its temporary build was removed and the borrowed frozen inputs stayed unchanged.

The rebuilt [four-check ownership cohort](../analysis/vehicles-tu15-goal-20261023/delivery-ownership-validation-v1/report.json)
passes. The subsequent [real-output Wraith run](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-rendered-v3/report.json)
and [dummy-driver control](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-rendered-v4/report.json)
both exceed their300-second deadlines after passing the earlier ownership
boundaries. A zero completed-run frame field is not proof that no intermediate
frame drew. The real-output run opens no game audio client and produces only
silence; it does not establish audio playback. Its private sink and119,608,320
raw PCM bytes were removed, preserving the desktop default. Passive timing in a
private fixture is the next diagnostic; no larger deadline or parity pass is
inferred from startup progress.

The [passive diagnostic](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-progress-v1/report.json)
enters `run_multiplayer`, finishes BSP light-material preparation and never
reaches its first input or completed-frame callback. It was deliberately
stopped; SDL handled SIGTERM without leaving startup, so only that owned
diagnostic process was killed. Its source/library copies were removed.
The [parent-owned debugger capture](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-parent-stack-v1/report.json)
places the active native main thread in NVIDIA's SPIR-V compiler at the
controlled stop. This identifies a renderer-startup diagnostic boundary,
not a completed in-game, audio or full-parity result. A deeper native caller
capture and the full371-case headless cohort are separate follow-ups.

The [deeper stack](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-parent-stack-v2/report.json)
and [native call-site disassembly](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-pipeline-site-v1.json)
identify `Renderer::Impl::make_pipeline`'s `create forward material pipeline`
call, before any input/frame callback. The existing [generic-combiner control](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-generic-control-v1/report.json)
completes90 Wraith frames, one projectile,46 colored trail submissions per view
and one matching Classic firing voice. The [private optimization-disabled control](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-pipeline-opt-control-v1/report.json)
keeps specialization and also completes90 frames in123.37 seconds. It changes
only native Vulkan creation flags in its owned interposer, preserving actual
API return values. Neither control qualifies the production default.

Vulkan documents `DISABLE_OPTIMIZATION` as a pipeline-creation policy that may
reduce compile time. [Vulkan flag reference](https://docs.vulkan.org/refpages/latest/refpages/source/VkPipelineCreateFlagBits.html).
The [native device probe](../analysis/vehicles-tu15-goal-20261023/renderer-native-vulkan-devices-v1.json)
records the measured Linux tuple `10DE/2C02/99D38200`. The candidate production
change applies that flag only to specialized forward pipelines on this tuple,
then resets creation flags before other pipeline families. Shader words,
specialization values, raster state and gameplay remain the same inputs.
A narrower private control and a real production-default rebuild/rerun remain
required before qualifying that candidate.

The [specialized-only control](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-specialized-opt-control-v1/report.json)
completes90 frames with the existing61-word material specialization intact.
The coherent production rebuild's [default-path run](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-rendered-v5/wraith/report.json)
then completes90 actual Vulkan frames, one mortar projectile, authored trails
and one Classic voice submission in134.28 seconds, without an interposer or
generic-combiner override. Its [retained frame inspection](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-frame-inspection-v5.json)
records the visible seated Wraith and blue-white trail. This qualifies the
bounded startup/firing path, not original framebuffer or full vehicle parity.

The real-output gate remains failed for that run: the private monitor receives
only silence. A [silent SDL routing diagnostic](../analysis/vehicles-tu15-goal-20261023/sdl-native-audio-route-probe-v1.json)
shows the real Pulse backend connected its owned client to the desktop sink
despite `PULSE_SINK`, while the private monitor remained empty. The capture
harness now binds the actual child process and moves only that live child's
streams to its owned sink. It records each move, rejects foreign clients and
preserves the desktop default. Actual game PCM must pass a fresh run; the
silent routing diagnostic itself supplies no gameplay-audio evidence.

The [fresh routed game run](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-rendered-v6/report.json)
passes all bounded checks in124.40 seconds. Its real game client supplies
956,415 nonzero samples, peak1.0 and RMS0.07591 at stereo48kHz. The report records
the actual child PID and its sole stream move; no foreign stream was moved.
The sink is removed, the desktop default is preserved and46,794,240 raw PCM
bytes are removed with the test owner. This verifies host output alongside
the native firing/visual events, without claiming original waveform equality
or physical-speaker audibility.

Both control PNGs were visually inspected. Their [full-frame comparison](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-combiner-control-pixel-comparison-v1.json)
has7,030 differing pixels. [Region counts](../analysis/vehicles-tu15-goal-20261023/delivery-wraith-combiner-control-pixel-regions-v1.json)
place7,029 in the animated projectile-effect rectangle and one at a HUD edge;
the lower view and remaining world pixels match. These independently started
runs do not control every presentation RNG input. No original Xbox pixel,
effect, waveform or full-vehicle parity conclusion follows from that comparison.

The fresh delivery snapshot must now compile the corrected extent together with newer
shared audio/physics/rendering work. It adds both contrail-input regressions to
the complete vehicle cohort. The prior two-word Wraith run remains a failure,
and no rendered/full-parity pass is inferred from these data checks.
