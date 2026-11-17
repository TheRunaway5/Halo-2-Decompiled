# BCDF0 salted object-body lookup

The typed kernel now owns the exact UnitView TU1.5 production lookup and the
public object creation/storage API's DATA decision tree. Other source owners
remain separately qualified or unbound as recorded below. They validate a
salted datum, reject absent/out-of-range/empty/stale slots, test the caller's
type mask, and return the slot's body pointer. The frame owns lazy table reads,
the signed limit and salt comparisons, original partial registers, hardware
integer flags, both saved words and RET4. It executes no child calls.

Files:

- `include/halo2/native_tu15_object_body_kernel.hpp`
- `src/gameplay/objects/native_tu15_object_body_frame.hpp`
- `tests/native_tu15_object_body_frame_tests.cpp`
- `tools/validate_native_object_body_frame_tu15.py`

The unchanged English TU1.5 image is authenticated by SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Both sanitizer compilers pass 732 physical-original paths: 128 full returns
and 604 declared before-read/write prefixes. All 35 original sites are covered.
Each compiler compares 1,464 ordinary/hostile-callback executions and 9,052
complete pre-I/O GP/full-flags/XMM/CW/SW/MXCSR/live80 packets, ordered accesses,
stores and complete banks. The corpus includes signed limits/salts, wrapping
strides, all caller alignments, high type shifts and aliases with saved words
and table fields. Six compiled defects per compiler reject: unsigned limit,
unsigned salt, accepting salt zero, truncated shift count, wrong body field
and swapped saved registers.

The prefix results are declared instruction boundaries, not native SIGSEGV
envelopes. Store observers are absent in this architectural proof. Before and
memory callbacks may leave hostile host FP state; the frame restores it while
preserving the source ABI. Inputs cover dense x87 depths 0/1/4/8, varied TOP,
precision/rounding and MXCSR. Opaque/MMX, unmasked exceptional domains and
MapPack11 are not qualified.

Capstone omits access flags for the exact original `8554240C` TEST at BCE3D.
The private observer metadata authenticates that opcode and declares its
four-byte read. POP/RET implicit reads and PUSH stores are also explicit.
No original text, child outcome or return is supplied or patched. The earlier
JSON fixture compilation error and missing-TEST-read diagnostic are retained.

The read-only owner audit identifies ten source leads in five complete body
groups, including canonical BBCB0 routes. Eight have exact simple-dispatch
evidence; two remain explicit callback leads. Their image/resolver/public
ownership and fresh/prepared contexts still need individual authentication.
One lead, ModelRender, already has the older
`model_lighting_leaves_detail::bridge` default controlled by the PRT reference
setting. Its 41-line body includes that existing prefix. It is an existing
native contract to preserve and audit, rather than a newly unbound owner;
the other leads do not acquire its qualification by sharing the address.
The physical-frame proof alone does not establish a production prefix. The
later exact UnitView default is separately qualified below.

Accepted reports under `build/test-results`:

- `native-object-body-frame-mutants-20261110/report.json`: current expanded
  corpus and both compilers, with all six defects per compiler.
- `native-object-body-frame-prefixes-20261110/report.json`: earlier 736-path
  corpus; the later corpus adds an incoming zero-salt defect witness and has
  732 paths because its genuine alias paths perform fewer reads.
- `native-object-body-frame-private-r3-20261110/report.json`: initial whole
  original calls under both compilers.
- `native-object-body-owner-audit-20261110/report.json`: static source leads,
  not runtime or native-default acceptance.

All recorded completed probe owners are removed. Only bounded reports remain;
no binaries, raw banks or original-section copies are retained. Remaining work
includes exact source/fresh/prepared default routing, other image/FP domains
and live runtime coverage.

Source transport needs a separate check before binding: the shared retained
`State::push` decrements ESP before its provider preimage/read/write calls,
whereas the raw architectural frame commits ESP after a successful write.
Normal returned ABI can agree while provider failures expose different partial
ABI. The raw proof establishes physical instruction prefixes. A separate local
source-provider mode now matches the actual retained helper on six
preimage/write/observer failure cases per compiler, including published ESP.
It also witnesses all six differences from raw publication, without rewriting
the retained helper. This is the two-PUSH transport, not whole source binding.
No shared push implementation was changed.

The permanent private module's registered sanitizer/defect and Release checks
pass 2/2 in 94.25 seconds. Current owning C++ inputs also match the independent
Clang original/transport proof exactly. Closure
`native-object-body-frame-component-closure-20261110/report.json` records all
eight completed probe owners removed and the temporary coordinator retired.

The public `object_creation_request.cpp` owner now delegates its lookup to
`native_tu15_object_body_value.hpp`, using the same typed DATA tree. The exact
complete predecessor SHA-256 is
`bc66a1665f61efa13e1b575af9914a20cebc314f0ec25f196b50380369db90d1`;
the installed whole owner is
`b7b4160c324e3c46ec9cd74826a42afdda2f7877f4bcd4d397950c4a6262a18c`.
The public view preserves the original API's backed record-zero behavior,
unlike the CPU controller's zero-EDI failed-lookup sentinel. It also preserves
the initializer and all unrelated owner bytes. Registered 2/2 plus independent
installed Clang sanitizers pass 4,045 comparisons, 2,803 provider-failure
prefixes, 79,783 provider calls and 27 ordinary initializations per compiler,
including mutable table feedback, missing/wrong extent providers and a
compiled wrong-sentinel defect. Five owners are removed; bounded closure is
`native-object-body-value-closure-20261110/report.json`. This API reuse does
not establish native default routing for any of the ten generated source leads.

The separate private `SourceFrame` preserves the retained provider ABI's
arithmetic flag choices and live register feedback. A policy checkpoint after
each completed provider instruction can hand off to the exact untouched
retained suffix. It never restarts a completed read, PUSH or observer. The
first complete source proof passes 6,844 comparisons per GCC/Clang sanitizer
compiler, 732 provider/observer throw prefixes and 2,004 actual retained
continuations. Memory callbacks change live GP/flags, tracing, callable type
or instruction presence. Full pre-provider/observer packets compare within
each compiler; their totals differ (79,300 GCC and 79,352 Clang) because the
retained C++ operand evaluation order admits different mutable-provider paths.
The physical flags proof remains separate. This prototype only covers the
exact current prepared unit-view owner; fresh owners, complete source context
admission, opaque MMX, hostile host FP feedback, other images and runtime are
not accepted. The stronger callback-count/five-defect gate now passes both
sanitizer compilers, including all 35 retained sites, all 13 normal provider
sites and 256 explicit normal returns per compiler. The five rejected defects
are ignoring a returned policy change, publishing PUSH ESP late, corrupting
product OF, caching the datum across a provider and replaying the completed
rows read on handoff. An earlier proposed physical-undefined-flags change had
no observable difference in this corpus, so that run remains failed evidence;
it is not counted as a rejected defect. Both owners are removed. The permanent
source-transport sanitizer/defect and Release gates pass 2/2 in 88.35 seconds
after R25's freeze. All ten recorded owners are removed. The 32 included C++
headers match the immutable R25 copy; bounded closure is
`native-object-body-source-frame-closure-20261110/report.json`.
Earlier fixture compilation diagnostics remain retained without acceptance
credit.

Genuine UnitView regeneration now reproduces the exact current raw and
prepared lookup, resolver, dispatcher, LiveState, sign-extension helper,
public entry and public ownership tuples. All four whole C++ contexts pass
both compilers' syntax checks. A separate private candidate adds only its
guard/prefix and authenticated next-instruction labels; it reverses to each
exact predecessor and is idempotent. Eight original/candidate contexts pass
16 complete C++ syntax checks and 56 context-drift refusals. Both scratch
owners are removed. Reports are `native-object-body-generation-private-20261110`
and `native-object-body-candidate-generation-private-20261110` under
`build/test-results`. This structural proof was followed by actual prepared
route execution with both sanitizer compilers: 13,688 default and 13,688
positive-reference cases per compiler, 512 ordinary native calls with zero
retained statements, 4,008 provider-driven suffix handoffs and 24 untouched
entry refusals. Four owner defects reject per compiler: ignoring own reference,
ignoring returned policy, replaying the completed entry and ignoring the
cached literal witness.

The exact UnitView source, final generator-wrapper hook, specializer receipt,
standalone copied helper and catalog row are now installed. Current complete
source SHA-256 is
`b4d1bf0441e599e81840ab78caffd25d3b959442f74a77291a9c2b3a078e7cab`.
Removing only this overlay reconstructs the complete predecessor; all other
function and context bytes remain unchanged. Final registered acceptance
passes 9/9 in 168.92 seconds with no skips. Independent installed Clang and
the catalog's two positive/seven drift cases pass. All 13 final acceptance
and diagnostic owners are removed. Bounded closure:
`native-object-body-installed-closure-20261110/report.json`.

The first installed run caught a fixture comparing a removed overlay against
its still-bound input; two physical gates also skipped without an explicit
original-image path. These failures/skips are preserved. The final run fixes
only the fixture and supplies the authenticated image explicitly. No numerical
acceptance is inferred from the failed run. Remaining work includes the other
source owners, opaque MMX, other images, hostile host FP feedback and coherent
runtime qualification. The older ModelRender native route is preserved.

Before R26, the binder was strengthened to authenticate each continuation
label immediately before its exact next instruction. All 48 relocation
controls across the four bound contexts reject, as do the catalog's 12 label
relocations; 104 total context controls pass. The installed label positions
and every qualified C++ subject were already correct. Addendum:
`native-object-body-installed-closure-20261110/continuation-auth-correction.json`.

After R26's immutable freeze, a genuine retained `State::push`/`w()` probe
exposed the previously unqualified observer-object replacement case: a write
that starts without an observer must not notify one installed by that write.
The local SourceFrame now selects that branch on entry, preserving late
clearing/rebinding on the observed path. Shared Frame/State are unchanged.
Both sanitizer compilers pass 24 cases/18 real observer replacements; a
compiled restoration of the old behavior fails. The affected permanent
source/route/generation suite passes 7/7 in 193.33 seconds, with independent
Clang route/guard confirmation. All 12 recorded owners are removed. Closure:
`native-object-body-observer-correction-closure-20261110/report.json`.
R26 does not qualify this later frontier; the original measured failure is
preserved in `native-object-body-observer-branch-before-20261110/report.json`.

The object-material module now owns the second exact lookup default through
its distinct authenticated prefix/adapter and final generator hook. Current
whole source SHA-256 is
`385eb61256873fa208d01ddae7cd1cb56296c87c1d17a21c485d725b7573cc15`.
Its original D1878 caller reference is pinned at
`5055f0507759fba420c7df96ba2c0b72db67002c362e62dbe3c26f54b7ef7451`.
Current selected operands are individually authenticated as identity mappings;
the genuine fresh body omits those wrappers. Their complete projection is
exactly the already-qualified 35-site lookup, with four full owner tuples.
Eight original/candidate contexts pass16 whole C++ checks and104 drift controls.

Final registered3/3 in158.78 seconds plus independent Clang sanitizers pass
13,688 default/reference cases,512 ordinary zero-retained calls,4,008 exact
suffix handoffs,24 untouched refusals and four compiled owner defects per
compiler. Its catalog preserves the distinct owner, shifted include position
and rejects all12 label relocations. All nine recorded owners are removed,
with no large artifacts retained. Closure:
`native-object-material-lookup-closure-20261110/report.json`.
The public18B4C0 parent and remaining lookup owners are not qualified by this
helper port. It postdates R26 and has no coherent runtime qualification yet.

SceneDevice, SandboxSpecial and ParticleDraw now have exact native defaults.
Genuine generation and specialization authenticate 12 complete
current/fresh/raw/prepared contexts, with the same LiveState/sign-extension
contracts and an exact identity-only projection to the qualified lookup body.
The original contexts pass 24 whole C++ syntax checks; the original/candidate
matrix passes 48, plus 312 strict context/continuation-relocation refusals.
The six genuine prepared routes pass both sanitizer compilers, each with
41,064 comparisons per mode, 1,536 ordinary zero-retained calls, 12,024 exact
suffix handoffs, 24 untouched refusals and four compiled owner defects.
Registered sanitizer/release/generation acceptance passes 3/3 in333.08 seconds;
installed independent Clang and the three-owner catalog method also pass.

A copied-helper regression exposed an absolute-file-loading import failure.
The generic/material binders now load their explicitly copied shared helper
by file-relative path. No sys.path mutation, validator import or C++ change is
needed. The follow-up registered 3/3 passes in65.55 seconds, including all
12 operand tests and isolated two-file helper packages. Original failure and
sandboxed dependency-configure diagnostics remain preserved. Every recorded
completed owner is removed; no large artifacts are retained. Closure:
`native-object-lookup-copies-closure-20261110/report.json`.

These ports postdate R26. Complete parent/runtime closure and the remaining AI,
canonical DirectDamage/Vitality and VehicleEnvironment lookup owners remain
open. The pre-existing ModelRender native contract is preserved separately.

A later real-provider exception probe exposed handoff-type impersonation:
an arbitrary Memory/observer exception using the private continuation type
could be consumed as if it were the kernel's own request. Each invocation now
owns a distinct stack token and rethrows foreign continuations unchanged.
Both sanitizer compilers pass 128 focused cases/96 actual provider exceptions,
including null and nonnull foreign identities at preimage/write/observer
boundaries. A compiled removal of the identity check fails. All 12 affected
registered source/observer/default-route checks pass in 743.78 seconds; the
three recorded test owners are removed. Closure:
`native-object-body-foreign-closure-20261110/report.json`.

Canonical DirectDamage/Vitality defaults are now installed. Their actual
35-site TU1.5 body maps exactly to the canonical BBCB0 observation contract;
both original executable byte streams/complete CFGs and eight current/fresh/
raw/prepared contexts are authenticated, with 16 whole-file syntax checks.
The image-dependent root choice is explicit: this qualification selects the
TU1.5 004ECBD0 branch; MapPack remains separate. Receipt:
`native-object-lookup-canonical-original-contexts-20261110/report.json`.

The private original/candidate canonical matrix also passes 32 whole C++ syntax
checks and 192 context/continuation-relocation refusals. Both sanitizer
compilers subsequently pass 27,376 default/reference comparisons each, 1,024
ordinary zero-retained calls, 8,016 exact handoffs, 24 untouched refusals and
four executed owner defects. A peer review identified two requirements before
installation: reject moving the exact bridge switch away from function
entry, and preserve direct instruction-callback streams when specialization is
disabled. The measured prefix-placement counterexample is retained at
`native-canonical-lookup-prefix-placement-peer-20261110/report.json`.
Prepared-route evidence does not establish raw callback parity. The qualified
per-format correction now uses explicit false/true template tags for direct raw
callbacks and prepared literals. Raw callback-bearing entry stays retained;
callback-free raw native execution hands off after actual callback installation.
Both sanitizer compilers pass 9,856 exact stream comparisons over 14 source
contexts. Uniform35 callback sites, exact function entry, wrong/mixed tags and
returned-byte specialization hashes are authenticated by the installed helpers.

Explicit actual/canonical cursor, store-event and handoff mapping passes the
eight affected registered checks in 439.37 seconds and independent Clang
sanitizers. The focused observer proof compares actual event instruction and
address fields across 48 cases/36 live observer replacements and rejects four
compiled mapping/observer defects. Clang's full source transport and five
defects also pass; every recorded owner is removed. Closure:
`native-object-body-site-map-closure-20261110/report.json`.

The first canonical route gate completed 27,376 comparisons but failed its
guard-defect applicability check: the wrapper had duplicated the shared guard
loop, leaving the mutated shared loop unused. That failed receipt is preserved
and its owner removed. The canonical wrapper now delegates to the same shared
guard/continuation loop with an explicit site policy. The private rerun passes
both sanitizer compilers and all four executed guard defects; the existing
owner's registered2/2 also passes. Closure:
`native-object-lookup-canonical-private-closure-20261110/report.json`.

Seven installed source owners and their current/fresh prepared routes pass the
final registered12/12 in524.36 seconds, with the complete build removed.
Independent installed canonical Clang and four catalog regressions also pass.
Insertion receipt: `native-lookup-format-canonical-insertion-20261110/report.json`.
The maintained actual-header format validator passes both sanitizer compilers,
including two executed runtime guard defects. The combined stray-prefix and
missing-include authentication correction passes the registered format 2/2
and four affected generation gates 4/4. Closure:
`native-object-lookup-format-closure-20261110/report.json`; its superseded
private prototypes are retired. The full catalog's sole failure (37/38) was
the older Boundary/Scene include-order assumption. Its exact known-sibling
correction now passes registered routes/generation/catalog 3/3, including all
38 catalog methods; the owned build is removed. Closure:
`native-simplex-boundary-scene-sibling-closure-20261110/report.json`.
Isolated copied canonical helper packages also pass the registered
operand-specialization regression, with its owned build removed.
These changes postdate R26; a new coherent runtime/native-only/Original-frame
qualification remains open.

The AI lookup is now the eighth installed simple/canonical owner. Four genuine
current/fresh raw/prepared contexts have correlated complete body, resolver,
dispatcher and public-entry pins; the complete State, sign-extension helper
and ownership method also remain pinned. Its private candidate passes sixteen
whole C++ syntax checks and 104 context/label refusals. Both sanitizer compilers
pass 13,688 default/reference comparisons per mode, 512 ordinary zero-retained
calls, 4,008 handoffs, 24 untouched refusals and four compiled guard defects.
Receipts: `native-object-lookup-ai-candidate-20261110/report.json` and
`native-object-lookup-ai-private-routes-20261110/report.json`.

The default and final owning generator hook are inserted with exact reversal
to all previous AI source bytes and 690 unrelated complete functions unchanged.
The expanded installed routing, generation, sixteen-context callback-format,
isolated-helper and catalog gates are running. This later addition postdates
R27; VehicleEnvironment's distinct MathState owner, legacy images and whole
runtime/native-only closure remain open.

VehicleEnvironment's distinct MathState owner now has a private raw callback
view over the same typed lookup tree. Unlike the prepared source view, it calls
the actual raw callback before all 35 original sites and consumes live flags
and registers afterward. It uses the retained source push/pop/read helpers,
including provider and observer effects on stack publication. The first GCC
sanitizer run passes 20,960 comparisons: 512 ordinary calls, 235,416 full
provider/observer packets, 270,368 full raw callback packets, 1,464 provider/
observer failure prefixes and 240 callback failure prefixes. Owned scratch is
removed; only bounded metadata remains in
`native-vehicle-object-lookup-raw-r3-20261110/report.json`. The subsequent both-compiler controls, exact owner/generator binding and public
proof are closed below; coherent runtime and other-image qualification remain
open.

The raw view's follow-up passes both sanitizer compilers with the same exact
counts and five rejected compiled defects each. Both owners are removed:
`native-vehicle-object-lookup-raw-both-20261110/report.json`. Its current and
genuine fresh generator outputs are byte-identical before the private prefix;
four whole C++ candidate syntax checks and sixteen strict refusals pass, with
exact reversal preserving all 25 other functions and the entire public API:
`native-vehicle-object-lookup-candidates-first-20261110/report.json`. The actual public API proof and corrected registered dependency-map comparison
are closed below; the initial Release false-drift diagnostic is retained.
This paragraph supplies candidate proof, separately from installed acceptance.

The corrected registered VehicleEnvironment raw/generation/Release gate now
passes 3/3 in 161.33 seconds; all three bounded inner reports are retained and
the owned build is removed. Private closure:
`native-vehicle-object-lookup-private-closure-20261110/report.json`.

AI's final installed frozen retry is closed with all eight combined AI/Edge,
operand, selector and full-catalog checks green in 2,207.58 seconds. This closes
the earlier cwd-reference repair without changing the AI kernel. The immutable
manifest is `a7c4589f159d535b38008fe01845f0d48d5ab907fac06a375d585405b23bf5c4`
and the complete source/build owner is removed. Report:
`native-solver-hull-edge-ai-installed-official-20261110/report.json`.

VehicleEnvironment's actual public proof is closed separately from the raw
view: both sanitizer baselines pass 21,664 comparisons/mode, and the later
finite controls reject four real public ABI/service-copy/FP statement defects,
exercise sixteen genuine pre-load refusals and admit 24 valid opaque MMX
returns/compiler/mode. Compiler-local exact callback equality is preserved;
GCC/Clang callback totals differ within the retained source's declared domain.
The exact private pointer hook and separate test revisions are documented in
`BCDF0_vehicle_public.md`; all owned trees are removed. Bounded public closure:
`native-vehicle-object-lookup-public-20261110/closure.json`.

The VehicleEnvironment default and final owning generator hook are now
inserted with exact reversal preserving all previous bytes and all 25 other
functions/public context. Installed current/fresh generation and its scoped
catalog test pass. Its final thirteen registered raw/public/original-owner/
catalog/selector cohort and independent Clang are closed in immutable manifest
`a571d4635a7ba847cfd24435edb27aa25301f103760498d972bb69b519bd1b6a`.
Reports: `native-vehicle-object-lookup-insertion-20261110/report.json` and
`native-vehicle-object-lookup-installed-final-20261110/report.json`. This is
still separate from a new coherent engine, all-images and native-only closure.

The installed cohort has twelve passes and one skip: the old original vehicle-
environment fixture `hardware-verified/u5-native.json` is absent. All six new
lookup tests, four original first-person regressions, full catalog and selector
pass; independent Clang passes. Full frozen source/test/aux hashes stay exact,
metadata totals1,109,503bytes and the entire owner is removed. Restoring the
historical fixture remains a separate unchecked regression requirement.
