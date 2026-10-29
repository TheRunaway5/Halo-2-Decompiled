# Native TU1.5 path-workspace and scratch-arena ownership

`src/campaign/ai/native_campaign_ai_path_arena_tu15.cpp` implements the actual
two-descriptor workspace lease/release (`1D2D90`/`1D2E30`), scratch allocator
reconstruction (`149680`) and authenticated `233120(delete=false)` retirement
inside `1498E0`. These are actual retained ownership transitions, not mutex
operations, host allocations, successful allocation fixtures or lifted calls.
The public header is `include/halo2/native_campaign_ai_path_arena_tu15.hpp` and
the module is `cmake/CampaignNativeAiPathArena.cmake`. It now includes the
semantic resource-retirement module and exposes an overload taking its required
native page-protection owner. No numeric router is added.

The initializer owns exactly two descriptors at `50208C`/`502098`, each with
actual `22974`-byte backing. It caches whether a primary allocator existed,
retires that allocator at its true phase, then reads actual `51DF44` and writes
both descriptors. Acquire marks a selected descriptor used **before** reading
its returned pointer. No free descriptor returns zero without inventing a
workspace. Zero backing/pointer results retain their original marker effects.

Release rereads each active descriptor and compares its real backing identity.
It clears all matching descriptors, counts unrelated live leases and restores
the allocator only when none remain and the actual saved ownership flag is
nonzero. Duplicate identities and unmatched releases keep the original behavior;
no new membership or availability policy is introduced.

`149680` reads actual scratch backing, publishes its active marker, aligns the
backing to16 bytes and reconstructs the actual descriptor at `48605C+105410`.
It retains wrapped-null pointer behavior, exact field widths/order, original
`0FFFFFFC` size mask and final vtable/global publications. It supplies no new
storage or guessed pointer. Existing native startup storage owns the backing.

Allocator retirement reads the actual vtable and slot+20. Authenticated selector
`233120` is handled natively with its real delete=false argument: publish base
vtable `45F4A0`, then the caller clears its globals/marker. Foreign selectors
throw a typed required-owner error with actual selector/receiver identity.
A secondary allocation is retired similarly, its backing identity is read and
`486074` is cleared before the actual `130030` phase. The overload with the
required typed page owner executes native resource retirement, then clears
`486060` at149921 only after success. The original overload preserves its
explicit130030 boundary when no page owner is provided. Associated GPU records
and foreign selectors retain actual required-owner boundaries. See
[the resource proof](CAMPAIGN_NATIVE_AI_RESOURCE_RETIREMENT_TU15.md).

`tools/oracle_native_campaign_ai_path_arena_tu15.py` executes unchanged physical
i386 code from authenticated English TU1.5, SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
No instruction, dependency return or resumed failure outcome is supplied.
Required-owner cases execute the actual direct/indirect call instruction and
stop at the real missing entry before its body executes. Their ordinary return
word remains a distinct stop address, so a normal return cannot impersonate
reaching a required callee. All preceding reads and stores are compared.

The required fixture is
`tests/fixtures/native_campaign_ai_path_arena_tu15.json.gz`. Its 263 cases cover
completed ownership operations, real required-owner entry boundaries and75
read/store/observer prefixes, with126 executed sites and no unexpected coverage
gap. This initial fixture excluded the two secondary-retirement return sites and
three delete=true sites. The new composed proof below covers the return sites;
the actual `1498E0` caller never requests delete=true.
Cases cover every backing alignment, wrapped-null/unaligned allocator targets,
all descriptor occupancy combinations, duplicate identities, no backing,
aliases between primary/secondary allocator identities, destructor selectors,
refusal, partial initialization and partial publication. Independent original
repeat evidence is byte-identical.

GCC normal and ASan/UBSan each passed1/1 CTest; Clang passed6/6. Five actual
mutations were rejected: wrong slice stride, a third descriptor, reading the
returned pointer before marking it used, restoring with another live lease
and erasing the required secondary owner. Tests compare exact workspace
identities, persistent bytes, ordered reads/stores/sites/widths, actual missing
owner identities, complete source FP and unrelated host FP preservation.

`analysis/campaign-native-ai-path-arena-20261028/proof.json` retains exact hashes,
commands, counts and verified cleanup. All original ELF/observer, raw banks and
compiler products were removed. Only the compact required fixture and bounded
receipts remain. Prior vector/query/geometry/count proofs were unchanged.

The current qualification is
`analysis/campaign-native-ai-path-arena-retirement-20261028/proof.json`.
Its312 composed original cases execute236 sites, including complete secondary
retirement before descriptor publication, with9,432 ordered reads,5,857 stores
and246 real page-protection operations. A separate actual0x452E8 workspace
allocation remains owned and writable; the retiring resource allocation keeps
its original lifetime and exact protection state. Unknown/GPU owners and three
true unmapped reads preserve every preceding publication. The independent
original repeat is byte-identical.

Current GCC and ASan/UBSan each pass3/3: all263 unchanged prior arena cases,
327 resource cases and312 composed cases. Clang passes9/9 with six additional
arena mutations rejected: missing/early backing publication, omitted secondary
retirement, dropped required protection service, skipping a secondary without a
primary and wrong workspace slice stride. The three cohorts total902 original
calls and297 unique sites. This supersedes the initial proof's current-source
pins; that report remains historical evidence for the initial profile.

The new required composed fixture is
`tests/fixtures/native_campaign_ai_path_arena_retirement_tu15.json.gz`,934,446
bytes, SHA-256
`192ef545ac076315e8689748b9761bf8244015fadd92c98a17383f724e1a8b78`.
All owned original/compile/build scratch was verified removed. Only required
reference fixtures and bounded reports remain. Native secondary teardown now
has a genuine owner for the authored scratch profile. Associated GPU resource
retirement, deleting destructor32B479, full1C9770/27B340/200D30 search/update and
generic native campaign acceptance remain open.
