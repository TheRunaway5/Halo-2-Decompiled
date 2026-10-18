# Bank b3 goldens provenance

Captured 2026-08-23 from real MCPX APU GP silicon (xbtest oracle, one fresh
boot per case). Fixed-slot layout: these cases use literal branch targets (the
assembler emits the short branch/jump encodings only for literals), so case
addresses must be predictable. Covers short/long/register forms of
Bcc/BRA/BSR/BScc/Jcc/JScc.
