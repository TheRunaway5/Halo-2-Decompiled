# Original customization model rendering

The preview is a distinct source rendering context. `2C560` installs the
authored model-widget camera and sets the UI rendering flags. The initial
`15D70 → 226D0` layer3 query returns implementation3, but it is not the final
choice. `4CBF0` then applies the collector's lighting classification at
`4CC70`: classification2 follows `4CD41` and changes the selection to
implementation2 before `4CE16` stores it. Independent layer1,2,7,8 and15
passes retain their own selections. Attached weapons retain classification3:
both the Chief's battle rifle and the Elite's plasma rifle finally bind
implementation3, while their parent bipeds bind implementation2.

`analysis/frontend/customization/original-preview-final-implementation-spartan.json`
retains the initial query, post-override selection and actual final `16F32`
binding at one UI-clock value. All four Chief parts select `3 → 2`, and the
bound object is appearance-preview Chief `E27F0010` (`E23400BF`). The actual
main shader's postprocess implementation pointer independently resolves to
index2. This supersedes the earlier `original-preview-shader.json` capture,
which stopped immediately after the intermediate query. The bounded unchanged
instruction oracle `tools/oracle_frontend_final_implementation.py` freezes all
40 combinations of eight collector classifications and five initial choices;
classification0..4 selects that same index and classification5..7 selects3.
These are source and live-draw selection checks, not a full framebuffer match.

`original-preview-final-implementation-elite-weapon.json` pairs the plasma
rifle's intermediate query, classification3 override and actual bound
implementation3. The battle rifle's actual bound implementation3 is retained
in `original-preview-shader-banks-spartan.json`; its later coupled-query
attempts did not complete and are not evidence. The Elite shader-bank capture
independently retains its classification2 body and classification3 weapon
passes. The production material policy follows those per-object classifications.

`original-preview-shader-banks-spartan.json` also retains each distinct final
bound pass's pixel program, vertex attribute and constant banks, selected
implementation, authored postprocess blocks and actual object context. This
includes Chief armor, visor, arms, weapon and emblem passes. The examined
armor passes have no postprocess+64/+72 records. Unused values in the retained
vertex banks can be inherited from previous passes and must not be treated as
new lighting inputs without checking the selected program's reads.

The original shader-detail calculation consumes the object rendering sphere,
not fitted mesh bounds. The frontend uses `BDEF0`'s object definition radius and
bounding offset with its current posed node zero. Both retained Spartan and
Elite reference spheres match all eight float words in
`original-preview-spheres.tsv`. Whole-owner coverage uses the enclosing UI view:
`2232A0` builds `2F800`'s null-observer camera and submits it through `2C8A0`.
`2B0DDC` prepares the object collectors before `2B0E12 → 2C560` installs the
authored model-widget camera. Attached weapons copy that owner's coverage.
The later widget camera supplies the draw projection and per-part viewport;
`2C560` explicitly forces the view modifier after `2F970`, while `2C490`
retains single-view coverage and shader-detail scales. Host presentation does
not resize the original canonical 640x480 enclosing view.

`tools/oracle_frontend_model_shader.py` executes the original default-camera
initializer and enclosing-view producer unchanged. Eight cases cover the
original standard/progressive and 4:3/widescreen video flags with cinematic
state off/on. The retained same-clock original capture
`original-preview-collector-prior-spartan.json` independently records the
producer, five collector byte stores and later per-part metrics. Its enclosing
camera produces coverage `0x419234cc`, byte4; using the later widget camera
instead produces byte101 and wrongly selects the armor's highest shader bank.
The frontend helper now preserves this ordering. Its focused test checks the
eight producer outputs, five captured stores and 32 original shader choices
through resident armor, visor, emblem and battle-rifle material selection.

Geometry detail uses a separate distance from that coverage value. The actual
`1337DF → 4B5D0 → 4BCC0` caller passes the x87 Euclidean distance from the
owner's cached sphere center to the enclosing camera's origin. `133E30` copies
the produced owner record, including that distance, before collecting an
attached weapon. The child does not substitute its own sphere or the later
widget camera. Mainmenu's literal scenario type2 enables `2F970`'s view
modifier; `4BE17` multiplies the distance by literal `0x3e99999a` before the
authored `4C450` thresholds. Object `10A` bit2 separately multiplies by literal
`0x3fd9999a` (1.7). Visible widget bipeds are forced to geometry level5 through
`2B0C08 → BBF40(true)`, before the auxiliary-scale branch. Attached BR and
plasma-rifle objects have neither force-highest nor that auxiliary bit.

`tools/oracle_frontend_model_geometry.py` executes the original owner caller
through its real stack allocator, the child record copy, `BBF40`, metric
branches, authored threshold selection and resident section selector.
`frontend-model-geometry-original.json` retains all ten captured owner/header
inputs: initial and retained choices select level5 for each visible body and
level0 for each held weapon. Sixteen separate branch cases cover the view
modifier, auxiliary scale and force-highest ordering. The captured BR collector
section0 uniquely maps to geometry level0; the captured plasma-rifle section0
allows levels0 or1, so its level0 conclusion additionally depends on the
executed distance producer. Explicit active-permutation0 is a section-selector
seam in this fixture, separate from the body's active-region selection.

The native frontend now loads held weapon regions at the source-selected
geometry level instead of the asset preview loader's highest level. It derives
the initial owner sphere from the verified startup primary and retains the
chosen level; later pose updates check it against the actual current owner
sphere. The focused geometry test also traverses every startup, combat and UI
primary frame at all ten authored scenario roots. Weapon shader-detail and
lighting selection remain independent of geometry detail. These checks do
not establish complete weapon framebuffer or animation-effect parity.

The retained Elite orange-highlight discrepancy exposed a separate object-input
boundary. The actual plasma-rifle base programs already matched their captured
program and consumed texture-constant banks. The Elite arms shader `E325017A`
instead had a black layer7 color: native template pass18 word10 was `00000000`,
while the original `16F32` bank had `00FFFFFF`. Its authored color overlay
consumes `shield_vitality` (`0F000559`), which original `BCC20` reads from the
biped's object+F0. The frontend formerly supplied0 for that input and animated
only the parent material, omitting separately submitted child layers.

This fraction is recovered from its producer rather than seeded from the live
capture. `D5B60` resolves bipd+38 to the first hlmt+60 damage-info record;
`B7B40` reads its maximum shield at+8C and initializes object+F0 to1 only when
that maximum is positive. Both actual menu species have maximum70. The unchanged
initializer and `BCC20` are executed by
[`oracle_frontend_model_shield.py`](../tools/oracle_frontend_model_shield.py):
all ten initialized fractions match the retained live objects, with twelve
maximum-value and seven input-fraction boundary cases. The native accessor reads
the same metadata, then the model-only animation helper updates the parent and
its selected environment/before/after passes at the same render time. Shared
child banks are copied before per-object writes. UI emblems, scenery and held
weapons retain their separate input contexts.

The regression compares all60 Elite arms child-program words with the actual
original bank at all four shader detail levels, including clock boundaries and
shared-pass isolation. The standalone check passes. The fresh production
captures under `analysis/frontend/customization/source-matched-customization`
restore the orange arms highlight in the Elite chooser preview. The retained
original and native animation phases differ, so these captures confirm the
visible correction without asserting identical silhouette pixels.
Unchanged inherited vertex-bank words feeding unused texture outputs are kept
separate from this proved color-input difference and are not fitted to captures.

The original mainmenu script also installs cinematic lighting. `BBFC0` converts
primary elevation/azimuth to the negative direction vector, copies it to the
shadow direction and writes literal float `0x3f4ccccd` to accuracy and shadow
opacity. `BC070` independently writes the secondary light; `2A2349` copies
ambient RGB. `BBF80` selects the bank through object flag bit24; `3DB00` returns
`[4DE2F4]+1C` for those preview bipeds. These commands are now dispatched from
the actual script arguments, not from fitted colors or hard-coded menu angles.

The startup script's `cinematic_start` also sets the interface flag consumed by
`1BEB0`'s shader-detail multipliers. The original opcode22A table entry resolves
to evaluator `2A9740`, which calls `13BFF0`; its write at `13C01E` sets
`[510C50]+5`. `tools/oracle_frontend_cinematic_view.py` follows that original
evaluator and the corresponding stop evaluator through their flag writes for
eight initial-state cases. It stops before unrelated audio/reset services.
The native frontend passes its actual script lifecycle state to the existing
source shader-detail helper instead of supplying an always-false flag.

`tools/oracle_frontend_cinematic_lighting.py` executes those original routines
unchanged. It freezes 402 cases with 8,040 output words, including nonzero
initial banks to check fields that must remain untouched. The native test is
bit exact for all of them. The live original preview independently confirms
the script's complete 20-float lighting bank. Actual classification2 armor
passes consume the original dynamic-lighting bindings. Other classifications
retain their distinct authored policies.

The held weapon receives its parent's cinematic lighting bank. Actual
`3DB00` input handles are retained separately for each visible biped and child
weapon in `original-preview-weapon-lighting-{spartan,elite}.json`. Each biped
returns the cinematic bank directly through its bit24 flag. Each weapon has
no own bit24 flag and follows `BAF80`'s parent chain to the lighting cache;
its returned 84 bytes are identical to the parent's bank. The unchanged
`4BA26 → 4BA36` copy writes all21 words to each actual object shader context at
+10. This proves the weapon material range must retain the parent's bank,
while each selected pass applies only its own authored extern remaps.

The Appearance children 49, 50, 52 and 233 use authored screen flags `0x25`.
Original `22F916` copies flag5 to screen+5F4, returned by `v27`. Their
channel3 `23029A → 234F8C` policy adds neither tint nor blur in the retained
mainmenu mode2. Appearance remains on channel5 and is submitted first.
`tools/oracle_frontend_appearance_composition.py` executes those original
instructions for all four literal child definitions, using the captured
Appearance/default channel headers and an explicitly supplied child policy
header. Its separate minimal-widget collector replay confirms the parent
list stays admitted whether channel3 is occupied or empty. The parent rows are submitted beneath the authored child panel; removing
those submissions would change the original collector policy.

The frontend preserves Appearance's scenario world filter when opening these
children and removes the guessed black dim for these routes. The composition
regression traverses all seven Appearance rows, retaining identical parent
widget vertices, world-filter admission and preview insertion before drawing
each child. The source channel-policy fixture is
`analysis/frontend/customization/appearance-composition-original.json`.

Appearance's world-color admission is now independently verified. The original
`13870` consumes the scenario screen-effect definitions, evaluates their named
functions through `134FC0 →134FE0`, and submits nonzero effects through
`2D140` at `13B52`. The literal mainmenu desaturation function has constant
output bounds1/1; its zero live state therefore still produces full strength.
Actual47 has render-view mode3, both offscreen gates0 and one full640x480
viewport. One source frame at UIclock145770 submits `E4380218` with inputs
`[1,0,0,0]`. Separately, actual `16F32` on47 binds shader `E4390219`,
implementation0, with all60 pixel-program words identical to the previously
verified title event8132 program. No screenshot colors were fitted.

`tools/oracle_frontend_appearance_world_filter.py` executes11 original admission
cases, including those exact captured47 gates and suppressed partial/offscreen
views. The native painter requests the decoded scenario filter for47 and its
retained49/50/52/233 children; both runtime and diagnostic sweep consume the same
flag. Their earlier full-screen blue alpha quad is removed. Title behavior is
preserved, while unverified menu contexts retain their prior policy. Evidence,
owned-reference records and precise limits are in
`analysis/frontend/customization/appearance-world-filter-audit.md`. The47
capture verifies admission and bound shader identity, not GPU output pixels
or a synchronized complete original/native frame.
The fresh production `retail_frontend` check passes in7.33seconds in
`analysis/frontend/customization/source-matched-tests.log`, covering filter
admission, all seven child routes and the retained parent composition. Full
aspect captures and shader/world input verification remain separately recorded.

These producer, pass-selection and integration checks do not establish a
synchronized full-frame pixel match. Original GPU rasterization, complete
weapon effects, rendering order, every nested route and all aspect-ratio
presentations require separate evidence. Native screenshots and validation
reports are retained under `analysis/frontend/customization` with their exact
binary hashes; screenshots of the original are separately labeled.
