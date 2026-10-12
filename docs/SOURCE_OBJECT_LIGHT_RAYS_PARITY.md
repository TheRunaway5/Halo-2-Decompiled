# Object-light ray orchestration

`source_object_light_rays` ports retail `D2A50`. The explicit contact callback
represents complete `14AF40`; the sample callback represents complete `D1E10`.
The module preserves their original call order and partial writes. It does
not create registered objects, current poses, resource residency, a native
map-light sampler, or a gameplay query schedule.

The existing native `query_object_lightmap` MAP adapter now consumes this
outer helper instead of duplicating its rays/start offsets. Its explicit
MAP-only contact/decoded-image services and independently proved part/UV/
incident corrections are documented in `SOURCE_MAP_LIGHT_SAMPLE_PARITY.md`.
This does not activate the complete dynamic/residency services described here.

## Inputs and exact control

The caller supplies the actual query center, exclusion handle, independent
initial-query byte, complete 32-bit query flags, initial fourteen-word contact
record and five-word sample. The `bank_valid` input represents the actual
active-lightmap/group/BSP/bitmap/checksum guard. A false guard returns status 2
without calling either service or changing either record.

Flag bit 0 selects these literal rays in order:

1. `(-10, 0, 0)`
2. `(10, 0, 0)`
3. `(0, -10, 0)`
4. `(0, 10, 0)`
5. `(0, 0, -40)`

Otherwise flag bit 2 selects exactly one supplied custom delta. With neither
bit, the single ray is `(0, 0, -10)`. Bit 0 overrides bit 2. The custom pointer
is not a count: original `D2A50` overwrites its stack count with one or five.
There is no custom multi-ray path in this routine.

Each start is `center - ray * float(0x3A83126F)`. Multiplication and subtraction
round separately; the source file disables floating-point contraction and
fast math. The selected exclusion handle and initial-query byte are passed
to every contact query. The full flags and initial-query byte are passed
unchanged to the sample service.

The status begins at 2. A false contact can partially write the contact
record. A true contact calls the sample service, which can write the sample
for any returned status. Another ray runs only while the status remains 2.
Statuses 0, 1 and 3 terminate immediately. There is no averaging, weighting,
or status filtering in `D2A50`; `D1850` decides whether the resulting sample
may update the retained object-light cache.

The raw contact array is the same complete 56-byte layout accepted by
`source_render_contact` and the separate `source_object_light_sample` object
branch. Unknown services must throw an explicit missing-input error. They
must not return an invented miss, unavailable status, white sample or zero
sample. The module similarly rejects a required custom delta or contact
service that was not supplied. A sample service is required only after an
actual successful contact.

## Unchanged original whole-call corpus

`tools/differential_source_object_light_rays.py` executes unchanged `D2A50`,
`14AF40`, `1697C0`, attribution routines and `D1E10` with their actual lower
callees over the retained Zanzibar snapshot. There are 202 complete owner
records and original raw spatial pools. Diagnostic queries use each owner's
literal cached center, both initial-query modes, the original one/five modes,
and six explicit axis/sign custom rays. These are supplied diagnostic inputs,
not observed gameplay queries or a claim that each owner was scheduled.

The immutable snapshot is paused at `0012B33E`, tick 12474. Its CPU segments,
FCW `023F` and MXCSR `1F80` are retained inputs. It is distinct from original
frame 20 and the later captured shadow producer phase. No guest I/O, input,
advancement or instruction patching occurs. Every executed instruction is
checked against the pinned retail XBE. Every written captured page is restored
between calls, and all 223 protected owner/spatial banks match their originals
after the replay.

As in the upper-contact corpus, unchanged `12DE70` preprobes distinguish actual
resident geometry from unknown resource/kernel paths. Resident byte 0 is not
assumed unavailable: ready resource headers execute full original `223C70`
fixup. Unknown scheduling remains an explicit boundary. Sample bitmap kernel
paths are also stopped, not converted into source statuses.

The whole-call corpus contains 3,758 inputs. Original execution completes
3,351: 2,464 status-0 calls, 26 status-1 calls and 861 status-2 calls. The 26
status-1 controls set the explicit current-PRT cache count to zero before the
whole call; geometry, metadata, contact, `24490`, and `D1E10` remain original.
They are diagnostic cache controls, not the snapshot's actual cache count.
Ninety-six invalid-bank controls preserve the caller records.

The remaining 407 whole calls reach unknown services: 263 geometry-residency
paths and 144 sample kernel paths. Their native comparison must reject the
corresponding missing service rather than fabricate a final status or sample.
No whole call in this retained corpus reaches the temporary/generated-bank
status 3.

## Separate return-state controls

Thirty-two explicitly labeled `suffix_` rows execute a real original `D1E10`
call, then supply its return ABI state at `D2B72`. They cover statuses 0, 1, 2
and 3 with either the original output or an explicitly supplied five-word
sample. The original `D2B72` suffix, loop and subsequent contact/sample callees
execute unchanged. The tool records the original sample status separately
from the supplied status.

These rows prove outer status acceptance, retained writes and remaining-ray
order. They do **not** prove a full `D1E10` status-3 producer or a generated
resource lifecycle. Status 2 controls on the five-ray modes reach a subsequent
real sample; status 2 controls on a one-ray mode retain the supplied output
and return 2. The other three statuses stop before another ray.

## Native verification and caller boundary

`tests/source_object_light_rays_tests.cpp` consumes the frozen whole-call and
separately labeled suffix traces. Callbacks supply their recorded complete
service outcomes and check the exact incoming packet, start, delta, exclusion,
initial byte, flags and previous sample. The native orchestration is compared
with 339,630 defined fixture words across 4,110 contact entries and 2,670
sample entries. Unknown service rows are checked for rejection at the correct
contact or sample stage. Separate controls check missing-input guards and
unchanged full flags.

The test does not claim that all native contact/sample callees were composed
in these callbacks: those modules have independent whole-source tests.
Production may call this service only with actual complete contact and sample
providers, including map sampling and known resource states. Live native
registration, current owner/pose banks, async residency and query scheduling
remain external producer boundaries. No runtime activation was added.

The fixture and its hash/provenance are
`tests/fixtures/retail_source_object_light_rays.tsv` and the adjacent
`.provenance.json`. The detailed original report is
`analysis/multiplayer-shading/object-light-rays-original.json`.

```sh
cmake --build build-mp-lightmap --target source_object_light_rays_tests
ctest --test-dir build-mp-lightmap -R '^retail_source_object_light_rays$' --output-on-failure
```
