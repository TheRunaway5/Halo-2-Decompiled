# Original particle target and viewport selection

The new target module owns complete TU1.5 bodies `14BA0`, `1CD10`, `1DCD0`,
`1DD00` and `25940`: five bodies and 278 original instruction sites. The
ordinary `2B9F0 -> 132B0` containing-body chain authenticates `132B0`'s call
at `1337A` to `14BA0`. That owner directly authenticates each of the remaining
four routines. The generator checks whole bodies and original call bytes
against the owned English TU1.5 executable, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

`14BA0` selects retained color/depth targets and their viewport. It preserves
mode-two fixed 128-pixel rendering, ordinary and scaled camera rectangles,
auxiliary-target selection, source float-to-integer conversion, and the full
existing `4021E0` viewport/matrix producer. `1CD10` retains the original cache:
an exact pair requires no SDK target call; matching color layout with changed
attachments takes `401C10`; other changes require `401950`. Target-record
availability, unavailable flags, counts and indexed lookups retain their
original `1DCD0`/`1DD00` arithmetic. No target pointer, callback result or SDK
return is supplied to make these paths complete.

Both original selector tables are mounted read-only. Thirty-three enum slots
have defined non-null dispatch targets. Six slots point at zero in the owned
executable and are not admitted as valid targets. This is an original input
domain boundary, not a substitute target mapping.

The module is connected to the shared particle/World execution context.
Existing graphics-root and queue-reset admission remain intact. The previous
context test now reaches the real required SDK attachment setter instead of
stopping before the newly implemented target owner. Native GPU binding remains
open: the original SDK setters dereference surface headers and allocation state
that the current particle graphics ABI does not own.

The permanent target test covers twenty complete declared-cache viewport
paths, all thirty-three defined target selections, twenty-four retained-record
lookups and six cache transitions. It retains occupied queue canaries 17/19.
The metadata fixture comes from the existing original layout arithmetic;
these are CPU inputs, not native surface allocation or startup evidence.
The source includes explicit checks for ordinary, half-size and fixed-128
rectangles. It also preserves the original unsigned handling of a negative
origin while clipping the far edge.

The next native resource connection should reuse the existing
`NativeTu15FramebufferStartup` and `NativeTu15FramebufferAliases` owners.
Those owners already publish opaque identities for actual renderer images,
retain their renderer use, and follow the original color-surface rotation.
Their identities must not be dereferenced as SDK headers. Target binding still
needs an actual prepared native pass, attachment selection and completion
lifetime, followed by the remaining original clear/shader/state consumers.

Validation reports are under
`analysis/map-world-features/particle-target-selection-20261018/`.
New compiler, oracle and CMake outputs use the managed test lifecycle; raw
banks, objects and binaries are disposable. The existing accepted source and
reference evidence, renderer captures and launcher runtimes remain separate
inputs. This work does not establish full original view execution, ordinary
scene-depth visibility, all-map interaction/network/reset parity or delivery
through the normal launchers. The full goal remains active.

The managed original-code comparison passes all 83 cases: 46 complete calls
and 37 prefixes that stop at required SDK owners. Each case compares 149,912
source bytes, general/SSE registers, defined flags, x87/MXCSR state and the
instruction path. No instruction patches, expected native data substitutions
or supplied game/GPU returns are used. All generated raw banks, oracle outputs,
objects and the diagnostic executable were removed with their owned scratch.
Only small case reports, commands, hashes and failure diagnostics remain.

Normal CPU/context and GPU camera-order validation is tracked separately in
`cmake-validation-v2/report.json` and `cmake-gpu-validation-v1/report.json`.
The GPU regression now follows boundary-two through the real target selector
to the still-required `401950` attachment owner. Failed initialization must
remain unpublished and cannot be retried or advance the pass serial again.
