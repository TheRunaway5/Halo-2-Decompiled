# Native actor/order camouflage dependency

This closes the original `1EB590` actor camouflage-control call context,
including `D3340` deactivation with its real supplied-speed constant and
`B7E50` dirty-entity publication with the camouflage dirty mask. It is a
required dependency of actor order-reset `1EC820`; it does not make
`ai_set_orders` or that complete reset native.

The typed kernel borrows the existing actor, object and simulation-session
banks. It preserves the unit-NONE no-read gate, ordinary shared unit flags,
the two separate deactivation stores, actual fade-speed field, object-owner
reload, entity-NONE guard and session/registration dirty-flag gates. It never
supplies a network result or creates an entity. The existing multiplayer
camouflage semantic owner establishes related behavior, but its eager unit
view does not supply this campaign call's lazy source accesses and faults.

The independent fixture executes unchanged English Xbox TU1.5 code from
SHA256 `21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Physical i386 checks the complete declared bank, boolean result and FP state;
Unicorn records ordered source accesses. All 83 original instruction sites
execute. Control cases cover enable-from-clear and preservation of unrelated
bits, both dirty registration states, all original session gates, entity index
masking, source SSE rounding/FTZ/DAZ/sticky status, source fault prefixes and
restoration of the host's ambient SSE environment.

Clang, root GCC and explicitly instrumented ASan/UBSan use the production
CMake module. A compiled private mutation removes actual dirty publication
and must fail the ordered-state reference. Reference replay must regenerate
the canonical bytes exactly. Only the required JSON reference and bounded
reports survive; original ELF images, raw memory banks, builds and the private
mutant are disposable.

This is the tested x86_64 native path with masked source SSE exceptions.
MSVC/non-x86 implementations and arbitrary callers of `D3340`/`B7E50` with
different supplied-speed/mask contracts remain separate qualification work.
No lifted fallback or runtime executable is used.
