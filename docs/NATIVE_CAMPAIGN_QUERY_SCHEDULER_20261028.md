# Campaign query memberships and retirement, TU1.5

The semantic native module closes these generic actor/query owners:

| Original owner | Capability | Original sites |
| --- | --- | ---: |
| `2786D0` | Find an actor's slot for a query | 24 |
| `278A50` | Release a slot and publish its member state | 25 |
| `278720` | Select a free/expired/evictable actor slot | 61 |
| `2787F0` | Reserve a query member | 48 |
| `278930` | Publish query membership to its actors | 48 |
| `278B50` | Retire one behavior membership and the final query | 75 |

The 281-site closure uses the existing semantic `16E8A0` data-array delete
(62 sites) for its actual final pool retirement. That helper retains its
single production library owner, `halo2_native_tu15_data_array_delete`.
`cmake/CampaignNativeAiQueryScheduler.cmake` links that owner and compiles
the new source into `halo2_physics_world_runtime`.

The public API is `include/halo2/native_campaign_ai_query_scheduler_tu15.hpp`.
The implementation uses typed arguments, structured loops and lazy retained
memory reads. It contains no virtual CPU, instruction dispatcher, reconstructed
register file/stack, generated fallback, supplied dependency result or callback
success. No whole behavior/campaign readiness gate is enabled.

## Observable contract

The real actor pool is selected through `5020C8`; actor rows use their actual
`888`-byte layout. Four actor membership slots start at `+194`, with kind WORD,
member-index WORD, query datum DWORD and expiration DWORD. Query rows come
through `50EEFC`, use their `BC`-byte layout, and contain ten twelve-byte
members starting at `+4`, followed by the WORD reference count at `+7C`.

Slot selection compares expiration and clock ticks as signed DWORDs. It
chooses the first strictly lower signed WORD priority, starting from priority
8, and evicts only when that priority is strictly below the requested one.
Every occupied-slot comparison reloads the actual clock owner at `51DF54`.
Release publishes member actor NONE, rereads the actor slot's member index,
publishes the member flag, then clears the actor slot kind.

Reservation preserves the original extra read at member index ten when all
ten real members are occupied. It publishes actor, expiration bits, then
priority. The expiration value is transported as unchanged DWORD bits; the
original MOVSS performs no arithmetic. Its successful original SIMD scratch
is `[expirationBits,0,0,0]`, and its semantic return is AL boolean.

Broadcast captures each target actor row before selecting its slot, then
rereads the request's query/kind/expiration at their original publication
points. Final retirement preserves the post-publication member-index reread,
the count read/decrement/store/reread, salted actor validation, the separate
find-slot lookup, ordered slot clears, and fresh query/pool reads before the
actual shared delete. The original delete's caller DF=0 contract remains
required at an ABI binding. Shared-helper writes are compared through actual
memory; its existing API has no separate original-site observer.

## Native caller-owned request

The address overload preserves the complete request alias contract. The
typed `NativeCampaignAiQueryBroadcastRequest` overload shares the same
structured broadcast loop and lets a native `278460` caller own its private
request as a C++ value. It needs no emulated source stack.

Authenticated original producer/address instructions establish the real
caller layout: with entry SP `S`, private frame `F=S-2C`, the request fields
are kind at `F+20`, query at `F+24`, and expiration at `F+28`. The real
`278566` call passes request `calleeSP+28`, disjoint from heap actor/query
backing. Seven physical-original disjoint callee profiles, including that
actual caller stack layout, compare the typed overload against the same
ordered data reads, writes and retained banks after excluding the private
argument loads transported by the native value. Every profile checks that
no original publication overlaps its private request. The complete `278460`
entry is explicitly not claimed executed by this storage-contract proof.

## Independent proof

The offline validator executes unchanged owned TU1.5 i386 instructions on
the physical CPU under ptrace observation. The owned image SHA-256 is
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
There are no patched instructions, edited original states, supplied returns,
resumed unknown owners or runtime binary execution. The full original generic
calls include their actual nested scheduling/release/find calls and the final
shared-delete tail call.

GCC and Clang with ASan/UBSan compare 69 original calls, 9,248 original
instructions, 1,759 interleaved mutable reads and 448 stores. All 343 original
sites are authenticated and reached. Checks include complete retained banks,
read/store interleaving, individual before/after stores, typed slot/boolean
results, the actual preserved original GPR/stack contract, SIMD values,
occupied 80-bit x87 values and FP controls/status. Native integer calls also
preserve the host x87 control/status and MXCSR.

Six private semantic mutants are rejected: unsigned expiration comparison,
changing the strict priority tie, omitting the release-index reread, omitting
the full-bank tail read, caching the broadcast query, and caching the final
retirement query. The earlier FP assertion failure corrected only the proof's
MOVSS upper-lane expectation; the base algorithms required no correction.

Accepted reports are under
`analysis/campaign-native-query-scheduler-20261028/gcc-native-request-v4`,
`clang-sanitizers-v1`, and `module-integration-v1`. Missing owned reference
returns skip 77 with `passed:false` and `validation_not_run:true`; a wrong pin
fails. Mapped input profiles and masked FP controls are covered; unmapped
fault behavior and arbitrary dead-register/stack values are not claimed.

All compiler/CMake/original work uses the official disposable wrappers.
Raw banks, reference executables, private mutation sources and builds are
removed. Small reports, authenticated graphs, source hashes and cleanup
receipts remain; no large artifact is intentionally retained.

## Actual behavior cancellation remains open

Actual behavior `1B` uses descriptor `48ABE0`. Its `+50` cancellation callback
is nonzero `1BD8F0`; `278310` must execute it before generic `278B50`.
This module does not skip or supply that callback.

The authenticated finite frontier is `1BD8F0` (65 sites) to `275770`
(157 sites, seven call sites to six distinct owners: `C1E90`, `160A60`,
`6CA40`, `1D0430`, `275AB0`, `275610`) and `203AA0` (58 sites) to `213D30`
and `203D80`. Those actual callback owners, the real getter profile and the
complete `278460` caller remain separate open dependencies. Their graphs and
descriptor fields are retained beside the proof reports.
