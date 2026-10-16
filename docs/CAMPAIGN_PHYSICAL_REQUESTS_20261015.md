# Campaign CPU simulation: physical request ownership

The campaign script probe reaches cinematic startup, whose `515E2C` state is
constructed by the complete original renderer initializer `12560`. That owner
reaches the D3D device constructor `3FF320`; the ordinary live renderer cannot
currently initialize inside the required memory budget. This remains a private
CPU simulation investigation, not a completed campaign or graphics startup.

Original TU1.5 device creation reaches `405CF0 -> 2DA8B0 -> 2DB45A` and requests
96 bytes with lowest address zero, highest address `FFFFFFFF`, alignment `1000`
and protection 4. Later it requests `100000` bytes with protection `404`.
These are observed original frames, not assumed SDK defaults. The unrestricted
upper bound also appears in the primary compatibility implementation's ordinary
[MmAllocateContiguousMemory wrapper](https://github.com/Cxbx-Reloaded/Cxbx-Reloaded-legacy/blob/master/src/core/kernel/exports/EmuKrnlMm.cpp).

The published memory owner accepts that upper-bound sentinel, keeps its existing
physical-token namespace bounded, and admits the reached writable/write-combined
requests. Anonymous native pages keep the host cache policy, as the existing
protection owner does; this does not establish Xbox GPU cache coherency.
The original allocation/protection wrappers retain their canonical owner.
`PhysicsLiveWorldTu15` routes their actual TU1.5 entry addresses through the same
arena resource owner. These page resources remain available after the startup
worker joins, while startup execution keeps its existing worker check.

`source_physical_allocation_requests_tests` covers original bounded allocation,
unrestricted/repeated requests, nonoverlap with the original arena, write-combined
backing, retained bytes, checkpoint ownership, rejection, namespace exhaustion
and RAII retirement. Both unrestricted allocation and write-combined allocation
have failing-before/passing-after native proofs. The freshly compiled published
implementation passes the same regression test. No D3D device result is supplied.

The existing original physical-stage comparator now reads expected backing files
in 64 KiB chunks instead of allocating another full arena. It still compares
all original bytes. Its unchanged corpus passes **48 lifecycle calls, 216 ordered
writes, 38 host resource requests and 153,796,608 final retained bytes** against
the published implementation, with a measured peak of 62,914,560 bytes and zero
swap. All native runs use the shared serial memory guard, with a 60 MiB hard cap
and independently enforced 64 MiB measured ceiling.

Evidence is in
`analysis/tags/gameplay-validation-20261014/campaign-hs-frame/`:

- `published-physical-verification-v1/report.json`: fresh published physical/arena
  compilation, unchanged public memory headers, separate current-root world
  syntax check. This is not a full current-root engine build.
- `published-physical-regression-v1/`: successful bounded regression.
- `published-original-physical-streamed-v1/`: successful complete original
  physical-stage corpus and kernel memory accounting.
- `physical-request-before-v3/`, `physical-request-after-v3/`,
  `physical-request-before-v4/`, `physical-request-after-v4/`: negative/positive
  allocation proofs.
- `actual-gpu-admission-v4/`: contained live-renderer admission failure at the
  memory cap. Do not repeat it without a substantive resource change or raise
  the simulation cap.

The private CPU renderer probe still requires additional platform ownership.
Full original HS-frame execution, mission progression, BSP/checkpoint/death
paths, and campaign presentation remain unfinished. The campaign goal is active.
