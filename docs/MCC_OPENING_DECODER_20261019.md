# MCC opening model decoder and renderer validation, 2026-10-19

This follows [the renderer investigation](CAMERA_CUT_PERFORMANCE_20261019.md).
Runtime acceptance uses the launcher's selected MCC cache exclusively. Original
TU1.5 executable instructions remain the behavior reference; Original/Vista map
playback is not substituted for MCC opening acceptance.

## Reproduced failure and actual cause

The previous launcher-selected runtime
`279e6d53193a509d2e1479bc5b1c2a43a80e50122818bf9e1a668fd4d8832cb4`
fails the actual MCC `00a_introduction.map` before rendering:

```text
MCC model vertex projection requires verified rigid or one-influence rigid-boned geometry
```

The rejected source owner is
`scenarios\objects\covenant\residential\holo_panels\holo_hallway_signs\holo_hallway_signs`,
datum 3799777542, section 0. It has geometry class 2, 164 vertices, three model
nodes, zero opaque influences and one transparent influence. The loader passed
only the opaque count to the verified rigid-boned layout adapter, incorrectly
presenting the valid transparent-only section as having no influence.

For rigid-boned sections the builder now passes the maximum stored opaque/transparent width,
as its separate skinned path and CPU model reader already do. The layout's
one-influence, typed declaration, stride, extent, hierarchy and index guards
are unchanged. The fix does not fabricate a palette, normalize weights,
resample positions/UVs, alter node maps, change parts or substitute a mesh.

The read-only diagnostic uses the actual Cache -> section builder -> layout
call chain. A first debugger probe omitted the hidden aggregate return pointer;
its invalid result is explicitly marked in `probe-abi-initial.json`. Disassembly
establishes RSI/RDX/RCX/R8 as the four scalar input registers, and the corrected
probe captures the tuple above (`rejected-section.json`).

## Original-code and regression evidence

The original TU1.5 executable is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Fresh execution of unchanged `1CA80` and complete `1CB00`, stopping before the
state consumer, passes 27 cases covering the observed (opaque=0, transparent=1)
tuple and its ordinary/mixed controls across nine part types. The original
selects rigid-boned class 2 in every case. No instructions or callee results
are replaced. This establishes class selection, not physical Xbox pixels.

The new `mcc_opening_transparent_rigid_model` regression loads that exact named
model from the actual MCC introduction cache and invokes the real section
builder. Before the fix it fails with the same decoder error. It also checks
literal resource ownership, positions, UVs, indices, node bytes and raw header,
allowing only the already verified declaration/basis representation projection.
Direct and projected indexed-range consumers must agree.

Evidence is under `analysis/mcc-opening-decoder-20261019`: `before/`,
`regression-before/`, `original-selector-proof.json`, `mcc-inputs.json` and the
frozen candidate source/command inventory. The failed regression's private
build was removed. The selected MCC cache's eight required map/data inputs
pass the prepared-cache provenance/hash check; its introduction map is SHA-256
`f6916833eedc5efba6250ff2bfd0d0547318572901776069ca043c0c8b4329b8`.

## Two subsequent startup ownership failures

After the model fix, the actual MCC opening exposed a scoped metadata error:
`Source tag pointer escapes explicitly owned payloads`. A read-only debugger
pinpointed a 10-byte LOD read at `0x5274c`. The named sky template had moved to
its canonical allocation while other MCC templates still shared that LOD in
its explicitly retained old allocation. The old and canonical LODs have the
same original-contract flags, layer mask and threshold bits; this is not a
schema conversion or a reason to invent default LODs.

`SourceTagMetadataStore` now includes selected groups' typed retained allocations
in bounded pointer resolution. Canonical datum definitions, table-slot addresses
and tag counts are unchanged. Unresolved definitions remain excluded. The
actual MCC regression failed on the old reader, then passed with literal-byte
comparison, canonical identity checks, cross-extent/overflow rejection and group
isolation controls. There is no cache-wide fallback on a scoped read miss.
Evidence: `tag-pointer.json`, `tag-pointer-owners.json`, `shader-owner.json`,
`candidate-v1/cmake-round-3/` (red) and `cmake-round-4/` (24/24 green).

The next real MCC startup passed geometry/texture loading but reached original
`C3300 -> C2A80` with missing backing at `0x44a6d8`. The native cold world lacked
five authored five-entry light-class fade tables (100 bytes). Their exact bytes
were read from the pinned TU1.5 executable (`original-light-fade-data.json`),
and the existing cold-world ownership test reproduced the same missing read.
They are now mounted as immutable original operands; no completed fades,
source instructions, light selection or light state are replaced. The canonical
root source amendment also carries the concurrent authored actor initialization
operand already present in the working tree. Other concurrent work is preserved.
Evidence: `boot-after-ownership/`, `light-storage-before/`, `light-storage-after/`
and the frozen candidate round-5 inventory.

## Actual opening CPU bitmap format

The next frame preparation reached `3DAF0 -> D5610` and rejected a CPU bitmap
outside the previously admitted DXT5 constructor domain. Read-only full-salt
argument observation identifies the actual MCC `cov_rug` floor bitmap,
datum4023783004: DXT1 format14,512x128,flags515. Its preceding `cov_dark_purple`
bitmap is format16 and succeeds (`bitmap-request.json`).

Fresh execution of complete unchanged TU1.5 `D5610`, including the complete
`1D2D0/407C00` CPU metadata constructor, passes32 samples of the rug's actual
encoded bytes across ordinary and high-word caller flags. Every executed
instruction byte matches the pinned XBE; no instruction or callee return is
replaced. This validates the DXT1 input to the existing original CPU decoder,
without inferring GPU residency or framebuffer parity. The private resource
owner and inline gate now admit DXT1 alongside DXT5. All encoded bytes, common
metadata and canonical descriptor identity remain unchanged; no recompression
or substitute texture is used. Other unsupported formats still reject. Four additional literal constructor/format
operands are mounted with their original read-only or D3D writable permissions.
The first DXT1 native attempt reproduced missing backing at `0x40db3c`; the
complete original read census identifies all four required entries, without
mounting a surrounding executable image. An early regression reader tried to
read a whole sparse116-byte row; it now reads only actually committed fields.
Those intermediate failures remain recorded separately.

`mcc_opening_cpu_bitmap` reproduces the old gate with this actual MCC bitmap,
then compares32 complete native constructor/sample calls with the exact fresh
original RGB words. It also checks literal resource delivery, canonical metadata
preservation and actual constructor/stack completion. The expected words are a
small permanent regression fixture; encoded resources and executable probes
remain private disposable scratch. Evidence: `bitmap-before/`, `bitmap-after-original-operands/`,
`original-dxt1-proof.json` and the frozen candidate round7 inventory.

The first real rendered MCC frame then reached checkpoint export's independent
format whitelist. The new regression reproduces `CPU checkpoint source closure
lacks its format proof` after32 correct samples. DXT1 is now admitted to that
same strictly authenticated resource exclusion; exact native leases, page
protection, sparse commitments, bytes, table identities and source preimages
remain required. The regression checks copy-only export, exclusion of exactly
its three derived regions, acceptance of that complete filtered owner and
rejection of changed immutable bytes. No arbitrary address or name-based
checkpoint exemption is introduced. Evidence: `bitmap-checkpoint-before/`,
`bitmap-checkpoint-after/` and `boot-after-dxt1-operands/`.

## First-scene screen-vector binding

The next real MCC attempt renders3 frames and enters `x01_scene_01a`; at
source tick91, complete`1FD00` takes its authored default-vector branch and
finds no cold binding at`0x474c80`. The original executable binds it to
`0x44ab1c`, whose exact vector bits are`0,0x3f800000,0`. Both operands are
now mounted read-only without replacing any screen update. The existing
cold-world regression reproduces the missing read before the correction.
Evidence: `original-screen-color.json`, `screen-vector-before/`,
`screen-vector-after/` and `boot-after-checkpoint/`. A queued normal replay
of the known failing candidate was cancelled during startup and retained as
`opening-attempt8/`; it is not a passed playback.

The first animated-camera action subsequently reaches original`16F6ED`,
which requires a missing dispatch row at`0x16fd38`. The original camera module
has three bounded dispatch tables; one was already present. The remaining two
four-entry tables are now declared in their exact source order, with their
original unsigned selector bounds and branch bodies unchanged. The cold-world
regression checks all eight identities and immutable ownership. Evidence:
`original-camera-tables.json`, `camera-tables-before/`, `final-source-contracts/`
and `boot-after-screen-vector/`.

The first normal MCC prefix then renders746 frames through source tick242,
before an original lens-flare command needs`0x474c2c`. TU1.5 points that word
to a complete16-byte authored RGBA record at`0x44aad8`;17C84C skips alpha and
copies RGB. The exact original binding and four IEEE754 one words are now
owned read-only (`original-effect-vector.json`, `lens-record-before/`).
No flare is skipped and no runtime color result is supplied.

The frame trace amendment initially landed while the first main compiler was
already running. Its completed object had a newer timestamp but old contents;
the live trace correctly exposed that mismatch. The subsequent build forces a fresh compile
of the identical pinned main source and requires its camera/fade string markers
in the exported executable before final live acceptance. `main-recompile.json`
records the exact source/object timestamp and hash evidence. Earlier prefix
receipts with no camera/fade fields cannot establish those live properties.
The replay helper now also forwards nonzero native exit status after rendered
frames; intermediate receipts already retain their real native status/errors.

A concurrent campaign-table amendment entered the shared world source while
this investigation was freezing it. Its required18-word include was initially
missing from that frozen dependency closure. Static review also found shared
ownership with the already declared`210768` arithmetic table. All18 words match the pinned original executable
(`concurrent-campaign-table-input.json`). The include now runs after complete
earlier owners, allowing its existing immutable-word checker to verify shared
words. A read-only breakpoint independently pinpoints the immediate overlap
between the new RGBA record and its existing RGB owner at`0x44aadc`
(`overlap-owner.json`). Only the missing alpha word is now newly mounted;
the complete existing RGB owner is reused. The native cold-world and
bitmap tests gate the final live replay; failed intermediate bundles are not
launcher delivery candidates.

The following MCC run reaches the first scene transition at tick698 and
renders1369 frames. The inspected2560x1378 tick450 image is coherent and shows
no obvious stretched geometry, but the run fails during the original vehicle
constructor at`F1AB9`, reading absent`0x473e34`. The literal scalar is the
original writable`.data` word`0x3ecccccd`; it is not a supplied physics result.
A bounded census verifies all16 absolute scalar-float operands in the
already-authenticated vehicle body against the actual TU1.5 instruction memory
operands and executable bytes. Thirteen remain immutable; three authored`.data`
operands retain writable ownership. Existing immutable owners are checked and
reused. The cold-world test reproduces the related missing operand at`0x46a488`
before the correction and checks all16 exact initial words/access modes.
Evidence: `original-vehicle-render-operands.json`, `vehicle-operands-before-v3/`
and `visual-attempt14/`. A concurrent placement row amendment is also verified
against the pinned XBE and preserved (`concurrent-placement-input.json`).

The private build's idle controller is suspended before its administrative
one-hour timer. Its original`run_test.py` supervisor remains active and retains
its cancellation cleanup guarantee, including killing stopped children.
Sequential CMake continuation workers reuse that owned tree; the final cleanup
will terminate the idle owner and verify its scratch was removed. No large
private build is promoted to a retained artifact.

The next vehicle constructor step reads an authored double at`0x46a600`.
A second census covers every direct immutable`.rdata` read by the retained
vehicle instruction sites, including 64-bit FP operands:16 declared operands,
76 total read bytes. The two newly needed doubles are mounted as complete
eight-byte immutable owners; the pre-existing 1.0 double and scalar owners are
reused. The native test reproduces the missing 64-bit read, then compares all
exact operand bytes and immutable access. This remains a finite declared input
set, with no section/image mapping, pointer-miss fallback or supplied runtime
results. Evidence: `original-vehicle-read-operands.json`,
`vehicle-read-operands-before/` and combined round16 checks (27/27 green).

The next constructor branch needs the vehicle kind dispatch at`0xf2bfc`.
Its actual`F25D3` comparison and`F25D6` unsigned branch bound the index to0..6;
all seven literal table targets already have authenticated native branches.
The complete28-byte table is now owned read-only, without introducing an enum
clamp or skipping any body. The cold regression reproduces the missing table
before the fix. Evidence: `original-vehicle-dispatch.json`,
`vehicle-dispatch-before/` and `visual-attempt16/`.

The live MCC trace through this prefix already records746 adjacent held-source
camera pairs with increasing frame fractions;253 have distinct rendered poses.
This is positive native presentation evidence for this shot, not a complete
cutscene or performance comparison. Stationary shots need not move.

## Renderer checks

The combined candidate includes the existing full-resolution crossfade image,
camera interpolation and bounded physical-memory guard. The opt-in campaign
frame trace additionally records exact source/render camera vector bits and
crossfade capture/composite events. It copies the already completed camera
instead of querying or advancing source owners again. Normal gameplay creates
no trace file; the trace changes no source clock or render inputs.

Combined CMake, actual MCC opening playback, normal-clock camera/fade checks,
visual inspection, runtime delivery and storage verification are in progress.

## MCC constant distortion sampler projection

The first scene transition next reached actual particle preparation and failed
with `particle sampler animation LOD/layer missing`. A census of the actual MCC
introduction finds 130/134 particle material records already load; three
`particle_alpha_blend_distortion` users have a zero available-layer mask and no
cooked NV2A overlays, references, parameters or mappings. Their authored
`distortion_scale` remains the same finite constant5 as the original; their
`bump_map` references remain MCC assets. The fourth census failure is an unrelated
unrestored glass vertex program, not silently admitted by this change.

The material reader projects only this constant contract when both the selected
template and pass are authenticated retained native programs, the D3D bank has
the verified empty shape, and the two authored parameters are the expected types
without animation. It reconstructs the original two scalar curves (authored
scale and zero) and four ordered mappings, then calls the existing original
sampler producer. It never supplies a default scale or skips the external23
binding. The original repeated property6 write and untouched SDK24 lane are
preserved. Unsupported parameter types, animations and programs still reject.

The actual Cache material-call regression fails before the projection
(`candidate-v1/cmake-round-20/tests.log`). Offline original metadata reference
is recorded in `original-particle-data.json`; this is not Original-cache runtime
acceptance. Three fresh complete unchanged TU1.5 `18540` calls with different
prior sampler banks pass with261 untouched instruction addresses, no supplied
services and no instruction patches (`original-particle-sampler/report.json`).
They establish the ordered writes and retained lane, not original GPU pixels.

## Scene01b raw CPU bitmap constructor

After the sampler correction, the actual MCC scene01b requested canonical root
`0x904c85e0`: full-salt bitmap3802202411,
`scenarios\bitmaps\covenant\military\floors\rubber_trim_a_dif`,
format10,256x128,flags513. A complete MCC delivery census with the actual world
base0x90000000 identifies the root; an initial probe used the test fixture's
base0x90010000 and correctly found no exact match. The fixed-base comparison
also matches the previously observed `cov_dark_purple` root.

Complete original `D5610/1D2D0/407C00` now has64 exact samples over this actual
MCC XRGB resource and its paired ARGB bump resource, both caller flags, two
high-word controls and eight coordinates. All681 executed instruction sites
match the original XBE; there are no supplied callee returns or instruction
patches (`original-raw-bitmap-proof.json`). The SDK multisample comparison uses
explicit declared device/surface inputs and is not a GPU completion claim.

The existing private CPU bitmap owner now admits verified raw formats10/11
and checks each against its exact existing Cache preparation. Inline D5610
admission and checkpoint ownership include those same formats. Canonical
metadata and encoded bytes remain unchanged. Eighteen previously unowned
literal constructor/selector/bit-depth operands are mounted in five exact
regions, with original D3D flags writable and dispatch data immutable. All
already owned bytes are reused. Regression round22 fails at the original
format gate; later rounds test complete native samples, immutable canonical
pointers, strict checkpoint ownership and exact cold bytes/access.

Round23 exposed the old raw-owner restriction to mipcount0. These actual
scene01b raw images have eight additional mip levels and174,764 encoded bytes,
which were already the inputs to all64 original samples. The owner now requires
the exact complete encoded mip-chain extent and compares every decoded mip
against the existing Cache decoder. No mip is discarded or generated. Round24
checks literal delivery of the entire encoded chain in addition to the original
CPU sample words. The failed round23 remains recorded separately.

## Original contrail startup state

The replay then rendered scene01b through tick791, including both crossfade
capture/composite, before the actual contrail update read unowned working word
`0x505e88` at21942E. Source investigation found both absent cold destinations
and omitted original new-map callbacks. Simply mapping zeroes would leave the
contrail state incorrect.

The original callback table selects row24 `11E550 -> 219F90` for the noise bank
and row42 `219300` for two derived scalar words. The cold scalar bytes are zero;
the3340-byte noise destination is filled by the original code using the real
source RNG and already-owned authored direction table. The conversion operand
at46A2FC is the exact original float word47E7AAAC, not a guessed unit conversion.
Fresh complete execution of both callbacks passes212 unchanged instruction
sites with no supplied services or patches (`original-contrail-startup.json`).
The scalar and complete noise bytes plus final RNG word are test references;
none is installed as a completed result into the production world.

The native world now owns the two exact writable destinations and immutable
scale input, and invokes the existing full original new-map callbacks before
object/effect playback. The regression first checks that cold bytes are zero,
then executes both callbacks and compares their computed state with the fresh
original reference. Production continues to use its current source RNG seed.

A complete direct immutable-operand census of the retained contrail geometry
module finds17 operand shapes/76 bytes. Only eight bytes were unowned: the
original interpolation and quantization constants at46A168/46A170. The cold
probe fails before these additions (`contrail-inputs-before.json`); all other
literal operands are reused. No mutable working state is copied from a capture.
Round26 passes27/27 with the original callbacks, including all3340 computed
noise bytes and exact final RNG state; round27 additionally checks those two
immutable words and access modes.

## Model resource request fields at scene02

The next actual replay passes the contrail failure and reaches scene02 tick1012.
Original130980 then reads/increments missing4F2D78. Original data and direct
instructions establish two adjacent loader-zero writable words4F2D74/4F2D78
(`original-model-cache-fields.json`): the resource-request policy and failed
request counter. They now have an exact eight-byte owned destination. The
source still selects policy, reports resource failure and increments the
counter; readiness, success and fallback results are not fabricated.

Incremental round28 picked up a concurrently added physics-profile test header
outside this frozen build and failed compilation; it is not runtime acceptance.
The private test copy preserves the original cohort and every opening regression
while excluding only that new profile's isolated-owner block until its own new
module is available. The root tests and concurrent work are preserved unchanged;
`candidate-v1/world-test-isolation-29.json` records both source hashes. Round29
then reaches the actual intended red cold-ownership check for4F2D74. This avoids
claiming a missing-header build error as a successful regression reproduction.

Round30 passed the new cache-field check but then reached newer concurrently
added physical-interface assertions outside the old frozen world composition.
Instead of excluding those assertions, round31 brings the matching exact data
owners forward: five physical interfaces,104 bounded readonly branch tables
(3541 literal bytes), and26 weapon dispatch words. Fresh independent comparison
against the pinned original executable verifies every imported byte/word
(`concurrent-native-tables-input.json`). The branch owner rejects conflicts,
reuses already-owned bytes and mounts only holes; its full isolated-owner tests
remain enabled again. The private CMake amendment adds only that source owner;
root CMake and unrelated work remain intact. This supersedes the temporary
round29 test isolation and does not claim all new physics behavior as tested.

## Cinematic shadow request storage

The next replay passes the resource counter and enters scene02 lighting at
tick1050. Its real `render_lights_enable_cinematic_shadow` command reaches
complete30E10 and faults on its first list write at4C6894. Original30E35's
unsigned16-entry bound and30E40/30E43 twelve-byte indexing establish the exact
192-byte object/marker/radius bank ending immediately before the already-owned
4C6954 counter. The original image has zero bytes in this writable destination.
The native renderer declaration now owns that complete bank. Source30E10 still
inserts, updates, removes and applies capacity limits itself; no script is skipped
and no shadow-enable result is supplied.

The added regression uses the actual cold-world factory and complete source
routine to insert all16 entries, reject the17th, and remove/swap the last entry.
The existing240-plus complete-original control fixture is also included in the
focused CMake suite. Those CPU checks establish request ownership/control,
not full original shadow framebuffer parity.


## Geometry-cache frame and retirement lifecycle

The actual opening continued to report incomplete BSP publication at scene02.
A CPU-only integration loop with the actual MCC intro reproduces all77 BSP
section publications in a cold cache, but fails under the preceding six model
owners' pressure. The original capacity remains6,815,744bytes. The presentation
hook advanced the draw arena/frame number but never called the original
geometry-cache frame owner, leaving preceding-shot allocations perpetually
current and protected from eviction.

Fresh original direct-call evidence places complete130890 immediately before
texture-cache update in the render frame (`original-geometry-frame-callers.json`).
It advances the physical-cache epoch, resets eight priority limits, maintains
predictions and applies pressure policy, including rollover. Three complete
unchanged original executions cover null, normal and rollover banks and caller
ABI (`original-geometry-frame-proof.json`); no game services or instructions are
supplied or patched. Rollover produces epoch257, as the original does.

The live presentation hook now runs that complete owner. The permanent lowered
module includes13 complete entries/607 original instruction sites, including
actual retirement130DA0, busy130D20 and descriptor resource walks. Source GPU
busy/wait operations remain required if a nonzero native resource is reached;
no synthetic idle result, larger cache, forced readiness or altered protection
window is installed. Generation and exact original bytes are retained under
`geometry-frame-source/`.

The new actual-MCC integration test first failed because the epoch did not
advance (round34), then reached the previously absent retirement callback
(round36). Round37 passes29/29 focused checks, including the actual six-model
pressure, original eight-frame aging and all77 BSP resource publications.
Compilation round35 picked up an unrelated concurrently added CTF dispatcher;
the private frozen copy retains the existing campaign/Slayer composition while
root CTF work is preserved (`candidate-v1/world-cohort-boundary-36.json`). This
is a source-cohort boundary, not a bypass of a reached campaign instruction.


Round38 adds exact whole-bank/GPR/EFLAGS comparisons for the three original
frame cases and passes29/29. Its application replay reaches a missing native
pressure-policy byte48C754 at the first frame. The focused actual-MCC pressure
loop had loaded the original executable data bank; it now explicitly uses the
same native startup factory as the application. Round39 reproduces the actual
missing byte through that path. The original .data byte is01 and writable
(`original-geometry-pressure-flag.json`), so its native owner retains exactly
that initial value and access. It is not disabled to avoid pressure handling.
The full-bank executable remains only an offline instruction/data authority.


The corrected native-factory loop next reaches the complete original SDK busy
method4057F0, which unconditionally reads the device-pointer cell before its
zero-resource-fence branch. Round40 fails at that missing411568 cell. The pinned
original D3D section declares a writable null cell (`original-sdk-device-cell.json`).
The native owner now retains only those four cold bytes. It does not construct
a fake device or supply an idle/busy return: the whole original method consumes
the actual resource header and submitted-fence word. Resources with a nonzero
fence still require an actual device owner at the reached dereference. Round41
passes29/29, including the complete native-startup, actual-MCC pressure loop.

## Coherent typed MCC node-map projection

A concurrently implemented decoder correction is brought into the frozen
candidate as one coherent module, after reviewing its source and paired-header
evidence. MCC type104/channel1 node-map resources use a120-byte serialized header;
the original resident consumer uses112 bytes and destination100/channel1.
The metadata copy projects only that destination. The resource-I/O copy projects
only raw+6C/+70 count/unloaded-pointer fields, authenticating the complete actual
container and typed owner even for partial read windows. Authored nodes, all
other resource bytes, cache files and original allocation/fixup consumers stay
unchanged. Bounds, malformed headers, fragmented reads and conflicting aliases
are tested. Actual MCC Cairo checks compare all five paired original hinge
headers; the original map is an offline fixture authority, never a playback
substitute. The complete MCC intro metadata-delivery test remains required too.


Round42 passes33/33 focused checks. The coherent node-map regression verifies
906 actual MCC resources, five complete paired original headers and23,676,834
byte/ownership checks. The complete actual intro metadata delivery verifies
35,842 pointer fields and30,773,614 owned bytes (see the retained JSON for exact
counts); all previous camera/fade/material/reciprocal/cold/native pressure checks
remain enabled. Runtime SHA256 is
`92c79aa56bcc9c6a5211b228e13a7490ebe984b2308e1bab24d0f0f33111467a`.
Full opening acceptance remains a separate required run.


## MCC English dialogue facial-resource ownership

Round42's real replay passes the previous scene02 BSP and shadow blockers, then
stops at tick1114 in original18F510's mouth/lipsync resource request. Its English
extra-info31 has offset149,102,464 and size752, beyond the actual MCC intro's
40,729,728-byte decoded file. The reader correctly rejects it. The source owner
is local gestalt4027584150, used by
`sound\dialog\levels\01_spacestation\cinematic\x01_0030_der`; it is not a
model or particle section. Neither shared-map directory owns that full-salt
local gestalt. Source18F510/130980 and the failed bounds gate are unchanged.

Direct inspection finds the complete declared `blkh`/`blkf` block in the selected
MCC `sounds_en.dat`. Other available language/neutral/remastered containers at
that offset do not contain this block. Its SHA256 is
`05f5a96948f0632f0c84131cce9b236cababcb2356fdb3c2d00f47019000506b`.
An offline paired original intro lookup by the exact sound name yields all752
identical bytes, including encoded mouth/lipsync payloads. The original map is
only the comparison input; production reads only actual prepared MCC DAT bytes.
The bounded original fixture records the original input hash and complete block.

The native adapter plans only declared English extra-info resources, validates
exact owner/extent/block markers, retains the actual payload and owns zero sector
padding separately. Unique aligned file tokens begin beyond every normalized
cache allocation. Only the copied native geometry locator changes; every other
header field, resource descriptor and payload byte remains authoritative.
The generic raw-file bounds gate remains strict. Native original130980/22A680
still allocates, fixes up and publishes the real sections; no dialogue command,
facial update or source readiness result is skipped or supplied. Retained sectors
also contribute to the resource identity so changed English payloads cannot be
accepted as an old checkpoint binding. Paths remain local and actual resource
bytes are retained by their owner, rather than reopening changed files on reads.

Round43 reproduces the out-of-file request through the permanent native-startup
MCC geometry integration test. The regression now walks English facial sections
on successive presentation frames alongside the prior model/BSP pressure test;
it does not pin the entire dialogue library in one impossible source frame.


Round44 passes33/33 with the new actual-MCC facial owner. Round45 adds the
complete original-block/actual-DAT/sector/header regression and passes34/34.
The native integration publishes all67 facial sections through unchanged
130980/22A680, alongside six preceding model owners and77 BSP sections over90
complete original geometry frames. The payload regression compares all72,572
actual English DAT bytes, retained-copy isolation, malformed partial requests,
exact native header projection, and the full paired original752-byte block.
`candidate-v1/regression-summary.json` retains the exact test output.
The runtime is
`a3005bcac37f925f5fdd91dad75ff8352d218963d8069777a71adfea8f98c200`.


The next actual playback loads that facial block and reaches original18F7A0,
then stops on an undeclared immutable operand at46A468. Fresh complete-module
census covers all13 absolute readonly operands of the nine speech/facial owners
(52 literal bytes). All other words already have their correct readonly owners;
only the exact originalBE99999A (`-0.3`) lower speech-clock-delta bound is absent.
The source's matching+0.3 bound and both normalization constructors are already
present. The new cold-factory regression first fails at that exact word in
round46. The scalar-data owner now supplies the original four readonly bytes;
source clock clamps, facial curves, weights and downstream calls are unchanged.
`original-speech-operands.json` retains complete original consumer PCs/bytes.


Round47 passes35/35, including the existing compressed local/shared resource
ownership, pinned-file replacement and bounds contract. Its real replay gets
through18F7A0 and reaches the unit's whole110350 facial fade phase. This routine
has no callees; its complete additional readonly census adds four distinct
operands, bringing speech/phase coverage to17 words/68 bytes. Only46A5D0 is
unowned. The exact original word is380A697B, used by ADDSS at1103B0. Its declared
readonly scalar owner preserves the source fade calculation. The two original
speech/facial normalization initializers already execute in native startup.


Round49 passes35/35 and actual playback reaches scene05 tick1943. WholeBF640
then faults on its real REP MOVSD destination4E57E8. This is a writable render
pose matrix scratch, not a missing model or an output to fabricate. Original
BF835/BF851 copy the same node count as32-byte orientations and52-byte matrices;
BF83D's4E8BC0 orientation bank already has its255-node owner. The original render
transform declaration also has255 entries. The corresponding13,260-byte matrix
bank at4E57E8 is loader-zero and ends twelve alignment bytes before that separate
orientation bank (`original-model-pose-scratch.json`). Only that bounded writable
scratch is added. Original pose producers, copy instructions and facial/render
overlays still compute and publish its contents; no captured matrix is mounted.


Round50 reproduces the exact missing matrix bank via the native cold-factory
regression. Subsequent complete-opening review also enables Vulkan core and
synchronization validation. These readback/validation runs establish functional
and visual diagnostics; their timings are excluded from clean performance claims.


Round51 passes35/35 with the complete native matrix scratch,17 immutable
speech/facial-phase operands, actual67 facial publishers and all prior MCC
model/BSP/camera/fade/GPU/memory regressions. Its runtime is
`288e822543274b8a004c55add0577ef0395e415a64dd951ed7e955aafbf6f27d`.
