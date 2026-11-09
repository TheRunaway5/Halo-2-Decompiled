# Private TU1.5 instanced ray callback frames

The new callback DATA core is shared by raw caller views and ordinary value
views. The existing public callback CPP remains unchanged by this component.
The raw forwarding frame implements actual `24DEF0`; its real virtual dispatch
can compose the new `24DF20` collector frame. The collector implements the full
source policy, but qualification currently ends at a genuine `16C000` entry or
a genuine duplicate-stamp return. There is no production/default binding here.

The final private GCC/Clang ASan/UBSan receipt is
`build/test-results/native-instanced-ray-callback-final-retained-20261109/report.json`.
It checks 824 physical paths: 148 normal paths (82 complete returns, 66 actual
query entries) and 676 transaction prefixes. Each compiler records 1,944 raw
transfers and 25,760 complete pre-I/O GP/flags/XMM/FP/live80 packets. The independent
hardware observations contain 10,272 packets. All 16 forwarding instructions and
23 collector prefix/duplicate instructions are covered, 39 of 62 combined sites.
The 23 collector instructions after the query returns remain unqualified.

The registered CMake sanitizer/release pair passes 2/2 tests with no skips
(`native-instanced-ray-callback-official-20261109/cmake/report.json`). Both
retained reports preserve their exact current before/after source cohort. This
includes the later shared MOVAPS destination-alignment helper correction; the
callback frame itself only uses scalar/narrow storage. Offline committed
dependency copies and the complete official owner were removed.

The corpus covers signed index endpoints, index -1 aliasing the epoch itself,
unrounded caller-stack alignment, saved BX/SI/BP aliases, collector fields sharing
argument/storage slots, and forwarding's private packet sharing the collector.
The epoch is captured before saved-register pushes. The result pointer precedes
the stamp write, and later argument reads stay interleaved with actual pushes.
Forwarding acquires its virtual target after publishing packet+14, then publishes
one CALL-before boundary and performs the atomic return-address store.

The ordinary value views also check 128 DATA paths. Their query callback validates
parameters against the unchanged CPU's genuine query-entry packet and then
throws; it never returns a fabricated hit. Their declared DATA excludes raw
control-stack storage. This is shared-algorithm support, not another full raw ABI
or positive-query proof.

Four compiled defects are rejected: unprotected FP providers, premature raw PUSH
commit, unsigned stamp-index decoding, and an early collector-owner read. Incoming
masked dense x87 counts 0/1/4/8, full logical 80-bit values, varied TOP/control/
rounding/sticky bits and hostile host-FP callbacks are covered. RawScope's sparse,
active-MMX, unmasked, overflow and State/DATA/policy-mutating callback exclusions
remain in force.

A physical `before_write` stop on the indirect CALL is a stop before the entire
instruction: the CPU observer has not committed its pending target-read event.
The twelve such cases are explicitly represented as native `before_instruction`
stops. No read event is removed to manufacture agreement. The physical proof does
not qualify a provider failure after a completed target read.

That distinct retained-provider contract has a separate receipt. It executes the
exact pinned canonical `24DEF0` body (only the fixture symbol is renamed) and the
raw adapter on 64 forward inputs. Both perform the real target read before the
stack-write provider throws; ordered memory and every other ABI field agree.
The raw adapter retains the pre-PUSH stack pointer until provider success. The
existing retained `State::push` predecrements GP4, so its failed-write state is
four bytes lower. Both exact expectations are asserted; full exception parity
and physical post-target fault delivery are not claimed. No collector outcome is
provided in this check.

Primary and AI-frame generated copies of both source owners still retain their
original bodies. No source routing, alternate-image closure, returned-query
outcome, full ray-parent admission, gameplay/performance or launcher qualification
is claimed. All completed private test owners were removed, with no large
artifacts retained.
