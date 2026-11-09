# 2DBF74 kernel yield

The sole Primary TU1.5 wrapper now defaults to a typed native status-to-BOOL
kernel. Its import-table read still precedes the call-stack write; the real
required owner receives the actual target and complete caller ABI. The wrapper
then preserves the source CMP flags, ECX/EAX result and return-stack read.
`HALO2_REFERENCE_KERNEL_YIELD` retains the authenticated six-observation body.
The unchanged whole-body SHA is
`d6497ece40ee9bfbdf4c108fb878701f7a705a1316fdc3187eda7bb673d94f0b`.

The actual imported word at `41B69C` is `800000EE` (NtYieldExecution). World now
owns that exact call/return frame through `source_try_kernel_yield_tu15`.
The existing DeathStartup scheduler and World share the same host policy:
actual POSIX `sched_yield` success maps to NTSTATUS zero; Windows calls its real
NtYieldExecution entry. A yield never completes an animation request or writes
a ready byte. The existing DeathStartup scheduling controls and worker-stop
checks retain their order.

The official CMake sanitizer and Release tests passed 2/2 without skips. Each
gate compares 256 native/retained and 256 forced-reference calls, executing 512
actual host yields. Across 128 inputs, unchanged original i386 executes 256
segments to the genuine import entry and from the actual post-service ABI.
All six source sites, GP/full flags/XMM/MXCSR/x87 controls/status/live 80-bit
values, full banks and ordered accesses agree. Eight cases overlap the call
stack write with the already-read import word. Eight body/dispatcher/prefix
mutations, seven admission refusals and four platform frame/ownership controls
fail closed. Fresh Primary generation and whole-unit syntax also pass.

Evidence is in
`build/test-results/native-kernel-yield-official-20261109/`, with 4,032-byte
sanitizer receipt SHA
`a9b2b6d8849b057ae773e39a3751da195740be2a9230c1566fc29995867c5b3a`
and 3,966-byte Release receipt SHA
`86fdd8890cef1d0768f01bde0088831c42c922371090e6d6a4c014ba7015fcc4`.
The earlier unconfigured invocation correctly skipped and is retained separately
as `native-kernel-yield-official-unconfigured-20261109`; it supplies no acceptance.

This Linux cohort qualifies the source CPU wrapper around its real host service.
It does not execute the Xbox kernel or establish Xbox scheduling equivalence,
Windows acceptance, pending-request completion, or stack-write fault atomicity.
The ready-byte/promotion/yield controller at 1262B0 remains a separate open port.
All owned build/reference storage was removed; no large artifacts were retained.
