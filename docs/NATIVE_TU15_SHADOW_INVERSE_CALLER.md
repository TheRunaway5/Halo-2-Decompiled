# Native TU1.5 shadow inverse caller

`native_tu15_shadow_inverse_binding.cpp` admits only the actual TU1.5
`C3DC0:C4879→144090` non-tail call. Its `SourceShadowProfileDirect` carries
the actual image and call metadata. It reuses the existing physically qualified
`native_tu15_camera_inverse_frame`; that core and its header are unchanged.
There is no new arithmetic implementation, register machine or source stack.

At the real call, EAX is `record+4` and ECX is `record+38`, giving adjacent
52-byte output/input frames. The ordinary profile requires DF=0, an available
SIMD policy, a representable dense x87 ABI, and no active MMX alias. It rejects
wrapping records and overlap of the complete `1BC`-byte record with the active
save/return span. These are integer admission checks; they read no source
memory and do not reconstruct private pushes.

The binding decodes arguments once, runs the typed inverse, and exports the
actual MXCSR on success and failure. It keeps opaque GP/SIMD/flags and older
x87 values rather than constructing the original private scratch values.
Only after completed math does it read the real return word and require
`C487E`. An unknown word raises `NativeTu15ShadowInverseReturnRequired`,
carrying that actual value, with data/FP committed and SP unadvanced. Missing
RET storage also leaves completed state and SP intact. There is no early
return-word read that could move a failure ahead of owed stores.

The seven-owner original graph is finite and has no engine/platform services:
`C3DC0`639 sites, `121FB0`122, `144090`82, `166C90`687, `1454D0`41,
`122130`177 and `30BD0`38. These1786 sites describe the graph, not an exhaustive
parent branch-coverage claim. The baseline oracle executes the entire
unchanged original `C3DC0` over declared inputs on physical i386 hardware.
Every captured leaf frame comes through the original `C4879` CALL and its
actual pushed `C487E` word. No original instruction or callee result is supplied.

The45 whole-parent cases capture50 actual leaf frames, including both
`166C90` early rejection conditions, its normal path, all six cube faces and
five next-loop iterations. Another250 cases cut ordered reads, before/after
writes and the real RET read. Three genuine SIGSEGV cases replay exact captured
input frames against unchanged `144090` with reduced ELF mappings: missing
source, missing output, and missing RET after completed writes. The latter
two use naturally captured page-boundary record/stack frames. This is explicitly
captured-frame leaf replay, not a claim that an unmapped whole parent could
first construct those records. Separate PT_LOAD segments avoid accidentally
mapping the supposedly absent pages.

Twelve post-return controls retain incoming opaque values or poison only the
original-written EAX/ECX/EDX, arithmetic flags and XMM0..5. They preserve whole
parent data, ordered publications, trace, full final GP/defined flags and owed
FP/live x87 results. Six controls corrupt an owed inverse word and fail the
same comparison. XMM6/7 and nonvolatile GP are not poisoned. Independent static
review also checked the early/normal continuations and587 next-iteration states.
These are offline liveness experiments, not supplied reference outcomes.

The current303-case compressed fixture is27,293 bytes, SHA
`28f35de49b097dc37593e9e540027c2e0a476aaca7eadf9aa45338d30fb8fcb0`.
An independent repeat with stronger full-parent GP/flags checks reproduces it
byte-for-byte. Genuine fresh-status cases include MXCSR `1F80→1FA0` and
`1F80→1FA8`. GCC1/1, Clang ASan/UBSan1/1 and Clang11/11 including ten real
binding mutations pass. Corrupt RET values and record/stack aliases have
explicit negative controls; the inverse core is unchanged.

The first host guard compared too much: C++ exception unwinding clears private
x87 last-instruction metadata. Its exact before/after failure is retained.
The corrected guard checks CW, SW/TOP/status, tags, occupied80-bit values and
MXCSR. The retained ABI has no FIP/FDP fields, and none of the authenticated
caller graph inspects an FP environment. Private unoccupied bytes and FIP/FDP
are not invented or restored as gameplay outputs. Kernel faults compare owed
memory/FP state, not the retired private EDI save or intermediate private SP.

`cmake/NativeTu15ShadowInverse.cmake` adds only the new production binding.
The existing projection module owns the single inverse definition. A separate
composition test compiles/links both actual modules, verifies one definition
per source and passes1/1. Evidence and bounded earlier failures are under
`analysis/campaign-native-new-game-camera-20261028/shadow-inverse-*`.

Shared World routing, counters, ShadowFrame retirement and coherent gameplay
remain separate gates. The future World route must retain its existing
`&memory==&bank` check before constructing this TU1.5 profile. The one shadow
call can then become an actual required-owner bridge. No universal `144090`
entry, other TU1.5 caller, MapPack `142D00`, whole `C3DC0`/`166C90` native
implementation or complete shadow/render-frame qualification is implied.
