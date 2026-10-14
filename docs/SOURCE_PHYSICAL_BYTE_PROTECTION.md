# Native source physical byte protection

Beaver Creek and Relic reach original `MmSetAddressProtect` with sub-page
metadata ranges. The old native adapter required both address and size to be
page-aligned, so valid requests stopped before object initialization.

The observed ranges are `8015CD6C,980,4` and `801E6814,E40,4`. After those
requests, the original producer requests protection2 at return `11F2DB`.
The pinned Cxbx kernel wrapper forwards byte address/length directly to
`VMManager::Protect`, which visits `GetPteAddress(addr)` through
`GetPteAddress(addr+Size-1)`. These are kernel compatibility ABI evidence,
not a hardware PTE/cache-coherency comparison.

The native owner now validates the original byte range against its actual
allocation, then rounds only the Linux `mprotect` range. Protection2 applies
host read-only access and registers matching page constraints on retained
source writes and mutable views. Protection4/404 restores writable access.
The source banks stay intact, preserving their addresses and borrowed constant
views. Mutable spans must not survive a protection transition. Native access
guards belong to the physical owner, not the memory-only checkpoint; complete
world restoration must restore that platform state separately.

The resource owner unregisters its guard before releasing its allocations.
Source virtual writes, mutable views, page rounding, unaffected pages, writable
restoration, allocation escape, unsupported flags and retired-address reuse
are covered by `source_physical_byte_protection_tests`.

The original red test and real map failures are retained in
`analysis/map-world-features/graphics-owner-20261013/`. The focused test now
passes. The independent physical-stage bank still matches **48 lifecycle
calls, 216 ordered writes, 38 actual allocation/protection requests and
153,796,608 final retained bytes**. Beaver Creek completes its first source
player tick; Relic advances past memory protection; subsequent original-owner failures
remain separate. Neither
observation establishes full gameplay or original physical-memory parity.

The delivery freeze uses the older memory interface of the installed baseline;
its unmodified implementation reproduces every allocated object section
exactly (`compatible-memory-baseline.json`). The same access guard is added to
that compatible body and to the current root implementation, preserving the
root's separate native-address work. No native class layout is changed by
these guards.

Pinned compatibility source hashes:

- `VMManager.cpp`: `40a7ae0c8d00f198f353c017ff7a8024ea22e1b91e8b7fa4cffe6e9306f2220e`
- `EmuKrnlMm.cpp`: `596efb2770c71f40a7f0773d35047f863e03c51feefa53d2e60f0394af109b99`

Both files are under
`analysis/multiplayer/integration/spawn-source/local-map-protection/primary-source/`.
