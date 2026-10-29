# Native two-overlay animation reset

Original `116920` is the unconditional reset called after nested action13
by `D0520`. It resolves the real unit's signed extension offset and resets
the actual 32-byte channels at extension `+9C` and `+BC`. It does not create
an animation manager, infer a clip, acknowledge an action, or zero the rest
of the extension. Graph/packed IDs, lookup keys, playback flags, event masks,
rate and frame are published with original widths and order.

The semantic implementation uses typed channel views. It preserves the
constant read before body lookup, one captured extension pointer, both
distinct channel publication orders, and every surrounding byte. Source
XORPS/MOVSS are raw bit moves; no FP arithmetic or NaN canonicalization is
introduced. The original one scalar bits are supplied by existing engine storage.

The independent reference executes unchanged English Xbox TU1.5 SHA256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Physical i386 checks complete 73728-byte nonzero patterned banks and SSE
status; Unicorn records exact source reads/stores. Canonical-one cases use
actual original `3F800000`. Additional raw-bit stress uses signaling-NaN
and negative-zero words to verify bit transport only. Controls cover source
indices, positive/zero/negative extension offsets and retained-pointer alias
at the offset field. Selected read/store fault prefixes independently check
every partial bank, including source constant faults before body lookup.

The real compiled binding is replayed on every whole/fault case and RET-read
controls. It consumes the caller's actual return word and no arguments;
it does not reconstruct private volatile GP/flags/SIMD scratch. Actual
direct caller continuations and the required ED060 path are authenticated
in `callers-and-frontier.json`. Read-only activity is diagnostic.

GCC, Clang and instrumented ASan/UBSan use the production CMake module.
Private compiled mutations omit the secondary lookup reset, publish a wrong
primary rate, or reorder frame/rate publication; each must fail. The original
reference must regenerate byte for byte. Only the compact required JSON and
bounded reports/source hashes survive; original ELFs, raw banks, native
binaries and mutants are disposable.

This closes the reset state producer. Full action13/17 begin, `ED060`
seat/parent transitions and speech remain open. The action17 animation
admission/stop path reaches authored lookup and resource-demand owners;
their exact frontier is retained with this component. Future `126200`
demand allocation will reuse root's proved shared `16E6A0` owner; `116920`
requires no allocator and does not alter either shared allocation kernel.
