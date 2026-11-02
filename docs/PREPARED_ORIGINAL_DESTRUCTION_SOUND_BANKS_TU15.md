# Prepared original destruction sound banks

`PreparedOriginalSoundBank` V3 retains the authored input needed by a native
sound-resource binder. It covers all31 named destruction sound banks in the
authenticated inventory, including the seven domains that V2 could not retain.
It does not allocate live sound tags, variation state, promotion clocks or voices.

The native C++ API exposes a vector of `Metadata::pitch_ranges`. Each range
owns its literal12-byte record, interval10-byte record, original import
StringID, optional initial used bytes, and all permutations. Each permutation
retains its16-byte literal, original import StringID and every12-byte chunk
descriptor. Sector payloads and tails are kept exactly. The last chunk word
at+8 is preserved without assigning it an unverified meaning.

Absent used storage is represented by `null`, not fabricated zero bytes.
V3 accepts absence only where the recovered count/class guard does not read
that owner. The original single-permutation and class-bit guards remain part
of native variation execution; presence is retained even when that path does
not need to read it.

Glass and rocks each retain three pitch ranges and a real promotion owner:
the literal28-byte row,16-byte rules and4-byte initial counters. Glass's first
authored rule order selects ranges1/2/0; rocks selects2/0/1. No sorting or
replacement by a default pitch occurs. Original pointers and table indices
remain literal until a runtime binder allocates and rebases the actual owners.
In particular, promotion selectors are not replacement sound datums.

V1/V2 disk formats still load through the same native class. Their single pitch
is exposed as `pitch_ranges[0]`; V1's unavailable initial used state remains
unavailable. Consumers of the old flat metadata fields must use that range.
`encoded(pitch, permutation)` now addresses every retained range and preserves
its independent permutation bounds.

The manifest digest is supplied by the admitting asset owner. It must not be
computed from an untrusted manifest and then passed back as its expected pin.
These are original20-byte sound records, not MCC28-byte records. A future
binder must retain the proper format semantics; for example, a guard specific
to MCC remastered-only data cannot be applied to raw original flags.

`tools/prepare_original_destruction_sound_banks_tu15.py` derives the V3 data
from authenticated original maps and resource files, checking every encoded
permutation against the independent named-payload audit. It requires a new
destination within `HALO2_TEST_SCRATCH`. Identical generated payloads use owned
hardlinks; original assets are only read. No cache or runtime selection changes.

`tools/qualify_prepared_destruction_sound_banks_tu15.py` runs the native loader
and production decoder through the official CMake test runner. Run both helpers
within one `tools/run_test.py` owner, so the preparation catalog remains alive
for qualification. Source snapshots, generated banks, controls, compiler outputs
and decoded data are disposable. Only bounded reports and hashes remain.

The [qualified cohort](../build/test-results/vehicle-destruction-original-banks-20261101/qualification.json)
covers908 map/bank contexts across38 original maps, exactly matching the
independent shape audit. Per compiler, it compares65,403 literal/payload slices,
9,450 permutations and12,722 chunks, with1,968 bound refusals. All273 unique
original clips match the prior same-PC PCM hashes, including21 stereo clips.
Both GCC and Clang reject18 malformed inputs and five fidelity mutations.
V1/V2 loading and the existing legacy C++ consumer also pass both compilers.

The temporary bank payloads total11,777,536 distinct bytes. They are not
published assets; cleanup removes them with their owner. The
[cleanup receipt](../build/test-results/vehicle-destruction-original-banks-20261101/owner-closure.json)
records removal. No new large reference is retained.

Runtime binding, original admission/permutation timing, combined loops,
normal gameplay/device output and Xbox waveform equivalence remain open.
All37 full-definition gates and D01-D13 remain part of the active goal.

## Owned native tables and resources

V3 exports now also retain the authenticated original map digest, full salted
sound/gestalt/class identities and all54 original92-byte class rows. The loader
checks the selected class against that complete owner. Earlier V3 inputs without
these proof fields remain readable for offline use, but cannot create native
runtime tables.

`OriginalSoundRuntimeBankTu15` builds an owned original-ABI table graph in a
caller-reserved32-bit metadata range. Bindings require exact resident native
`snd!` identities and names, one original map namespace, and caller-owned native
StringID resolution with a matching reverse lookup. The builder retains genuine
table aliases, contiguous intervals, absent used storage, promotion rule order
and full class data. It rebases indices and real nested pointers while preserving
other literal bytes. Resource locators address its owned sector file; reads
require the exact admitted physical chunk descriptor and retain sector tails.

The graph exposes mutable memory for the original range/promotion owners. Its
reader follows the actual current UGH descriptors. Resource reads own admitted
physical descriptors independently of that mutable table selector. Class/table
arithmetic checks full extents before32-bit address calculation. The graph rejects
mixed original namespaces, salted-index aliases, incomplete provenance, foreign
StringIDs and out-of-owner reads.

`tools/qualify_original_sound_runtime_bank_tu15.py` resolves the actual38 MCC
namespaces from the authenticated destruction inventory. MCC contributes tag
identities and import StringIDs; original banks contribute behavior and audio.
The native test builds two independently based graphs and compares literal rows,
alias identities, native gain/pitch/permutation execution, encoded routes,
sectors and the273 prior same-PC PCM hashes. Promotion comparisons run the
existing native kernel on original raw rows and rebased rows with explicitly
declared clock inputs, comparing returns, selectors and every ordered store.
These checks measure graph equivalence, with the kernel's separate original-code
qualification retained as its authority.

Live publication must still join the actual World allocation/directory,
metadata epoch, resource service and continuation owners. The append path below
retains unrelated sound tables and classes. `MultiplayerAudio`
currently reads encoded resources through `Cache::load_sound`, so the new graph
alone does not change normal playback. That integration must carry original
metadata and resources together and preserve original flag semantics, admission,
timing and source-owned state. Device output, combined loops and exact Xbox
waveforms remain separate open acceptance gates.

## Appending after complete native tables

`OriginalSoundRuntimePrefixTu15` supplies an already admitted native table bank,
its complete54-row class bank and a strong owner for these staged views.
The live publisher must also retain the enclosing metadata allocation for
external nested pointers. `create` copies the eleven prefix tables without changing their
bytes or indices, then appends/rebases the original bank rows. Existing custom,
promotion and extra-info pointers keep their existing allocation owners.
Only the newly appended chunk descriptors can read this graph's original
resources; prefix chunks retain their external resource owner.

Every class selected by an original binding must equal its existing native row.
All six classes used by these31 banks pass this check across38 namespaces:
1,13,14,22,28 and37. The complete existing class bank remains intact, including
the six repurposed MCC slots. The graph retains its prefix lifetime even when
the caller releases its wrapper and views during StringID resolution.

The class authoring profile now includes16 exact raw serialized namespaces
covering these38 maps. These differ from Cache's relocated/cloned globals.
The [audit](../build/test-results/vehicle-destruction-sound-publication-20261101/serialized-class-contexts.json)
matches their complete class banks to already qualified bank digests and checks
each selected original class row. The existing48 word corrections and all49
prior contexts remain unchanged. Raw route names, full salted identities,
addresses, extents, descriptors and whole-bank hashes remain required.

The [append qualification](../build/test-results/vehicle-destruction-sound-publication-20261101/native-prefix-v5/report.json)
uses the production Source converter and qualified class projection on actual
MCC metadata, with declared identity logical relocation. It preserves1,323,550
prefix rows,16,808,174 bytes and the prefix indices used by109,261 sound
definitions across the38 map contexts. Both compilers also compare all908
original bindings,9,450 encoded routes and273 original PCM hashes. Malformed
prefix ownership, class, stride and ordinal-overflow inputs reject. This is
staged metadata proof; Cache-specific geometry/timing corrections, canonical
World publication and live playback remain separate integration work.

## Descriptor-carrying sound-cache reads

`SourceSoundCacheReadTu15` carries the actual physical descriptor, encoded
locator, raw size/flags word and rounded sector extent from the allocated
`21F4BD ->1A3B70` caller. A locator alone cannot identify an original bank:
independently owned banks can both legitimately use offset0. The native I/O
boundary now forwards the descriptor alongside those original inputs. Its
allocation/frame checks, real read, retained receipt and done-byte ordering
remain with the existing native synchronous policy.

`OriginalSoundRuntimeBankTu15::read_cache_resource` accepts an exact request
for one admitted original chunk. Locator, complete size/flags and whole-sector
extent must match that descriptor's immutable route. Prefix descriptors belong
to their existing resource owner. The reader leaves the source cache handle
at+8 to the actual allocator and emits no readiness or completion result.

The [native bank request qualification](../build/test-results/vehicle-destruction-sound-io-20261101/native-cache-requests-v1/report.json)
compares all12,722 original sector requests, rejects63,610 malformed/foreign
requests and retains all273 original PCM hashes with GCC and Clang. Its
12,722 equal-locator controls reject a descriptor from the independently based
graph even though the locator and flags match. Whole native-prefix preservation
and the [standalone bank regression](../build/test-results/vehicle-destruction-sound-io-20261101/standalone-cache-requests-v1/report.json)
remain qualified separately.

The [source boundary qualification](../build/test-results/vehicle-destruction-sound-io-20261101/source-cache-read-v2/report.json)
uses a genuine retained original allocated call. The47,616-byte resource and
done byte match the original observations, all nine failure controls pass, and
the forwarded descriptor fields match the original entry. Both real caller
translation units compile with each compiler. These are component and syntax
checks; full World linking/execution and Xbox worker scheduling remain open.

The current World still uses its existing resource owner. Installing original
banks requires canonical publication plus matching immutable resource identity,
timing/epoch and continuation ownership before this descriptor-aware route can
select them in normal gameplay. Actual Audio must also consume their original
metadata and encoded payload together. No live publication, voice, device or
complete destruction acceptance is inferred from these checks.

## Staged canonical original-sound lineage

`stage_mcc_original_sound_runtime_delivery_tu15` appends the original graph to
the complete native sound plan. It validates raw root preimages, full salted
targets, typed globals/Legacy class ownership, native allocation separation
and complete resident sound/gestalt/class root coverage before returning.
Every owning UGH root receives the combined eleven descriptors; only the bound
snd! roots receive original headers. Existing MCC chunk descriptors are rebased
to their new physical table locations while retaining their encoded routes.
Original chunks remain with their own retained bank and descriptor owner.

An opaque `MccOriginalSoundRuntimeDeliveryTu15` retains the copied parent plan,
bindings and immutable original graph. Its seal covers the complete supplied
native plan; replacing the lineage, rows, routes, patches or authored-flag owner
rejects. Caller flags are copied before resolver callbacks. The parent remains
available for independent timing/projection authentication. Its gestalt layouts
and MCC chunk list describe the preserved prefix; appended original routes are
owned by the graph. External nested allocations stay with the enclosing
canonical metadata publication and its live memory owner.

`MccRuntimeCacheDelivery::plan` has an explicit original-input overload. It uses
the same complete canonical metadata builder, root preimage checks, allocation
reservation and final admission as the existing path. It retains the original
owner with the metadata delivery and transfers the complete sound plan only
after admission. Timing admission now joins the retained parent as described
below. Variation/PCM consumers still explicitly reject this lineage pending
their joined metadata/resource admission. The normal product asset loader has
no original-bank catalog or automatic input path yet.

The [staged canonical qualification](../build/test-results/vehicle-destruction-sound-canonical-20261101/canonical-plan-v3/report.json)
uses actual raw MCC allocations through the production Source converter and
qualified class projection, with declared identity logical relocation. Across
38 contexts, both compilers check908 target bindings,969 replaced roots,
108,429 unchanged roots,460,082 rebased MCC chunk routes and9,450 original
encoded routes. Each compiler rejects494 malformed/altered-plan cases.
The broad v3 source snapshot records a concurrent edit to `frontend.hpp`;
both actual native compiler dependency closures exclude it. The
[consolidated qualification](../build/test-results/vehicle-destruction-sound-canonical-20261101/qualification.json)
checks the consumed sources separately and preserves the original broad result.
Existing prefix, original sector/PCM and native variation/promotion checks
retain their prior component scope. This raw Source cohort does not exercise
Cache-specific authored-flag authentication or live canonical allocation.

The [production consumer check](../build/test-results/vehicle-destruction-sound-canonical-20261101/canonical-consumers-v3/report.json)
compiles the World, replay, canonical producer, timing and variation-provider
translation units with both compilers and repeats the actual retained original
sound-cache read test. Compiler dependency receipts identify the precise frozen
source closure. Full linking, World admission, resource identity, current timing
epochs, restore/continuation, actual Audio metadata/payload joining and device
output remain required. Original wet-feedback format/time and Xbox waveform
equivalence remain open without original hardware measurements.
The [closure receipt](../build/test-results/vehicle-destruction-sound-canonical-20261101/owner-closure.json)
records removal of disposable banks, sources and builds. No new large reference
artifact or selected runtime is retained or promoted.

## Joined original timing admission

The timing builder recognizes the opaque original-sound lineage and first
rebuilds its retained parent through the existing complete projection. The
Cache overload keeps its flags/class/permutation/stored-time/geometry policy;
the generic Source overload keeps its existing metadata contract and explicit
unknown/custom diagnostics. A complete parent-plan digest must match before
the replacement set can be joined. A failed seal, changed parent projection,
foreign native names or failed original input rejects the entire join.

Existing unrelated binding checks and permutation references move to the
combined table addresses. All UGH and snd! root expectations are rebuilt from
the sealed final patches. Original replacements use their exact native headers,
stored signed milliseconds and authored U16 permutation times. Named identities
and actual original encoded bytes supply their payload, codec/rate/channel,
sample/class and packet facts. Original SampleSize must equal the actual encoded
extent consumed by the native duration caller. MCC owner statuses retain their
independent meaning; original admission never promotes a custom MCC owner.

The resulting plan uses the unchanged current-memory lease implementation.
Binding, encoded-resource authentication and duration lookup still require the
actual full-salt directory, complete roots, immutable inputs, exact caller and
current producer thread/epoch. Identical-byte rebinds invalidate old proofs;
range selector bytes4/5 remain mutable. No source clock, cache handle, readiness
or device state is created by this construction.

The World duration path selects original encoded bytes only when its sound
plan's lineage is the exact owner retained by canonical delivery. Existing
prefix bindings keep their Cache resource route. This is preparation for live
integration: immutable resource identity and descriptor I/O routing, the
variation/Audio PCM consumer, normal asset admission and continuation still
need the same joined owner before the original path can be enabled.

The [timing qualification](../build/test-results/vehicle-destruction-sound-timing-20261101/original-join-v3/report.json)
uses38 actual raw MCC namespaces, including literal serialized null slots and
native import names, plus all908 authenticated original bank bindings. With
GCC and Clang it checks9,450 original timing/resource routes,908 stored headers,
9,450 authored permutation times,38 execution transactions and380 refusal
controls. There are zero timing-module writes. Changed resources/native inputs,
foreign same-address owners, identical-byte rebinds, broken seals, changed raw
parents and foreign native names reject. Existing timing/lease/permutation
contract tests and class-profile generation also run through the real sources.

This raw Source cohort has zero admitted MCC prefix timing bindings. Positive
preservation/rebasing of Cache-aware prefix timing bindings and full live
canonical publication remain required qualification. The explicit component
memory fixture is separate from a World, allocator or SoundSystem. The
[production consumer check](../build/test-results/vehicle-destruction-sound-timing-20261101/production-consumers-v2/report.json)
compiles the actual World, replay, canonical producer, timing and variation
provider and repeats the retained original sound-cache I/O check with both
compilers. Full World linking/execution, normal admission, restoration, Audio,
split/LAN/device output and original waveform/wet-feedback equivalence remain
open. The [consolidated qualification](../build/test-results/vehicle-destruction-sound-timing-20261101/qualification.json)
retains source hashes and scope; the
[closure receipt](../build/test-results/vehicle-destruction-sound-timing-20261101/owner-closure.json)
records disposable bank/source/build removal with no new large artifact.

## Immutable original resource identity and typed reads

The sealed lineage now authenticates its native metadata layout, every original
binding's provenance/full identities, and all actual whole-sector bytes. Its
resource digest is independent of host pointers and donor filesystem paths.
An identical reconstructed publication has the same digest; a different logical
publication layout has a different digest and refuses the old descriptors.
Binding this digest to the existing resident digest preserves the serialized
`SourceRuntimeResources::Identity` shape while including original sound assets
in the identity already compared by checkpoints, recipients and transfers.

`SourceRuntimeResourcePreparation` has an explicit original-input overload.
It finishes canonical metadata, the sealed sound plan, all original timing
bindings/stored/permutation references, and the resource identity before
publishing its registry entry. The resource constructor is private to this
preparation. Both consumers receive the same retained original owner. While an
original preparation remains alive, consuming either owner twice rejects;
the registry retains the required-original state to prevent a silent ordinary
asset fallback after consumption. Ordinary preparation retains its existing
consumption behavior.

The World requires the resource plan to match canonical sound ownership before
publishing tag data. Duration resource proofs also require that same resource
owner. Both actual native sound-cache callers now use the typed
`SourceRuntimeResources::sound_cache` boundary. Original physical descriptors
select their exact retained sectors. Preserved prefix requests first validate
the admitted physical descriptor, locator, complete flags and rounded sector
extent, then retain the existing external read policy. A foreign or changed
request throws before delegation. This does not allocate a cache handle, mark
readiness, change the source receipt or bypass the real allocated caller.

The [identity/descriptor qualification](../build/test-results/vehicle-destruction-sound-resources-20261101/identity-descriptors-v2/report.json)
passes with GCC and Clang across38 real raw contexts:12,722 original whole-sector
reads,460,082 validated external prefix routes and342 refusal controls per
compiler. Identical independent owners retain their content digest; a second
canonical allocation changes the identity and refuses old descriptors. Existing
908 bindings,9,450 original timing routes,273 same-PC PCM comparisons, native
variation/promotion, timing leases and class-profile checks still pass. Prefix
classification reads zero external resource bytes in this cohort; native MCC
DAT backing remains separate required qualification.

The [six-consumer compile and original I/O check](../build/test-results/vehicle-destruction-sound-resources-20261101/production-resources-v3/report.json)
covers the World, replay, canonical producer, timing, variation provider and
resource/preparation implementation. The new Cache-aware preparation overload,
registry retry refusals and checkpoint identity extension still need positive
execution with an actual Cache/preparation/World cohort; these are syntax and
component results. Positive Cache-aware prefix timing rebasing, variation/Audio
metadata and PCM joining, continuation, product asset catalog/admission,
loops/split/LAN/device output and original waveform/wet-feedback equivalence
remain open. The [consolidated record](../build/test-results/vehicle-destruction-sound-resources-20261101/qualification.json)
retains source/dependency scope, and the
[closure receipt](../build/test-results/vehicle-destruction-sound-resources-20261101/owner-closure.json)
records disposable bank/source/build removal. No new large artifact or selected
runtime is retained or promoted.

## Actual Cache preparation and positive prefix timing, 2026-11-02

The [Cache qualification](../build/test-results/vehicle-destruction-sound-cache-20261102/qualification.json)
links the complete frozen `halo2_cache` library and executes the explicit original
preparation with actual Cache instances in all 38 prepared map contexts. Its
compiler dependency record covers 404 objects and 1,114 source dependencies.
The frozen source closure is authoritative for this result: concurrent edits to
`frontend.hpp` and `physics_contact_runtime.hpp` occurred after the freeze and
were not incorporated. Declared refresh receipts cover only the test driver and
its CMake registration; this phase changes no production behavior.

The [38-context matrix](../build/test-results/vehicle-destruction-sound-cache-20261102/cache-matrix-v2/report.json)
passes 908 original bindings, 9,450 encoded routes and 12,722 whole-sector reads
through the actual `SourceRuntimeResources::sound_cache` boundary. It positively
preserves and rebases 268,007 MCC timing bindings, 74,603 stored headers and
268,097 permutation-time references. Actual preparation registry consumption,
retry refusals, unchanged Cache bytes and the original contribution to the
resident resource identity all pass. The ordinary serialized identity shape
remains unchanged; checkpoint transfer and restore were not executed.

Each context also binds its actual canonical tag image through the existing
timing memory lease. A preserved Cache-loaded resource and an original-bank
resource pass encoded-resource authentication and timing lookup. Old prefix
addresses, mutated immutable inputs and identical-byte stale rebind proofs
reject. These read-only component fixtures use declared lookup entries and
selector globals; they execute no native caller or World. Together with the
preparation/resource controls, the matrix checks 342 refusals and performs zero
timing-module writes. The existing ordinary Cache timing conversion CTest passes
against the same full library.

The initial matrix passed 37 contexts. Its remaining failure came from inferring
the original context from the MCC filename: pinned `highplains2.map` content
uses MCC `highplains.map`. The driver now requires the independently pinned
original context explicitly. The corrected complete rerun passes all 38;
neither production admission nor map validation was weakened.

This closes the positive actual Cache/preparation and preserved-prefix timing
component gaps described above. Native MCC DAT backing, original variation/Audio
metadata and PCM joining, normal product asset catalog activation, continuation,
live World execution, loops, local/split/LAN output and original waveform and
wet-feedback equivalence remain open. Xbox hardware is unavailable; native work
continues without assigning an unmeasured wet-feedback format or time. The
[closure receipt](../build/test-results/vehicle-destruction-sound-cache-20261102/owner-closure.json)
records removal of the disposable banks, frozen source and complete build.
No new large artifact or launcher-selected runtime is retained or promoted.

## Native variation and Audio resource joining, 2026-11-02

The variation provider now accepts an original replacement only when its sealed
plan matches canonical delivery and every original-file classification matches
the retained bank. It keeps that immutable resource owner with its current
native memory lease. Metadata still comes from the actual canonical image;
full-salt identities, roots and immutable range/permutation associations remain
required. Literal empty tables retain their original descriptors. The tutorial
and Cairo contexts contain table 6 `{count: 0, address: 0}`; a positive-size
allocation check had incorrectly rejected them. Empty tables own no readable
interval, and the row reader still rejects every ordinal in an empty table.

The provider's encoded-resource operation validates metadata and the selected
chunk descriptors in one retained read transaction. Original sounds load their
exact retained encoded bytes; preserved sounds keep the actual Cache DAT loader.
The descriptor's locator and complete flags must match. The runtime handle at
`+8` remains source-owned and is ignored by this resource operation. Audio now
uses this operation for a bound native provider before decoding PCM. It grants
no source cache handle, readiness, source voice or timing result.

Audio restore preparation also compares the target and saved immutable original
resources before retaining saved PCM. The target needs its genuine current
native binding. A saved provider's old epoch may already be stale; the immutable
resource comparison does not read that old memory. Missing or different original
resources reject. An original-free target retains the existing original-free
metadata route. Actual original-bank checkpoint restoration remains unexecuted.

The [38-context matrix](../build/test-results/vehicle-destruction-sound-pcm-20261102/pcm-matrix-v4/report.json)
passes 1,060 exact original variation definitions and 9,450 encoded routes. The
real Audio/SDL owners submit 908 selected original PCM voices and check a finite
256-frame manual mix for each voice, using declared Audio RNG seed `0x5821b36d`.
Selected decoded PCM totals 125,088,399 frames. For 603 submissions the original
frame count differs from the old MCC route; the submitted extent matches the
selected original resource. This qualifies the resource-to-PCM path, not complete
playback, source event admission, physical audibility or Xbox waveform equality.
Preserved Cache encoded resources and packet boundaries also remain exact.

The matrix retains 12,722 original sector reads, positive preserved-prefix timing
coverage and current-memory timing lookups from the preceding phase. It checks
722 refusal controls, including descriptor changes, permutation remapping,
stale cached Audio and encoded reads, and changed original-resource compatibility.
An actual independent canonical allocation supplies the different-resource
control. Identical-resource fresh bindings accept a stale saved provider, and
changing only a runtime chunk handle preserves encoded loading. Source-memory
writes are zero. The tutorial/Cairo empty-table failure, measured diagnostic,
test reservation correction and declared refreshes remain in the reports.

The [consolidated qualification](../build/test-results/vehicle-destruction-sound-pcm-20261102/qualification.json)
covers a complete frozen Cache build and real Audio owner closure: 441 objects
and 1,547 source dependencies. Ordinary Audio owner/continuation checks also
pass. The World is compiled, but ELF section collection drops unused World
functions; this is not a complete World execution result. Concurrent checkout
changes are recorded separately and were not incorporated. The
[closure receipt](../build/test-results/vehicle-destruction-sound-pcm-20261102/owner-closure.json)
verifies removal of disposable sources, banks and builds, with no new large
artifact or selected-runtime promotion. Native MCC DAT reads through the
original worker, normal product asset activation, live World playback,
restoration with originals, loops, local/split/LAN/device output and original
waveform/wet-feedback equivalence remain open.

## Actual native World publication and restore, 2026-11-02

The [native World matrix](../build/test-results/vehicle-destruction-sound-world-20261102/world-matrix-v1/report.json)
passes all 38 admitted contexts using the complete native World and systems
libraries. Each case consumes actual original-bank preparation, initializes a
real `PhysicsLiveWorldTu15`, and configures the declared multiplayer session
variant, simulation mode 1, 30 Hz and seed `0x6a19d3e1`. This common session is
an integration fixture for the campaign and multiplayer map namespaces; it
does not certify complete campaign or multiplayer gameplay.

Across 908 original bindings, the test executes actual `213410` selection,
checks the original stored milliseconds, and runs `213690` through the genuine
`21F6B0` / `21F590` duration caller. Its read-only input observer records 946
authenticated native duration calls and checks the exact original encoded byte
count, rate, encoding and compression operands. It supplies no duration result.
The actual World provider also feeds 908 selected original PCM voices to the
real Audio/SDL manual mixer. Their submitted frame counts match the selected
original resource; each receives a finite 256-frame mix check. Full playback
completion and physical audibility are separate acceptance.

Every case captures and restores the native World checkpoint with its original
resource identity. An ordinary or changed resource identity rejects. A native
selection/duration branch before restore matches a fresh branch after restore.
Old provider, script access and cached Audio epochs reject; a fresh binding can
play again and teardown permanently retires that binding. The matrix checks
380 refusals, retains the same original resource owner across restore, and
leaves raw Cache bytes unchanged. This restores the native World bank with
quiescent Audio. It does not restore an already-playing Audio checkpoint or
mint an owning native publication scope.

The [preserved-native checks](../build/test-results/vehicle-destruction-sound-world-20261102/preserved-native-v1/report.json)
also pass the existing actual World metadata-to-Audio and authored selected-time
tests for Lockout. The [qualification](../build/test-results/vehicle-destruction-sound-world-20261102/qualification.json)
records the full link and execution result, with 1,844 compiled units, 3,545
frozen source dependencies, 215 generated dependencies and 228 third-party
source dependencies. The dependency manifests retain actual compiler inputs
in small pages. Eight concurrently changed checkout files are listed separately;
the result applies to the frozen cohort. This phase changes test infrastructure
and adds execution evidence; it changes no production behavior.

The [closure receipt](../build/test-results/vehicle-destruction-sound-world-20261102/owner-closure.json)
records removal of disposable source, build, banks and checkpoint scratch.
No new large artifact or selected runtime is retained or promoted. Actual
original-bank World publication and native-bank restoration now have execution
evidence. Product catalog activation, the owning factory/playing-Audio handoff,
native MCC DAT worker backing, source voice allocation, ordinary destruction
event-to-device flow, loops, local/split/LAN acceptance and original waveform
and wet-feedback equivalence remain open.

## Portable catalog admission and preparation, 2026-11-02

`PreparedOriginalSoundCatalogTu15` is now a production asset-admission module
in `halo2_cache`. Its externally pinned `catalog.json` uses schema
`H2_PREPARED_ORIGINAL_SOUND_CATALOG_TU15_V1`. Each context contains the exact
runtime filename, original map context, SHA-256 of the complete normalized
`Cache::source_runtime_cache_bytes()`, and relative bank directory/manifest
digest pairs. It does not use the exporter's absolute reference paths. Selection
requires the exact snapshot before checking the filename; unknown or mismatched
snapshots reject. Duplicate contexts, fields and bank directories also reject.

Binding loads authenticated V3 banks, requires their original map provenance
and complete sound-class owner, and resolves unique Classic sound tags with
full salts. Original pitch and permutation names join the actual owning
gestalt's native StringIDs. The resulting immutable input retains its binding
storage and banks even after the binding wrapper is dropped. It borrows its
exact Cache for equality only; this creates no World, memory epoch, native
cache handle, voice or device eligibility.

The new `SourceRuntimeResourcePreparation(cache, binding)` overload consumes
this admitted input through the existing atomic original metadata/resource
preparation. The [catalog matrix](../build/test-results/vehicle-destruction-sound-catalog-20261102/catalog-matrix-v1/report.json)
passes all 38 actual Cache contexts: 908 original bindings, all 9,450 encoded
routes, and 342 rejection controls. Each case compares actual catalog inputs
against independently admitted exporter inputs and executes canonical metadata
and resource preparation. Relocating its entire portable bundle preserves the
resource identity. Returned inputs retain valid storage and raw Cache bytes
remain unchanged. Controls cover a wrong external pin, snapshot, filename,
original context or bank pin, traversal, duplicated bank, symlinked bank and
foreign Cache owner. The [focused CTests](../build/test-results/vehicle-destruction-sound-catalog-20261102/catalog-v2/report.json)
also pass the existing MCC timing conversion regression.

The [qualification](../build/test-results/vehicle-destruction-sound-catalog-20261102/qualification.json)
and compiler dependency pages record 405 compiled objects, 1,118 frozen source
dependencies, two generated dependencies and one borrowed input. One unrelated
concurrently changed checkout file, `src/gameplay/objects/prop_sandbox_definition.cpp`,
was excluded from the frozen cohort. The [closure receipt](../build/test-results/vehicle-destruction-sound-catalog-20261102/owner-closure.json)
records removal of this phase's sources, banks, binaries and build tree. No new
large artifact or launcher-selected runtime is retained or promoted.

These tests produce declared per-context portable catalog fixtures from the
authenticated exporter inputs. They do not install a product bundle or activate
the ordinary map loader. Next work must produce and admit that bundle through
normal map/factory preparation, then qualify the owning factory and already
playing Audio continuation. Native MCC DAT worker backing, actual destruction
event-to-device flow, loops, local/split/LAN acceptance and original waveform or
wet-feedback equivalence remain open. The user has no Xbox hardware access;
native work continues without guessing the missing feedback format or timing.

## Produced bundle and normal Cache admission, 2026-11-02

`prepare_mcc_original_sound_catalog_tu15.py` now produces a private prepared
cache with the complete portable sound bundle. It retains the existing encoded
MCC maps and native libraries through distinct Linux FICLONE files, copies the
authenticated original banks into private inodes, and runs the native
`prepare_original_sound_catalog_context_tu15` producer for every context. That
producer constructs the actual Cache, records its normalized snapshot identity
and binds the exact original-bank set. This does not consume test-generated
context manifests as production inputs.

The [production receipt](../build/test-results/vehicle-destruction-sound-admission-20261102/production-v1/report.json)
passes all 38 contexts and 908 bindings. The new provenance pointer names
`original-sounds/catalog.json` with its exact size/digest and an explicit map
set. Each map entry agrees with the existing indexed scenario owner's complete
encoded input digest and size, and declares its original map context. Unknown,
resource, mismatched or path-aliased owners cannot inherit this bundle by
basename. The ordinary Cache constructor authenticates the pointer before
decoding, then binds the catalog only after canonical initialization. A declared
catalog failure rejects loading. The original source cache remains unchanged.

Default `SourceRuntimeResourcePreparation(cache)` now consumes that admitted
binding automatically. Direct `SourceRuntimeResources(cache)` constructs the
same complete metadata/timing plan and retains its exact paired resource owner.
Direct World construction consumes that metadata once at the required canonical
base/budget; countdown loading retains its existing registry handoff. Both
paths use the same preparation implementation and preserve resource identity.
This supplies immutable assets, not a native epoch, publication or voice token.

The [normal admission matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/admission-matrix-v1/report.json)
passes 38 actual candidate Cache contexts, 908 bindings, all 9,450 encoded
routes and 228 controls. It executes both default resource paths, compares
their equal resource identities with distinct paired owners, rejects cross-owner
metadata, wrong layouts and repeated consumption, and preserves raw Cache bytes.
The [prepared-owner CTest](../build/test-results/vehicle-destruction-sound-admission-20261102/build-v1/report.json)
passes the existing owner suite plus 15 sound-pointer refusals. The
[preserved catalog matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/preserved-catalog-matrix-v1/report.json)
also passes its existing 38 contexts, 9,450 routes and 342 refusals after the
shared preparation refactor. The [component qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/component-qualification.json)
records the 410-object compiler closure and its exact source dependencies.

Full native World validation of the newly admitted direct and countdown paths
is in progress in the same owned investigation. The
[active-build receipt](../build/test-results/vehicle-destruction-sound-admission-20261102/active-build.json)
records the live handle and disposable source/build/candidate owner. The updated
World test requires an identical pre-catalog Cache baseline, independently
checks original banks, then exercises actual selection, duration, manual PCM
and native checkpoint restore. These new World paths are not yet qualified.
The owning factory/playing-Audio handoff, native MCC DAT worker backing, source
voice allocation, ordinary destruction event-to-device flow, loops, local/
split/LAN acceptance and original waveform/wet-feedback equivalence remain open.
No product asset selection or launcher-selected runtime has changed.

Before the DAT-owner work below, the retained MCC-prefix worker gap was visible
in source:
`MccOriginalSoundRuntimeDeliveryTu15::read_cache_resource` validates a prefix
descriptor and returns `nullopt`; `SourceRuntimeResources::sound_cache` then
used `load`, whose backing was the resident/shared map snapshots (plus the
existing generated-resource policies). That path did not admit the neutral,
English or remastered DAT banks. Native prefix-worker qualification therefore
requires a typed, authenticated DAT resource owner and cannot follow merely
from successful Audio DAT decoding or original appended-sector reads.

## Direct/countdown World qualification and DAT owner, 2026-11-02

The [direct matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/world-direct-matrix-v1/report.json)
and [countdown matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/world-countdown-matrix-v1/report.json)
now pass all 38 produced-candidate contexts each. The same actual native test
initializes 76 Worlds and executes 1,816 original selections, stored-time
queries, duration queries and manual PCM voices. It observes 1,892 actual
duration callers and checks 760 refusals. Both paths retain resource identity
through native restore, reproduce native selection/duration branches, reject
old epochs and play from fresh bindings after restore. Each test uses the same
declared multiplayer variant/mode/rate/seed as the earlier fixture, including
campaign namespaces. Audio remains quiescent during native bank restore and
each manually submitted voice receives a finite 256-frame check. This does not
qualify complete gameplay or already-playing Audio continuation.

The [native qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/native-qualification.json)
records 1,858 compiled objects, 3,576 frozen source dependencies, 216 generated
dependencies and 228 third-party dependencies. Seven unrelated concurrent
checkout changes are recorded; none were incorporated into that cohort.
The current produced bundle/default loading paths now have native World
execution evidence. This result precedes the new DAT-owner implementation.

An independent [prefix census](../build/test-results/vehicle-destruction-sound-admission-20261102/prefix-dat-audit-v1/report.json)
uses the frozen actual Cache/canonical-delivery implementation across all 38
pre-catalog maps. All 460,086 retained chunk descriptors have one exact DAT
file owner, with no ambiguous or unreferenced descriptors. Of these, 346,396
have unaligned file offsets and 1,842 have high bits that overlap Xbox location
bits. Every actual rounded sector fits its selected DAT file. These observations
require full 32-bit MCC offsets, not location masking, aligned seeks or invented
padding. This is a metadata/extent census, not native worker execution. Its
separate owned compiler/probe scratch was removed; only bounded receipts remain.

`MccSoundDatResourcesTu15` now implements this data owner. It independently
checks the complete Cache-aware timing/metadata admission, derives file ownership
from the retained parent, captures private disk-backed DAT snapshots and checks
exact current descriptor/offset/flags/sector extents. Its immutable resource
identity includes full file bytes and source routes without host paths or native
placement. Original appended reads keep their original sealed owner. MCC-prefix
reads use the DAT owner; original cache allocation, handles and completion
receipts remain unchanged. Direct ordinary MCC Worlds also retain paired
metadata, and countdown preparation constructs the same pair once.

This new implementation is still under validation in the same owned build.
The updated map test compares every retained prefix route with independent
Cache audio and full DAT sector bytes, including real padding and high offsets.
The [DAT worker run](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-owner-v1/report.json)
and subsequent matrices must pass before this phase is qualified. Full actual
native-prefix scheduling/voice delivery, owning-factory and playing-Audio
continuation, ordinary destruction event-to-device flow, loops, local/split/LAN
acceptance and original waveform/wet-feedback equivalence remain open.

## Typed prefix DAT bytes and native regression qualification, 2026-11-02

The [DAT admission matrix](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-admission-matrix-v1/report.json)
passes all 38 contexts with 460,086 typed prefix routes and 11,538,438,144 bytes
of actual padded reads. Every encoded segment agrees with independent Cache
audio, and every complete padded sector agrees with the selected source DAT.
All 346,396 unaligned and 1,842 high offsets are covered. The matrix also
preserves 908 original bindings and 9,450 original encoded routes, checks 380
refusals and leaves Cache bytes unchanged. DAT snapshots now participate in the
same serialized resource identity without adding fields or host paths.

Both [direct](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-world-direct-matrix-v1/report.json)
and [countdown](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-world-countdown-matrix-v1/report.json)
native World matrices pass all 38 contexts again with the DAT owner enabled:
1,816 original selections, stored-time/duration queries and manual PCM voices,
1,892 observed duration callers, and 760 refusals across the two paths. Native
restore retains the expanded resource identity and deterministic branches;
stale consumers reject and fresh bindings play. The unchanged original
sound-cache I/O boundary regression also passes. This preserves the earlier
finite-mixing/quiescent-restore scope; it does not certify full voice scheduling.

The [DAT qualification](../build/test-results/vehicle-destruction-sound-admission-20261102/dat-qualification.json)
records 1,861 compiled objects, 3,579 frozen source dependencies, 216 generated
dependencies and 228 third-party dependencies on Linux/GCC. Eight unrelated
concurrent checkout differences are retained separately. macOS/Windows execution
of this new owner remains unqualified. The
[closure receipt](../build/test-results/vehicle-destruction-sound-admission-20261102/owner-closure.json)
records removal of disposable source, build, banks and candidate, with no new
large artifact or selected-runtime promotion.

The next required join is retained-prefix PCM. `MccSoundVariationProvider::encoded`
still uses `Cache::load_sound` for prefix payloads, and its restore preflight
compares original-bank identity rather than the complete DAT resource identity.
Those consumers must use the same pinned DAT owner before prefix metadata,
native I/O and PCM can be considered joined. Actual native-prefix scheduling,
cache readiness/source voice allocation, genuine owning-factory and playing-
Audio continuation, ordinary destruction/event-to-device flow, loops, local/
split/LAN acceptance and original waveform/wet-feedback fidelity remain open.

## Retained PCM and complete resource preflight, 2026-11-02

The [shared-parser matrix](../build/test-results/vehicle-destruction-prefix-pcm-20261102/prefix-pcm-matrix-v1/report.json)
passes all 38 contexts. It compares 425,146 retained-parent permutations and
460,086 encoded chunk reads with independent Cache file reads, including codec,
sample/channel format and decoder packet/reset boundaries. The same owner also
passes all 460,086 native sector routes, with 11,538,438,144 real padded bytes,
346,396 unaligned offsets and 1,842 high offsets. Original bindings/routes stay
908/9,450, Cache bytes stay unchanged, and 608 refusal controls pass.

The variation provider now retains the World's exact DAT snapshots. Cache has
one metadata/codec/packet parser with an explicit typed resource reader; an
empty reader rejects and a provided reader never falls back to files. The
native duration-query resource callback uses this same DAT owner. Original
replacements continue to use their sealed original bank. Audio restore
preflight compares the complete immutable `SourceRuntimeResources::Identity`,
while genuine native publication additionally requires that World's exact
retained DAT owner. A saved unbound native route has no immutable resource
proof and rejects.

The [preparation/PCM matrix](../build/test-results/vehicle-destruction-prefix-pcm-20261102/preparation-matrix-v1/report.json)
also passes all 38 contexts: 1,060 provider definitions, all 9,450 original
encoded routes, 908 manual PCM voices and 722 refusal controls. The decoded
original banks contain 125,088,399 frames; 603 selected packets have different
frame counts from their MCC counterparts.

Both [direct](../build/test-results/vehicle-destruction-prefix-pcm-20261102/world-direct-v2/report.json)
and [countdown](../build/test-results/vehicle-destruction-prefix-pcm-20261102/world-countdown-v2/report.json)
World matrices pass all 38 contexts. Across 76 actual Worlds, 1,816 original
selections/PCM voices and 1,892 original duration callers remain qualified,
with 760 refusals. An additional 76 observed retained-prefix duration callers
match the independently authenticated original arithmetic and replay after
restore. Actual MCC encoded bytes/codec and original arithmetic operands stay
separate: the recorded Delta Approach dialogue case has 11,485 MCC encoded
bytes and a 25,416-byte original timing reference.

The [owned-copy controls](../build/test-results/vehicle-destruction-prefix-pcm-20261102/retained-control-v2/report.json)
pass with all three DAT paths absent, a newly constructed provider and uncached
Audio. An actual encoded-byte overwrite changes the independent file reader
while leaving retained PCM and complete native sectors unchanged. Independent
equal-byte owners pass resource preflight but cannot borrow another World's
exact publication owner. Appending outside admitted sectors changes the actual
resource identity and refuses preflight. Stale saved epochs remain admissible
only with matching immutable resources; active stale consumers still refuse.
The distinct-inode clone is removed and original input owners remain unchanged.

The [qualification](../build/test-results/vehicle-destruction-prefix-pcm-20261102/qualification.json)
records 1,907 compiled objects, 3,592 frozen source dependencies, 217 generated
dependencies and 228 third-party dependencies on Linux/GCC. Seven unrelated
concurrent checkout changes remain separate. The narrow Audio closure links
the real resource implementation; its [regressions](../build/test-results/vehicle-destruction-prefix-pcm-20261102/regressions-v2/report.json)
pass without replacement symbols. macOS/Windows execution remains unqualified.

These results do not qualify source cache scheduling/readiness, native source
voices, the genuine owning factory or playing-Audio handoff, ordinary
destruction output, local/split/LAN/device acceptance or original waveform/
wet-feedback equivalence. All 37 complete-definition gates and D01-D13 remain
active; no selected runtime is promoted by this work.

The [closure receipt](../build/test-results/vehicle-destruction-prefix-pcm-20261102/owner-closure.json)
verifies removal of the disposable source, build, banks and candidate, plus
empty case/syntax scratch parents. No new large artifact is retained. The
required original before-state reference remains unchanged.

## Genuine playing-Audio route: archive prerequisite, 2026-11-02

The opt-in `source_common_cut_original_sound_mcc_http` fixture now admits an
exact produced sound bundle inside its disposable build owner. It preserves
the normal native shadow-lane admission order and uses the real prospective
common-cut factory. Its playing-voice helper is compiled to check original
encoded data, saved position/parameters/clock, fresh receipts, exact PCM/RNG
replay and native/timing/SDL publication. **None of those Audio cases executed.**

The [actual route](../build/test-results/vehicle-destruction-audio-takeover-20261102/native-common-audio-v2/evidence/original-common-cut-playing-audio.json)
rejects the complete paired native archive before any common cut or Audio
binding is published. Its unchanged network decoded limit is 100,663,296 bytes.
An [offline native comparison](../build/test-results/vehicle-destruction-audio-takeover-20261102/archive-budget-summary.json)
constructs twelve actual native players on each input: the unchanged pre-catalog
source serializes to 118,737,415 bytes, and the produced original-sound candidate
to 119,056,981 bytes. Both reject under the existing network policy. The larger
offline census image is diagnostic only; it is never offered as a network cut.
No archive limit, shadow admission guard or readiness requirement was changed.

The [qualification record](../build/test-results/vehicle-destruction-audio-takeover-20261102/qualification.json)
is explicitly failed for Audio/factory acceptance. Exact candidate admission
and three refusal controls pass. The failed route's compiler closure records
1,875 objects and 125 linked archives; seventeen concurrent checkout changes
remain outside that frozen cohort. The older peer fixture was restored exactly
to its initial content. The new CMake option is
`HALO2_ORIGINAL_SOUND_TAKEOVER_BUNDLE`, taking the current owned producer report;
it does not select or promote runtime assets.

Genuine factory/playing-Audio continuation remains open, alongside source
cache/voice scheduling, ordinary destruction output, loops, local/split/LAN
delivery and original waveform/wet-feedback equivalence. Xbox hardware remains
unavailable; the user's instruction is to continue native work. All 37 complete
definition gates and D01-D13 remain active. The
[closure receipt](../build/test-results/vehicle-destruction-audio-takeover-20261102/owner-closure.json)
records removal of the disposable source/build/banks/candidate; no new large
artifact is retained and the required original reference is preserved.

## Immutable client original-bank playback, 2026-11-02

The worldless client Audio path previously created MCC-only immutable metadata
and decoded its payloads through raw Cache reads. The
[pre-fix native test](../build/test-results/vehicle-destruction-client-sounds-20261102/baseline-red-v1/report.json)
rejects all 26 original Lockout bindings. `MccSoundMetadataPublication::create`
now joins the provenance-admitted original catalog through the existing
complete canonical publisher. Its encoded reader selects the same sealed
original bank; Audio uses that reader for immutable playback. Retained MCC
sounds keep their existing Cache resource/decoder policy. This path creates
no native World epoch and consumes no World preparation registry.

The [corrected Lockout and ordinary MCC tests](../build/test-results/vehicle-destruction-client-sounds-20261102/fixed-lockout-v1/report.json)
pass. The [full matrix](../build/test-results/vehicle-destruction-client-sounds-20261102/client-matrix-v1/report.json)
passes all 38 contexts: 908 bindings, 1,060 pitch definitions, all 9,450 original
encoded routes, 908 actual manual PCM voices and 190 refusals. Every voice also
passes playing-cursor restore, fresh receipt checks, exact PCM suffix replay
and physical-output-clock preservation through its existing immutable Audio
publication. These are worldless client component checks, separate from the
still-unqualified genuine native factory.

The [captured caller replay](../build/test-results/vehicle-destruction-client-sounds-20261102/actual-and-loops-v1/evidence/destruction-actual-audio.json)
passes all 561 original 160-byte requests in 30 cases with 1, 2 and 4 coincident
listeners. Across those listener cases, 162 PCM submissions use admitted
original banks, 750 retain MCC prefix resources and 771 authored remastered-only
routes remain silent. Selection and PCM remain equal across listener counts.
The captured requests, listener positions and auxiliary RNG are declared
component inputs; original event scheduling and device delivery are not proved.

The [loop comparison](../build/test-results/vehicle-destruction-client-sounds-20261102/loop-comparison.json)
remains red: original banks pass three of seven profiles. Metal-thick/hubcap
normal and alternate banks expose nine authored multi-chunk permutations and
require their queue/retirement owner. Covenant damage and gasoline-fire loops
require four-bin combined playback; these two also reject on the MCC baseline.
No guard was weakened. The
[original topology receipt](../build/test-results/vehicle-destruction-client-sounds-20261102/original-loop-owner-requirements.json)
pins the remaining queue inputs for native work.

The [qualification](../build/test-results/vehicle-destruction-client-sounds-20261102/qualification.json)
records 447 objects, 14 archives and five unrelated concurrent source changes
outside the frozen cohort. Its overall verdict remains failed because loop
and full sound acceptance are incomplete. All 37 complete-definition gates,
D01-D13, source cache/voice scheduling, genuine native handoff, live/local/LAN
delivery and original waveform/wet-feedback equivalence remain open. The
[closure receipt](../build/test-results/vehicle-destruction-client-sounds-20261102/owner-closure.json)
records removal of disposable source/build/banks/candidate. No new large
artifact is retained; existing selected runtimes remain unchanged by this work.

## Native stock packet CPU primitives, 2026-11-02

The normal native sound registry now includes complete TU1.5 `21F360` cache-data
reference/address resolution, `2B98D0` stock encoding0/1 packet construction and
`2B9950` accepted-submission bookkeeping. These preserve the saturating byte
reference, actual allocator/block lookup, low16 cache-token index, x86 shift
mask, signed three-bit submitted count, literal shared 72-byte cursor clamp,
six packet stores, queued/submitted bit separation and original return ABI.
They create no cache entry, readiness, source voice or device result.

The [direct original comparison](../build/test-results/vehicle-destruction-chunk-queue-20261102/packets-final-v4/evidence/native-sound-stream-packets.json)
passes 1,530 declared raw-ABI cases: 1,519 complete returns and 11 matching
storage-fault prefixes. It compares all general registers on return, caller
FP/SIMD state and 7,074 ordered observable writes. Callee dead-stack scratch is
excluded under the existing native-kernel contract. The original observer runs
all 80 instruction sites unchanged, including the real nested `21F360` call;
no callee or platform result is supplied. An independently owned repeat produces
the same fixture hash, including salted and unallocated `NONE` handle controls.

[Four real code mutations](../build/test-results/vehicle-destruction-chunk-queue-20261102/mutation-controls.json)
to cache-reference saturation, the tail clamp, queued-count preservation and
cache-token masking are rejected. The existing resource regression also passes
56,000 randomized comparisons with its lifted references. The
[qualification](../build/test-results/vehicle-destruction-chunk-queue-20261102/qualification.json)
pins 31 compiled objects, two archives, the source/operator inputs and exact
limits. It has no compiler-dependency drift from the current checkout.

This is CPU-owner progress, not successful multi-chunk playback. Actual enqueue,
accepted two-slot PCM delivery, completion/cache retirement and decoder-owned
source-byte/PCM boundaries remain required. The two multi-chunk and two combined
loop failures remain open; their guards are unchanged. All 37 complete-definition
gates, D01-D13, genuine native handoff, normal sound delivery and original
waveform/wet-feedback proof remain active. The
[closure receipt](../build/test-results/vehicle-destruction-chunk-queue-20261102/owner-closure.json)
verifies disposal of source/build/fixtures and preservation of the required
original reference. No large artifact or runtime promotion is added.

## Native stock chunk retirement CPU owners, 2026-11-02

The native sound registry now includes TU1.5 `2B82A0` queue retirement and
`2B99C0` stock chunk completion. Retirement decrements the separate submitted
and queued fields, debits the cache pin with the original byte wrap, compacts
the pointers through the integer forward path of `32A990`, clears freed slots
and updates the stream mode. The callback then re-reads the descriptor handle
and conditionally debits its data-reference byte. It releases on every status;
actual TU1.5 `227140` separately owns success-only pumping. The original x86 ABI
has its direction flag clear. No source voice, admission, cache allocation,
readiness, I/O completion or device result is supplied.

The [direct original qualification](../build/test-results/vehicle-destruction-chunk-retirement-20261102/qualification.json)
passes 1,302 declared raw-ABI cases: 962 returns and 340 matching storage faults,
with 16,538 ordered observable writes and 196 unchanged original instruction
sites, including the actual CRT copy. All general registers on return,
FP/SIMD preservation, every mapped native byte and fault address/extent/store
prefix agree. Callee dead-stack scratch is excluded. Coverage includes all
byte flags, four record alignments, success/flush/failure status arguments,
zero and saturated pin/reference bytes, salted/unallocated handles and a
callback handle changed by queue compaction. A separately executed original
repeat has the same fixture hash.

[Six compiled negative controls](../build/test-results/vehicle-destruction-chunk-retirement-20261102/mutation-controls.json)
reject an incorrect queued count, clamped pin underflow, incorrect reference
guard, clear-before-copy order, success-only release and stale callback handle.
The stale-handle control is ordered to fail on the actual alias case. Existing
packet checks pass all 1,530 cases; the sound-resource suite passes all 56,000
states, including 4,752 matching faults. The qualification retains compiler
inputs, objects/archive hashes and any concurrent checkout drift separately.

Actual multi-chunk loop playback is still unqualified: genuine enqueue,
accepted two-slot PCM delivery, decoder-owned byte/frame boundaries and the
completion pump must compose with these CPU owners. Native four-bin combined
output remains open. No source guard was weakened. All 37 complete-definition
gates and D01-D13, source voice admission, genuine handoff, normal local/LAN/
device delivery and original waveform/wet-feedback equivalence remain open.
The [closure receipt](../build/test-results/vehicle-destruction-chunk-retirement-20261102/owner-closure.json)
records disposable build/fixture removal and preservation of the required
original reference. No new large artifact or runtime promotion is added.

## Original chunk topology and emitted PCM boundaries, 2026-11-02

Both original-bank encoded loaders were dropping their authenticated resource
chunk extents. The focused pre-fix check reproduced this on all 9,450 routes
across 908 contexts. Prepared and runtime banks now publish those literal byte
extents in `EncodedSound::packet_chunks`. Original flags retain their original
meaning; they are not interpreted as MCC reset or decoded-byte fields.

`decode_sound` now records `PcmSound::chunk_boundaries`, pairing cumulative
encoded bytes with frames actually emitted by the continuous decoder/resampler.
It validates complete block-aligned coverage, keeps state across resource
boundaries and attaches final decoder/resampler flush frames to the last chunk.
A short chunk can emit zero frames. Inputs without declared chunks retain an
unavailable map. Container decoding does not acquire an invented byte/frame
map. MCC Opus records its existing per-chunk trimmed output, bounded again by
the permutation's final sample extent.

The [qualification](../build/test-results/vehicle-destruction-decoder-chunks-20261102/qualification.json)
checks all 9,450 routes through both loaders. All 273 distinct permutations,
including 55 with multiple chunks and 375 total chunks, preserve byte-identical
same-PC PCM and frame counts from the captured pre-fix decoder. This is a PC
decoder comparison, not an Xbox waveform measurement. The test declares its
native tag/StringID component owners; it does not construct a native World or
supply a source voice/cache/device result.

An independent known-PCM resampler comparison passes 24 rate/channel/endian
cases, with 54 zero-frame chunk witnesses, 90 non-proportional boundary
witnesses and 121 malformed-extent refusals. Existing non-silent MCC Opus
state/reset/padding/tail controls also pass. [Six compiled mutations](../build/test-results/vehicle-destruction-decoder-chunks-20261102/mutation-controls.json)
are rejected: dropped prepared or runtime extents, proportional frame estimates,
resampler reset at a chunk boundary, omitted final-flush ownership and omitted
MCC final trimming. Compiler closure pins 26 objects, one archive and linked
system libraries, with no current compiler-input drift.

The playback Bank still needs to retain and consume this map through its
actual two-slot queue, backpressure, completion and publication lifecycle.
Source multi-chunk guards remain intact. Combined four-bin playback, genuine
source voice/cache admission, normal local/LAN/device delivery, Xbox waveform/
wet feedback, all 37 complete-definition gates and D01-D13 remain open. The
[closure receipt](../build/test-results/vehicle-destruction-decoder-chunks-20261102/owner-closure.json)
records private build/bank disposal and the preserved required original
reference. No new large artifact is retained and no runtime is promoted.

## Selected-loop chunk output and material profiles, 2026-11-02

`MultiplayerAudio` now retains decoder frame boundaries in its Bank and the
unsubmitted remainder in each selected-loop voice. It fills the actual SDL
two-slot queue with owned PCM spans, advances only after accepted submission,
refills after actual successful consumption and retains per-chunk attenuation.
A new controller selection waits until the previous permutation's remaining
chunks are submitted and an actual slot is available. Checkpoints retain the
same immutable backing and next-chunk cursor; flush retires accepted packets
without restarting the remainder. Decoder-empty spans create no fake packet or
completion. The authentic vehicle cohort has no empty-span queue witness.

The original single-chunk capability remains truthful. A separate chunk-queue
capability admits a multi-chunk phase only when this real native output owner
has a retained decoder map. Other output ports keep their existing strict
default, and the four-bin combined-output refusal remains unchanged. No Xbox
cache handle, cache pin, source readiness, source voice or device result is
fabricated. Source CPU cache/scheduler composition remains a separate owner.

The [qualification](../build/test-results/vehicle-destruction-chunk-playback-20261102/qualification.json)
checks all 55 distinct authenticated multi-chunk permutations through real
`MultiplayerAudio` and manual SDL consumption: 54 Coagulation and 41 Sentinel
Walls context cases cover the complete distinct set. All 95 cases compare exact
PCM bits against uninterrupted playback, cross the first boundary before a
producer refill, update gain across queued/future chunks, restore the playing
unsubmitted remainder, preserve backpressure and flush without restart. These
are declared output/controller inputs, not captured source gameplay events.

[Six compiled Core mutations](../build/test-results/vehicle-destruction-chunk-playback-20261102/mutation-controls.json)
are rejected: one initial slot, absent refill, lost restored remainder, wrong
chunk offset, wrong chunk gain and pumping after flush. Existing Banshee
same-stream phase, source voice-ring, retaining-MCC Core enrollment and ownership
preflight checks also pass. The frozen compiler closure contains 457 objects
and 14 archives. Six unrelated physics/weapon compiler inputs changed in the
checkout during qualification; the report pins both versions and preserves them.

The [full loop comparison](../build/test-results/vehicle-destruction-chunk-playback-20261102/loop-comparison.json)
improves original profiles from three to five of seven. Metal-thick and hubcap
normal/alternate playback now pass; both alternate-only profiles pass. Tiny
Covenant damage and gasoline fire still require native four-bin combined
output. Genuine source admission, native handoff, local/LAN/device gameplay,
Xbox waveform/wet feedback, all 37 complete-definition gates and D01-D13 remain
open. The [closure receipt](../build/test-results/vehicle-destruction-chunk-playback-20261102/owner-closure.json)
records removal of the private build/banks/catalog clone and preservation of
the required original reference. No new large artifact or runtime promotion is
added.


## Native stock stream caller qualification, 2026-11-02

The [native stream qualification](../build/test-results/vehicle-destruction-native-stream-pump-20261102/qualification.json)
adds original TU1.5 `2B9A00`, `2B9980`, `2B8530` and `227140` semantic kernels.
It preserves packet availability, accepted/rejected bookkeeping and completion
retirement/refill, including stack bytes reused across the callback chain.
There are 1,367 unchanged-original comparisons, 24 actual manual SDL completion
events and nine rejected incorrect compiled variants. The 1,302-case retirement
fixture now checks original stack bytes; packet and resource regressions pass.

This uses a test-only native service ABI with an explicitly synthetic decoded
PCM mapping. It does not enroll a source voice or prove a retail SDK result,
production handoff, original waveform or Xbox wet-feedback behavior. The prior
55-permutation native output evidence and five-of-seven loop result remain
separate; the tiny Covenant damage and gasoline-fire combined four-bin profiles
are still open. No asset admission, runtime selection or complete destruction
sound-delivery gate is changed by this CPU qualification. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-pump-20261102/owner-closure.json)
records removal of temporary builds and preservation of the required original
reference.


## Stock enqueue/release CPU qualification, 2026-11-02

The [queue qualification](../build/test-results/vehicle-destruction-native-stream-queue-20261102/qualification.json)
adds native `2B8210`, `2B85F0` and the stock codec's `2E4D60` ret4. It preserves
cache-pin acquisition and tail-first release, signed count behavior, authored
mode transitions, wrapping byte arithmetic and independent data references.
The `21F260(-1, chunk, 4)` helper keeps its actual private frame writes.

All 5,579 unchanged-original cases pass, including 82 matching faults, complete
mapped-memory/stack checks and exact CPU-callee frames. Independent captures
match, and nine compiled incorrect variants are rejected. Packet, retirement,
resource and SDL-service regressions pass in the restored frozen build.
These records are raw ABI inputs, not source cache/voice admission. No resource
is declared ready and no device result is supplied. Production handoff, all
normal destruction sound delivery, the two combined four-bin profiles and
original waveform/wet-feedback proof remain open. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-queue-20261102/owner-closure.json)
records removal of temporary builds/fixtures and preservation of the required
original reference; no selected runtime changes.

## Authenticated native stream lifecycle, 2026-11-02

The [payload qualification](../build/test-results/vehicle-destruction-native-stream-payload-20261102/qualification.json)
uses a freshly exported, externally pinned original catalog: 908 bank contexts,
9,450 routes, 31 sound names and 273 distinct stock ADPCM permutations at
44.1 kHz. Actual native descriptor reads reproduce the authenticated encoded
bytes. The composed stock kernels send all 375 chunks through real manual SDL
acceptance, consumption, source callbacks and pin/data-reference release.
Rendered PCM matches uninterrupted same-PC decoding bit for bit, including all
55 multi-chunk permutations. The 1,147 scenarios cover successful playback,
callback-driven refill, flush, device removal and actual rejected submissions.

Completed PCM remains retained until source accounting returns. This corrects
the earlier test adapter's premature acknowledgment; all 1,367 unchanged-XBE
service comparisons pass with the corrected policy. Eight compiled incorrect
variants are rejected and all six restored regression suites pass. Unsubmitted
chunks have separate pin cleanup and receive no invented completion events.

Already-loaded cache rows, unity gain/rate and manual scheduling are explicit
component inputs. Source admission, seeked/zero-output packet mapping, original
event timing/selection/spatial behavior, production handoff, normal local/split/
LAN sound delivery and Xbox waveform/wet-feedback proof remain open. The two
combined four-bin profiles and all complete destruction gates remain open.
The [closure receipt](../build/test-results/vehicle-destruction-native-stream-payload-20261102/owner-closure.json)
records removal of the private catalog/build/source, preservation of the
required original reference and no new large artifact or runtime promotion.

## Native common stream-constructor integration, 2026-11-02

The [constructor qualification](../build/test-results/vehicle-destruction-native-stream-construct-20261102/qualification.json)
adds native `2B8370` common construction and literal `2B7F40` PCM initialization.
All 892 unchanged-CPU comparisons pass under an explicit real SDL creation/Pause
ABI, including 50 matching faults and private stack/page checks. Eight compiled
incorrect variants are rejected. The actual SDK boundaries remain declared
native test services, with no retail SDK or Xbox device result qualified.

The complete original catalog still covers 908 contexts, 9,450 routes and 31
sound names. Its 273 distinct permutations now traverse native construction
before enqueue, real SDL consumption and source completion. All 1,147 scenarios
perform actual creation/unpause and retain the prior uninterrupted PCM bits
and pin/reference cleanup. All seven restored regression suites pass.

The typed caller-format owner and independently checked original operands
prepare this component input; full `224E20`/`225EF0` ABI and genuine source
driver/cache/voice admission remain open. Production handoff, normal local/
split/LAN destruction-sound delivery, both combined four-bin profiles and Xbox
waveform/wet-feedback proof remain open. The prior archive-budget rejection is
not rerun here. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-construct-20261102/owner-closure.json)
records private catalog/build/source removal and preservation of the required
original reference. No new large artifact or selected-runtime promotion is added.

## Native cache datum dependencies, 2026-11-02

The [core allocation qualification](../build/test-results/vehicle-destruction-native-cache-datum-20261102/qualification.json)
adds GPR/frame/DF owners for general `16E6A0`, requested `16E770` and initializer
`16E860` allocation. All 4,390 original-code cases and ten restored suites pass;
eight compiled incorrect variants are rejected. The complete authenticated
273-permutation/1,147-scenario payload report is unchanged, along with its
constructor/service/format input and trace hashes. The separate C1 opcode
profile and raw Unicorn mismatch remain recorded.

These are raw core allocation inputs. Native `13FE50`, complete `21F420`, the
new allocator-to-authenticated-read join, source cache/voice admission,
production handoff, normal local/split/LAN destruction-sound delivery and Xbox
waveform/wet-feedback proof remain open. Both combined four-bin profiles and
all complete destruction gates remain open. The
[closure receipt](../build/test-results/vehicle-destruction-native-cache-datum-20261102/owner-closure.json)
records private catalog/build/source removal and preservation of the required
original reference. No new large artifact or selected-runtime promotion is added.

## Native format/caller integration, 2026-11-02

The [format qualification](../build/test-results/vehicle-destruction-native-stream-format-20261102/qualification.json)
replaces typed preparation with native `224E20` and its actual `225EF0` caller.
All 1,147 authenticated payload scenarios execute those entries before stream
construction and playback. The 908 contexts, 9,450 routes and 31 sound names
still cover 273 distinct permutations and 375 chunks, with unchanged exact
uninterrupted same-PC PCM and pin/reference cleanup.

The admitted CPU/FP profile passes 698 format/caller comparisons and all eight
restored regression suites. Eight compiled incorrect variants are rejected.
Raw Unicorn C1 mismatch remains recorded: a separate native original-opcode
oracle supplies that bit for 585 cases, with 295 raw-emulator differences.
Other status bits and signed-zero delta words agree; corrupting either side
of the C1 check fails. This establishes no Xbox hardware FP equivalence.

Masked controls, contiguous logical x87 headroom and immutable 1.0/2.0 operands
bound the format proof. Full physical/sparse/stack-fault ABI, source driver/
World/cache/voice admission, production handoff, normal local/split/LAN sound
delivery, both combined four-bin profiles and Xbox waveform/wet-feedback proof
remain open. The prior archive-budget rejection is not rerun here. The
[closure receipt](../build/test-results/vehicle-destruction-native-stream-format-20261102/owner-closure.json)
records private catalog/build/source removal and preservation of the required
original reference. No new large artifact or selected-runtime promotion is added.


## Native page allocation to actual resource reads, 2026-11-02

The [qualification](../build/test-results/vehicle-destruction-native-cache-pages-20261102/qualification.json)
connects sound-owned `21F420` binding to native core page allocation, real
busy/delete callbacks and complete datum retirement. All 2,585 original-code
comparisons pass, including 24 faults and 187 read-boundary stops. These stops
compare the original CPU call ABI without supplying an I/O result; they do not
compare the original return after I/O. The private stack-probe scope and
unexercised negative-ring/alignment sites are retained in the coverage report.

A separate cold-cache lane runs all 273 distinct permutations and 1,147 lifecycle
scenarios from declared empty pools. Native code allocates/binds the records;
the existing strict native I/O service reads all authenticated sectors and
retains receipts before readiness. PCM decoded from the actual physical cache
bytes reaches the native stock stream lifecycle and real manual SDL, with exact
uninterrupted same-PC PCM and pin/reference checks. The full 908-context/
9,450-route/31-name catalog passes, with 55 multi-chunk permutations, 375 chunks
and 1,657 actual reads totaling 53,057,536 bytes. No loaded cache rows are supplied.

All twelve restored suites pass; eleven wrong compiled variants are rejected,
including skipped reads and receipt publication before physical bytes. The
prior loaded-cache payload report remains identical, as do the constructor/
service/format input and trace hashes. The independent C1 reference profile and
295 raw Unicorn differences remain unchanged. Declared empty-pool/physical
reservation, driver ABI, unity gain/rate and manual producer timing still limit
this component result. Source World/cache/voice/device admission, production
handoff, normal local/split/LAN delivery, original event timing/selection/gain/
pitch/spatial behavior, both combined four-bin profiles and Xbox waveform/wet
feedback remain open. No complete destruction gate is closed by this result.
The prior factory archive-budget refusal is not rerun or bypassed. The
[closure receipt](../build/test-results/vehicle-destruction-native-cache-pages-20261102/owner-closure.json)
records private source/build/bank removal and preservation of the required
original reference, with no new large artifact or selected-runtime promotion.
