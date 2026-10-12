# TU1.5 renderer storage producer

The native renderer storage prefix now follows the original TU1.5 producer:
it constructs both parameter arrays, acquires fresh anonymous host pages,
publishes the renderer base, and enters its actual descriptor constructor.
Execution stops at the constructor's first ordinary process-heap allocation,
with its complete pending source call frame. No allocation result, descriptor
completion or usable renderer is supplied.

Three complete bodies are paired against both owned executables and compiled
as native C++: canonical133760→actual134B00,1360F0→137460 and165740→166B20,
192 instruction sites altogether. TU1.1 addresses are internal correspondence
IDs; English TU1.5 is the sole execution target. The native path also uses the
previously verified raw-page wrapper and resource owner. Production contains
no opcode fetch, decoder or original instruction execution.

The independent original-only corpus has12 fresh cases: three declared native
address layouts, two flag inputs and two stack alignments. It executes744
original instructions across62 sites, retaining60 ordered non-stack stores,
12 actual host allocations and20,263,200 final bank bytes. Native comparisons
check the complete trace, stores, real initially zeroed allocation bytes,
whole banks, pending eight-byte frame, GP/XMM, applicable flags and FP state.
Independent fresh missing-page-owner cases retain the earlier original raw
allocation call boundary. Native page retirement is checked separately.

The renderer requests452E8h bytes, rounded by the declared native resource
owner to46000h. Its first descriptor requests18h bytes from actual32A3BB at
13747E, with pushed return137483. The ordinary process allocator remains a
required owner; raw pages are not substituted for it. Only the executed prefix
is dynamically verified. The remaining constructor paths are statically
compiled and remain unverified in this corpus.

Both original captures, recovery and generation repeat byte-identically.
Fresh Release, ASAN/UBSAN/leak and TSan builds each pass both selected CTest
checks using the actual production CMake modules and frozen compiler header
closure. Evidence is under
`analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012/`.
Validation seal SHA-256:
`ad52a38c92306c01970a287fdcdae95be3c95f6eee83c166c97814ee6631b71d`.

The complete producer return, renderer overflow join, occupied world setup,
live tick integration and multiplayer acceptance remain open. No launcher
build or live Lockout lift/vehicle fix is established by this comparison.
