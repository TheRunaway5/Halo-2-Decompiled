# Semantic native campaign listener selection

`include/halo2/native_campaign_audio_listeners.hpp` implements the complete
`12A810` listener-selection dependency graph as typed queries and numerical
algorithms: `12A930` distance, `2287B0` cluster cutoff, `253070` cluster distance
scale and `2531E0` triangular cluster-byte lookup. The original register-level
listener implementation has been removed. The five existing sound-manager
entries now transport actual arguments/results through thin adapters.

There is no virtual CPU, instruction dispatcher, scratch register array,
virtual x87 stack or fallback in this algorithm. Scalar SSE and actual x87
instructions retain the source's rounding, intermediate precision and NaN
operand precedence. Source, listener, player and structure views retain their
real owners and read fields lazily in source order. No callbacks, imports or
unknown native dependency owners occur in this finite 312-site graph.

The capability selects the nearest active listener for positional sources,
computes ordinary and vertical-column distances, applies cluster cutoffs and
distance limits, and uses the first occupied local player for the original
nonpositional/unlimited-distance fallback. Optional result-pointer stores retain
their instruction address, including when that pointer aliases source/listener
data. Invalid locations/clusters, zero/full table cells, interpolated scale
ranges and the fallback cluster byte path preserve their distinct behavior.

## Actual caller and result contracts

`12A810` returns the listener in EAX and distance in XMM0, optionally writes that
distance and pops 12 argument bytes. `12A930` and `253070` return XMM0. `2287B0`
and `2531E0` return AL; upper EAX bytes are dead scratch. Every original preserved
GPR and SIMD register remains part of its contract. The shared sound-manager
comparator now has an appended low8 mask with a default-zero optional argument;
existing full-register and low16 contracts retain their behavior.

`analysis/campaign-native-listeners-20261028/caller-contract.json` records the
authenticated `12AB30` call at `12ACEA`: source, distance limit and null output
pointer enter; `12ACEF` consumes the listener in EAX. Complete unchanged original
calls also execute all four internal caller/continuation relationships. The two
object-loop callers likewise consume EAX and the optional stored distance.

The typed state service checks available physical extended-operand capacity;
it performs no virtual stack operations. The absolute-distance expression needs
five free x87 entries and the selected-distance square-root/ratio needs one.
Calls with insufficient capacity fail closed as the existing reference backend
requires. The proof includes zero through three retained caller x87 operands,
with their exact 80-bit values checked after the whole original call.

## Independent original proof

The offline reference executes unchanged i386 instructions from the user's
owned TU1.5 image on the physical CPU, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
It patches no original instruction, supplies no dependency return and resumes
no unknown original owner. No emulator participates in this proof or algorithm.

672 whole calls execute 62,508 original instructions. Every one of the 312
instruction sites executes: 98 selection, 77 distance, 45 cutoff, 66 scale and
26 byte-lookup sites. Native comparisons match all 10,616 mutable reads with
their store-count interleaving, 264 ordered stores and before/after bytes,
complete retained banks, actual return widths, stack cleanup, preserved
GPR/SIMD registers, retained 80-bit operands and FP control/status/MXCSR.

Cases cover listener/player activity and fallback, modes and invalid clusters,
ordinary/column distances and bounds, cutoff ratios, scale-byte endpoints and
interpolation, source/listener output aliasing, signed zeros, denormals,
infinities, NaN payloads and signaling NaNs, all four rounding modes, three x87
precision controls and DAZ/FTZ settings. GCC and Clang ASan/UBSan pass. Private
mutations are rejected for reversing nearest-listener selection, discarding
column bounds and moving the upper scale-range read before the lower read.

The actual shared manager's low8 comparator also passes its separate regression;
removing that comparison is rejected. Full EAX and low16 tests remain distinct.

This is bounded implementation/original evidence for the closed listener
capability, not whole campaign audio or playback acceptance. Unmasked FP traps,
faulting/unowned memory, arbitrary dead scratch registers and live gameplay
acceptance are outside this proof. `12AB30` still needs source eligibility and
retirement (`12AAD0`/`129E30`) and their actual dependencies. The whole campaign
audio gate remains closed at four qualified required entries of forty.

Final reports in `analysis/campaign-native-listeners-20261028/`:

- `gcc-occupied-v3/report.json` and `clang-occupied-sanitizers-v2/report.json`.
- `contracts-v1/report.json`: actual manager/registry contract regression and mutation.
- `root-targets-v1/report.json`: focused registered listener/contract integration.
- `owner-graph.json`, `caller-contract.json`: authenticated graph and campaign caller.
- `cleanup.json`: final source hashes and disposable-owner removal.

The completed fade, ducking and channel algorithm files remain unchanged. Raw
banks, ELF files, vectors, private mutations and builds are removed through the
official scratch wrappers; only small reports and source references are retained.
