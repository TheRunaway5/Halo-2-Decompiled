# Static instance accelerator and lighting contact order

The native static lighting contact path now traverses the original serialized
instance accelerator. Equal-distance instance contacts retain the original
first callback owner. This replaces the previous cluster-membership iteration
and its unresolved-tie rejection; it does not add dynamic-object contacts.

## Original producer and data

The source path is `1697C0 → 244980 → 2DF760 → 2DF720 → 301800 → 3009E0 →
244BB0 → 244C10 → 1691A0`. Literal `244980` instructions resolve an apparent
decompiler argument reversal: cluster `+AC` is passed directly as MoppInfo,
while the other argument is the local shape container. No runtime wrapper,
Havok reconstruction, or fitted callback order is needed.

Each cluster has a padded bytecode capacity at `+A8` and a MoppInfo pointer at
`+AC`. The info contains four source floats at `+0`, logical code length at
`+20`, and bytecode at `+30`. `301800` reads those floats directly, initializes
the grid factor as `scale / 65536`, and enters the bytecode. Its inverse scale
and float spills are retained. A complete serialized span is `48 + capacity`
bytes. Cluster membership at `+98/+9C` remains metadata and does not determine
callback order.

All 21 supplied maps contain 721 clusters, including 382 nonempty accelerator
banks and 40,956 logical bytecode bytes. Every logical length rounds to the
authored capacity at 16-byte alignment. `+A8 == 0` skips the source accelerator
call even if other cluster metadata exists.

`244BB0` creates a source shape record from the literal key. `244C10` consumes
its signed low 16 bits as an instance ordinal, deduplicates it, and performs
the existing original instance query with the collector's current fraction.
The fraction returned by that callback affects subsequent interpreter
branches. A precomputed traversal order would therefore be incorrect.

## Native module and integration

`source_instance_accelerator.hpp/.cpp` owns the immutable decoded info/code,
whole-program grammar inventory, and source visitor. The visitor takes the
actual origin, displacement, initial fraction, and a callback returning the
collector's updated fraction. Its result can retain keys and opcode offsets
for independent source comparison. Unsupported instructions and truncated
operands or targets throw `FormatError`; both branches are validated while
loading, including branches a current ray does not visit.

`Cache::load_bsp_surface_lighting` retains one optional accelerator per
cluster. `original_map_vector_contact` keeps its existing original main-BSP
leaf/cluster order, then dispatches each nonempty decoded bank. Instance
deduplication spans all visited clusters. The old secondary full-length query
used only to detect membership-order ambiguity has been removed. An optional
read-only `MapInstanceCallbackObserver` exposes the source `244C10` entry
state before deduplication for tests; normal callers omit it.

The interpreter is compiled with strict floating-point behavior and FMA
contraction disabled. Source branch order, separately rounded SSE products,
x87 double arithmetic, and explicit float spills are represented directly.

## Independent evidence

`tools/audit_instance_accelerator_original.py` executes the unchanged original
caller/interpreter/contact routines from the supplied XBE and authored cache
bytes. Its four observers only read cluster entry, opcode dispatch, callback
entry, and callback return. The frozen 29 real tie observations compare every
callback key, incoming/returned fraction, opcode/byte offset, and final
fraction. All match bit for bit. Headlong's real `3EAAABAD` tie visits
`[27, 5]` and retains owner 27; Triplicate's real tie visits `[49, 46]` and
retains owner 49. Other real Headlong rays have different original orders.

`tools/audit_instance_accelerator_banks_original.py` freezes all 382 authored
banks and 5,348 unchanged original traces on fourteen explicit diagnostic
rays per bank. All 35,556 opcode visits and their callback states match the
native visitor. These traces exercise all 60 opcode types reachable in the
supplied bytecode grammar. Elongation cluster 29 retains an empty
`FLT_MAX/-FLT_MAX` render-bounds sentinel while owning a valid accelerator;
its diagnostic rays use an explicitly labeled grid cube through its literal
MoppInfo, without replacing those authored bounds.

The independent full `1697C0` fixture covers 13,212 authored spawn-direction
rays and 17 distinct real tie rays across all 21 maps. The composed native
path matches all 13,229 contacts and all 2,641 original callback records.
Before this change, all spawn rays already matched; eight tie records had a
different or rejected owner. The new intended delta is original tie ownership.
The four focused tests pass in coherent normal and ASAN/UBSAN builds with leak
checking enabled. The integrated contact test compares 168,995 contact words
and 26,434 callback words, including 135 callbacks rejected by deduplication.
The real main-BSP-versus-instance equality census found no exact equal
fractions among 53,334 authored spawn/face rays; controlled equality fixtures
remain explicitly controlled inputs.

All execution fixtures set FPCW `027F` and MXCSR `1F80`, static query flags
`04800005`, test flags 9, and an explicit cold all-one breakable bank. They
replace no collision, math, or state service and do not advance a guest.
The original executable SHA-256 is
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

## Scope

The proof covers the supplied authored static instance accelerator and the
represented static map contact path under explicit cold state. Dynamic-object
registration/contact ordering, evolving breakable state, and broader live
query scheduling remain separate services. Contact and instruction agreement
do not establish full-frame or pixel appearance parity.
