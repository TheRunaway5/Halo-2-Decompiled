# Node progress query 1CF0E0

The original TU1.5 routine pushes ECX, initializes that stack word to zero,
and checks the signed node handle. A present handle calls the actual node
descriptor lookup 1E3150 at 1CF108. Its returned pointer supplies the signed
divisor. The routine returns the clamped ratio in XMM0 or reloads its scratch
word when the divisor is not positive. A write observer can replace that
scratch word in the retained source contract. The native view preserves
the three original return sequences and all 34 callback boundaries.

The original frame sees ESP before a push retires; the retained source
updates ESP before its provider call. The two views preserve those separate
contracts. At 1CF0FD, the original i386 hardware also sets AF after
`SHL EAX,4` on the measured CPU, where the retained source model clears it.
The physical view runs the actual shift to obtain its flags; the retained
source view keeps its existing flag helper.

- [x] Pass both original frame sanitizer compilers. Each covers 34 parent
  sites, 100 genuine returns and 215 stopped prefixes, 3,037 full packets,
  and 167 descriptor entries including prefixes. The returned paths
  observe 96 actual descriptor calls and returns. The four reached sites
  in the real 1E5C80 lifecycle owner stay in scope; later gated resource
  paths do not run in this finite corpus. The original frame subreport
  passed before the outer wrapper hit a scratch path collision.
  `native-node-progress-query-both-20261110/frame/report.json`.
- [x] Pass both source sanitizer compilers: 500 normal calls, 3,755
  stopped prefixes and 212,606 complete packets each. Five deliberately
  broken native variants are rejected per compiler. This later frozen
  source copy matches all 52 owning inputs from the physical result.
  Both owners were removed.
  `native-node-progress-query-source-both-r2-20261110/report.json`.
- [x] Authenticate the complete current and freshly generated AIFrame
  binding. The existing local caller is 1D63C0 at 1D63D9. The direct
  public switch contains 1CF0E0; the live ownership predicate omits it.
  All four exact raw/prepared contexts are recorded, with the fresh file
  generated from 49 admitted references. Owner removed.
  `native-node-progress-query-owner-audit-r2-20261110/report.json`.
- [x] Qualify the exact private AIFrame candidate in four whole C++ files.
  It reverses exactly, preserves 690 neighboring bodies and rejects ten
  malformed contexts per file. Owner removed.
  `native-node-progress-query-binding-private-20261110/report.json`.
- [x] Trace the original local 1D63C0 caller across all 25 sites and 100
  returns. Its real child and descriptor execute 38 times, each returning
  to the actual 1D63DE boundary. Owner removed.
  `native-node-progress-ai-local-original-20261110/report.json`.
- [x] Exercise the actual local caller with the exact private candidate.
  The default, own-reference and unrelated-reference routes each match
  500 returns, 2,010 prefixes and 132,343 packets. The native route enters
  1,353 times by default and zero under its own reference. Owner removed.
  `native-node-progress-ai-local-candidate-20261110/report.json`.
- [x] Install the exact AIFrame-only default. Raw and prepared reversal
  preserve 690 other bodies and the same live ownership predicate.
  `native-node-progress-query-insertion-20261110/report.json`.
- [x] Pass the five registered installed checks, then both focused
  operand/catalog checks. The first focused assertion assumed the older
  descriptor binder accepted prepared source; that pre-existing refusal
  is identical after exact removal of this port. The corrected test
  checks both outcomes. Owners removed.
  `native-node-progress-query-installed-components-20261110/report.json`,
  `native-node-progress-query-installed-focused-r2-20261110/report.json`.
- [ ] Qualify the older descriptor's prepared catalog scope separately.
- [x] Authenticate both external Primary D3950 edges. Its unchanged
  original body covers all 37 sites and calls 1CF0E0 from D3998 and
  D39B3. The physical fixture enters the actual 1E3150 descriptor on
  all 28 calls.
  `native-d3950-function-progress-original-20261110/report.json`.
- [x] Admit only those two non-tail calls through the actual World
  dispatcher. The generic AIFrame ownership predicate stays unchanged.
  The installed linked replay passes all 100 rows, 347 ordered writes
  and 500 memory comparisons. Owner removed.
  `native-d3950-world-live-admission-installed-20261110/report.json`.
- [x] Install and qualify the D3950 native parent with its independent
  reference switch. Eight actual World routes match, with 14 real child
  returns at each call site per route and isolated parent/child controls.
  Fresh generation, catalog and permanent registration pass; all 12
  scratch owners removed.
  `native-d3950-parent-bridge-installed-20261110/closure.json`.

Inherited C9070, other images and coherent runtime remain open.
