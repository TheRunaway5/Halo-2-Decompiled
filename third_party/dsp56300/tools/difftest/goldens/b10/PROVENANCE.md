# Bank b10 goldens provenance

Captured 2026-08-24 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Seeded value sweeps: 72 deterministic cases from the
cases_valsweeps generator (random.Random(0x56D5B3 ^ 10), 75% boundary-pool /
25% uniform operands), each initializing x0/x1/y0/y1 and both accumulators
then chaining five silicon-safe single-word ALU/multiply/shift ops with SR
stashes (movec sr,r4/r5) between them so intermediate CCR state is observable.

The op pool draws only from instruction forms with existing silicon
goldens (banks b0-b9): no stack, loops, AGU updates, peripherals,
mode-SR writes, or out-of-range shift counts.

All 72 cases matched the JIT on the FIRST capture, in both
--mode=step and --mode=block - no emulator defects surfaced, which is
itself the result: the boundary-value cross-product over the
already-validated op set holds on silicon.
