# Native TU1.5 custom constraint schema emission

The native emitter replaces actual `32A390` as a typed integer algorithm.
It reads the actual payload size and retained builder packet, writes the
source schema14 header, reads the callback identity at the original late
point, publishes that identity, advances the retained schema cursor and returns
the actual payload pointer. Header and cursor arithmetic retain unsigned
32-bit wrapping. Emitting an identity does not execute or qualify that callback.

The late callback input is deliberate. The original reads its first cdecl
argument only after the header store. The native API uses a required readonly
callback-identity input closure at that same point. This preserves actual
argument/header aliases without putting a virtual GP register file or an
instruction dispatcher in the kernel. The third cdecl argument is unused.

Both GCC and Clang pass 69 unchanged original cases: 59 complete calls, four
read-fault prefixes, three rejected stores and three exceptions delivered after
a completed store. The retained inputs include the actual original argument
cells, so tests cover headers overwriting callback, payload-size and builder
packet arguments, plus packet/header aliases. The proof compares 264 ordered
reads, 187 ordered stores, retained bytes and the true payload return. The
original executes 955 instructions. A private mutation that reads the callback
before publishing the header fails the original comparison.

An independent source ABI audit finds exactly three direct calls in the
authenticated TU1.5 text: `31F7C3`, `31F8A0` and `31F95E`. Their continuations
overwrite ECX, EDX and EFLAGS before any use. These internal callers consume
only the actual EAX payload return; the source callee changes no FP/XMM or
callee-saved GP values. The native seam must replace the internal generated
`run_326820` function as well as any public entry dispatch. Unknown indirect
caller ABI contracts remain unqualified.

Reports and source/authority hashes are retained in:

- `build/test-results/native-constraint-schema-emitter-final-tu15/`
- `build/test-results/native-constraint-schema-emitter-abi-tu15/`

The 1,199,447-byte fixture, original executable/banks and both compiler build
trees were removed. The cleanup receipt confirms owned scratch removal. The
generated seam, actual constraint-producer bodies and visible multiplayer
participation require separate current evidence.

The generated internal function now has a minimal scoped native entry. Its
policy is tied to the actual retained memory bank and constructing executor.
Before publishing the native leaf phase, it authenticates one of the three
actual parent callsites from the world's existing last-instruction cursor.
There is no speculative memory read of the return slot or payload. Early
payload-size/packet inputs and the late callback input retain source read
order. The real return slot is read at RET, and an unqualified continuation is
reported explicitly after that read. The adapter publishes only the genuine
EAX payload return and actual RET stack pop; it preserves caller GP, flags,
SIMD and live x87 state instead of inventing dead volatile outputs.

An active unsupported live scope refuses explicitly. Full instruction tracing,
mutable store observation, an unknown caller, the wrong executor/image and
invalid caller storage cannot silently fall through to the literal body.
Unscoped offline/reference execution retains that body. Per-bank activity and
refusal counts have a mutex-protected getter and explicit lifetime reset.
The live world can leave the optional direct activity pointer null and use
that protected getter; direct pointer readers require external synchronization.

The final scoped proof has 210 actual three-caller cases and passes against
both exact old frozen and current state headers on both compilers. It compares
804 ordered reads and 570 stores, including a real late-RET read-fault prefix,
committed observer exceptions, header/argument aliases, actual/canonical store
sites and caller FP/SIMD/live x87 preservation. Policy tests cover both trace
flags, trace activation after a committed header, nested bank ownership and
all explicit refusal categories. Removing the generated native seam fails the
native execution gate. The report is
`build/test-results/native-constraint-schema-emitter-scope-final-tu15/`.
Its 3,661,486-byte fixture, exact dependency overlays and four private compiler
builds were removed, with owner removal recorded in `cleanup.json`. The source
generator retains the same minimal native entry. Current visible multiplayer
participation is still a separate gate.
