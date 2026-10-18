# Native light shapes for the campaign adaptation

The current opening contains authored frustum lights named `x01_light_masterchief`,
`x01_light_master_guns`, `x01_light_cairo_fill`, `x01_light_arbiter`, and
`x01_light_prophets`. First/third-person flashlights also use authored shape2.
These are present in the actual current cache; they cannot be replaced with
spheres to complete the presentation. The
[definition audit](../../analysis/pstencil/opening-campaign/native-lights/opening-definition-audit.txt)
retains all24 resident light definitions, including the16 currently supported
sphere definitions and the plasma-charge definition's animation block.

The source-backed current-asset shape adapter compiles complete original TU1.5
`C3960`, `C39B0`, `C3A20`, and `C3AB0`:303 authenticated instructions from the
unchanged English TU1.5 image. `C3820` is the original caller. Its first call
passes the full native light in EAX and the caller's shape in EDI. Its second
passes the actual light frame at record+84 in ECX, the shape in EDX, and the real
definition pointer on the stack. The adapter supplies that ABI and reads the
actual output. It does not supply a computed shape, change tag parameters,
change light state, or initialize unwritten caller bytes. A byte-write mask
keeps those unwritten bytes explicit.

The [generation receipt](../../analysis/pstencil/opening-campaign/native-lights/shape-generation/generation.json)
and [generator](../../tools/pstencil/generate_native_light_shape_tu15.py) retain
image/instruction/module hashes. Native code lives in its own pStencil module;
the shared world has only its include and dispatcher route. Existing Original
and pStencil gameplay/control captures remain pixel-identical to live-v13 after
this route was added.

The real heated Plasma Rifle fixture compares all consumed completed sphere
fields against the independently checked existing sphere arithmetic, including
position, orientation and radii. The Cairo fixture creates an actual authored
`ss_elevator_light` through whole native `C2380`; its complete producers return
shape2 and depth2. The isolated current-source camera adapter now runs complete original TU1.5
`C3DC0`, already owned by the retained native shadow module. It passes the
actual produced shape and light frame as caller inputs, reserves separate
native caller/output storage, and reads the original count and complete
`0x1BC` camera records. It supplies no camera/count result. The actual authored
frustum fixture returns one camera. Its444 camera bytes exactly match the
independently checked existing single-camera arithmetic, with zero differing
bytes in `analysis/pstencil/opening-campaign/native-lights/camera-v1/`.
This current-source follow-up remains separate from the frozen live-v14
helper and does not establish GPU frustum rendering.

The current campaign sphere bridge retains all native light records and actual
PVS order. It selects only its explicitly supported sphere subset. Frustum and
runtime-animation branches remain visible in the resource gaps and are not
relabeled. In campaign-v5, all submitted dynamic-light draws remain zero; the
positive shadow controls there exercise the distant-light path.

The [pass audit](../../analysis/pstencil/opening-campaign/native-lights/opening-frustum-pass-audit.txt)
shows current Chief/Elite materials have real layer12 frustum implementations:
implementation0 uses `lightaccum_frustum`, implementation2 uses
`lightaccum_frustum_no_specular`, and implementation3 uses its gel variant.
Implementation1 is the separate sphere diffuse path, with layer13 sphere
specular. Correct frustum GPU work therefore needs the real implementation
selection, constant/falloff/gel bindings, receiver/portal query, and virtual
light origin. Existing sphere validation and radius<3 receiver helpers do not
admit those branches. Recover those owners before claiming cinematic-light
shadows/specular or the full opening presentation.


The isolated live-v15 candidate adds whole original49790 (679 authenticated
instructions). Its actual heated sphere output exactly equals the independently
checked sphere/fade arithmetic. Its authored Cairo frustum output is finite,
preserves the caller's C39, and retains the original virtual light origin rather
than substituting the light record's physical origin. The constants module,
generator and generation receipts remain in the pStencil folders.

The current candidate also compiles3817 authenticated original instructions for
the previously missing large-bound portal construction, clipping, main-view
intersection, receiver classification and projected admission owners/callees.
The receiver wrapper supplies the literal31E27..31EBD ABI to whole134C00 and
31650, then copies the produced visibility and BSP/object queues. The geometry
wrapper runs the actual31EDA..31172 collection/PRT/cull/command owners; it does
not pass a frustum to the typed sphere-only culler. Its separate31570 admission
wrapper retains original31500/C5170/C3820 and the cinematic feature gate.
Selection must follow the real main collector's C35D0 per-view update.

A declared native opening-cache fixture exercises all five actual cinematic
light definitions at a point taken from the current BSP bounds. Chief, Master
Guns and Cairo Fill produce small-bound mode1 queries (radii2.98433,2.55865,
2.16047). Arbiter and Prophets produce mode0 portal queries (radius13.9258,
four constructed frustums). Each preserves the complete single native camera.
This is an isolated native fixture, not a photographed campaign shot. The Cairo
biped fixture also verifies a real dynamic-object receiver and complete native
collection. It produces four draw units but zero light commands without a full
ordinary main-view setup; no GPU submission is inferred from that fixture.

The projected material binder now retains actual layer12 implementations0/2/3/4,
source gel/default2D bindings, native C31..39 and original shader switches.
15D50 reads shad+3E for30D80. 16D17 publishes the current LOD record;1AFCC
passes its available-layer mask to34050 switch6, where bit13 matters. It is not
an inferred part flag.720 declared cases execute the unchanged original30D80
and34050 switch6 and match the new selection helpers. The fixtures cover every
30D80 instruction and every switch6 branch, not all other34050 switch entries.
120 actual Chief/Elite armor/LOD/light bindings pass the complete native bank
and compiled-material checks. A LOD without layer12 remains without that pass.
Projected passes retain their authored ONE/ONE or DST_ALPHA/ONE blend factors;
they do not inherit the sphere pass's constant-alpha factor.

Campaign-v6's diagnostic reaches the later Chief scene but stops when its old
typed candidate admission consumes shape2. It must not be called a completed
opening run. Campaign-v8 stages native admission after the real C35D0 update,
retains source candidate/history math, and completed the next full integration diagnostic.
The installed runtime remains live-v14. Projected GPU light/shadow pixels and
recording-ready campaign presentation remain unaccepted.


The corrected campaign-v8 diagnostic completes all13 main scenes in215.65s,
rendering3666 frames with zero script faults or Vulkan validation errors. It
executes1908 actual projected receiver/geometry queries across the three live
Chief/Master Guns/Cairo Fill definitions, admits up to eight dynamic objects,
and produces up to17 native layer commands per light.480 distinct native tick
samples have positive command counts. The other two named light definitions
remain covered by the explicit isolated portal fixture, not a claimed live-shot
submission. All submitted projected GPU draw counts are still zero in this
CPU integration diagnostic. Next work is original per-material implementation/
sort, raw geometry/material GPU packets and pStencil caster/receiver controls.
