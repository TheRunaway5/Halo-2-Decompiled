# Node-resource lifecycle (`1E5C80`)

Primary, AI-frame and AI-actor copies now share a typed native controller. It
checks both live enable bytes, resolves the handle's descriptor and node, calls
the actual first-node resource owner, then follows the backed successor chain.
Descriptor/node fields remain lazy; the successor is reread after each real
resource call. The core owns semantic values and phases. Its narrow ABI adapter
publishes retained GP/full flags and source cursor at memory/call boundaries,
preserves the caller's FP file, and keeps exact push/pop/RET4 behavior.

Whole original body and selected-resolver pins authenticate each binding.
Complete dispatcher pins authenticate real `1E3270`, `1E5AF0` and `126200`
handoffs. AI's nonlocal descriptor lookup uses the existing live-world primary
owner route. The component executes its actual primary lookup implementation,
without supplying a lookup or resource result. `126200` is an explicit genuine
entry stop in this parent proof; its full lifecycle is separate work.

GCC ASan/UBSan, GCC Release and Clang each pass 1,580 native transfers and 1,580
forced-reference transfers across three production copies plus the authenticated
fresh literal form. The 160 unchanged-i386 cases include 110 full returns and
50 genuine resource-owner entries. Another 235 before-parent-read stops compare
complete partial GP/full flags/XMM/FP/logical-80 state. All 76 original parent
sites run. Checks include ordered read/write addresses and widths, implicit
stack reads, full banks, signed handles, saved-register/descriptor/global-stack
aliases and caller live x87 depths up to eight. Seven unsupported protocols and
26 body/resolver/dispatcher mutations are rejected. Qualified parent paths emit
zero parent instruction callbacks; forced reference delivers actual callbacks.
Child pre-I/O cursor PCs are not claimed by this cohort.

Complete primary and AI-actor generation and raw/specialized syntax pass.
Current AI-frame generation needs 49 authenticated original references. The
older retained 48-reference command omits `27FA20`, `29B4A0`, `32A604` and
`32D9E0`; that incomplete dispatcher is rejected. Adding the existing ranged
script-target API original preserves all 691 current owners. Exact whole pins
cover identity-selected removal and the generator's numeric case order; there
is no general normalization of arbitrary body or dispatch drift. Both required
archived banks are reconstructed only in scratch, hash-checked and removed.

Evidence and cleanup:
`build/test-results/native-node-resource-lifecycle-closure-20261109/report.json`.
The current reproducible AI command is in
`build/test-results/native-node-resource-lifecycle-generator-commands-20261109/`.
No large artifacts are retained. Arbitrary observer/memory exceptions, physical
page-fault atomicity, complete real resource-owner returns and all-map/campaign
runtime qualification remain open. The frozen r4 runtime predates this port;
its 12,318,040 observations remain historical counts, not a measured speedup.
