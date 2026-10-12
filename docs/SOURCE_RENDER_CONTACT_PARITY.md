# Attributed render-contact query

`source_render_contact` ports retail `14AF40` over the complete ordinary
`source_vector_contact` dispatcher and `source_object_light_attribution`.
The caller supplies the actual registered spatial and owner banks, current
WORLD matrices, active permutations, render/collision metadata, and resource
residency. This module does not register native objects, generate current
poses, schedule resource loading, or activate gameplay lighting queries.

The result retains all fourteen dwords of the original 56-byte record consumed
by `D1E10`. A miss can partially write that record. The caller's initial words
are inputs, and fields not written by the selected source path survive.
`SourceRenderContactPacket` and `SourceObjectLightContactPacket` have the same
literal array layout; the complete record can be passed directly to the
separate object-light sampler.

## Exact control

The bank-valid input represents the original active-lightmap pointer, positive
group count, active BSP, bitmap NONE, and matching checksum guards. The source
always copies the initial query start before this guard. Guard failure leaves
all attribution words untouched. A valid bank initializes words 0, 1 and 2 to
NONE, then initializes only the low material halfword of its internal 96-byte
collision packet.

The initial-query AL input selects `4480000D`; retained queries use
`04800C0D`. This is independent of the `D2A50` one/five-ray mode and its leading
`.001 * ray` start offset. Those outer services remain separate.

An ordinary terrain or instance contact enters `14AC60`. An object contact
uses its actual body type byte at `AA`. Type 6 scenery with definition policy
other than 2 rejects attribution. Other admitted objects call `BF9A0` and then
`14A8E0` when the model, render-model, collision, positive PRT count and
positive render-leaf count are available. `BF9A0` can write render/collision
datums even when its count guard fails; these partial writes are retained.

A failed `BF9A0` changes the stored query flags to `04800005`, immediately
queries the static map with both exclusions NONE, and attributes that map
contact if present. The changed flags survive any subsequent outer retry.

Successful attribution returns before checking the collision surface flag.
Failed attribution retries only when the current collision's **authored**
surface byte has double-sided bit 0. This is separate from the returned plane
orientation. It advances the current start by the original full ray multiplied
by literal `3CA3D70A` (.02), preserving separate multiply/add rounding. There
are at most three collision attempts. Native closest-triangle replacement,
geometric backface guesses and generic fallback contacts are not used.

`14A8E0` clears its literal output fields before selecting the current active
highest section. Section selection is visible on source misses. Its accepted
leaf/strip/triangle, node hint and render barycentrics come from the existing
unchanged-source-proved service. Map attribution similarly retains its early
instance render-leaf guard and literal word-6 writes.

## Residency and immutable original replay

`tools/differential_source_render_contact.py` executes unchanged original
`14AF40`, `1697C0`, `14AC60`, `BF9A0`, `14A8E0`, `12DE70` and their actual
callees over the immutable restored Zanzibar snapshot. Executed instructions
are checked against the pinned retail XBE. Read-only hooks observe attempts
and admitted target order; no code, lookup, math or allocation routine is
replaced. Every written captured page is restored between probes and cases.
No guest I/O or advancement occurs.

The source snapshot is paused at `0012B33E`, tick 12474. Its FCW `023F`,
MXCSR `1F80`, GDT and segment descriptors are explicit inputs. These are
**diagnostic rays over retained banks**, not observed gameplay light queries,
original frame 20, or the later live shadow phase.

All 55 cluster and 19 instance-definition descriptors are probed independently
with unchanged `12DE70` mode 3. The source-ready count is 42 clusters and 15
definitions. Twenty-two of those descriptors have resident byte 0 but a ready
resource-header byte 1: the complete original `223C70` fixup executes, rather
than equating resident byte 0 with a source miss. The remaining 17 descriptor
probes reach uncaptured kernel service `80020F5A` and stay unresolved.

The native residency spans distinguish 0 (proved source miss), 1 (actually
resident bank) and 2 (unresolved scheduling). Selecting state 2 throws an
explicit missing-input error. It is not mapped to false, white lighting or
an assumed synchronous source load. The native model-attribution banks also
require explicit, actually resolved resident section data.

The maximal corpus has 3,912 supplied inputs: all 3,792 retained diagnostic
rays and 120 active-bank/caller-word controls. Whole original execution
completes 3,758 inputs; 154 select a proved unresolved descriptor and stop
before that `12DE70` call. Their lack of a final packet is preserved as an
unknown-input rejection in the native test.

The initial object-contact census is 80 scenery rejections, 65 model
attribution branches and 12 map fallbacks. All 80 and 65 complete; nine
fallbacks complete and three select unresolved descriptor `801871A4`
(`actual_418`, `actual_422`, `actual_430`). Actual double-sided contacts
`actual_405` and `actual_407` exercise the full three-attempt limit. Sixty
invalid-bank controls prove unchanged caller words.

## Native comparison and reproduction

`tests/source_render_contact_tests.cpp` uses the literal 202-owner raw spatial
snapshot, independently frozen 39 model/collision banks, 158 current WORLD /
active banks and 44 lookup misses. The target callback runs complete native
`source_object_posed_target`; it does not replay recorded target outcomes.
The three original resident render-attribution banks supply CPU geometry and
leaf maps. Source-ready map geometry is decoded from the supplied cache.

The frozen corpus compares status, the final current query start, all fourteen
attribution words, every complete 96-byte collision attempt, flags,
exclusions, retry starts and admitted target order. The exact word/count result
is recorded in `analysis/multiplayer-shading/render-contact-exact.log`.
The source fixture and provenance are
`tests/fixtures/retail_source_render_contact.tsv` and its adjacent
`.provenance.json`; the full report is
`analysis/multiplayer-shading/render-contact-original.json`.

Rebuild `source_render_contact_tests` in a private directory and run CTest
`retail_source_render_contact`. Regenerate the source fixture with
`.tools/python/bin/python tools/differential_source_render_contact.py` when
the original XBE and immutable snapshot exports are available. Normal and
ASAN/UBSAN/leak checks validate this CPU slice; they do not prove actual native
registration chronology, asynchronous-loader parity, current PRT creation,
`D1E10` sampling, or live gameplay integration.
