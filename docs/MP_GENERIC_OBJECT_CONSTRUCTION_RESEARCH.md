# Generic object construction: complete direct owner

`construct_source_object` ports the complete TU1.1 `B8A30` direct body. Its
614 original instructions uniquely match authenticated u5English `B9B70`,
including every internal branch and the relocated 43 direct calls. This is a
caller port with mandatory lower owners. It does **not** create a live native
Unit by replaying a source body, or establish grenade gameplay.

The new implementation is in `include/halo2/object_construction.hpp` and
`src/object_construction.cpp`; its registered test is
`original_object_construction`. All evidence is external/ignored under
`analysis/multiplayer/integration/spawn-source/generic-object-constructor`.

## Executed authority and differential

Two independent unchanged original executions produce identical
`measured-v3/report.json` / `repeated-v3/report.json`, SHA-256
`8ffdb075f939ac69eb3c2c95171c0809d0cceba7e4462e471660ae8dcdddc881`.
Preparation retains the immutable PlayerTrace platform/options/resource
boundaries. Its first four player stages reproduce the frozen positive
player-visibility case exactly. No instruction patch, newly supplied success,
body offset, pose, contact, capacity or readiness value was added.

| Original path | Result and ownership |
|---|---|
| Natural biped | Returns `E26F0000`; source lower constructors grow its body to 5750 bytes. |
| Natural SMG | Returns `E2700001`; source constructors produce its actual weapon body. |
| Real allocator pressure | 905 complete `BD170(1124)` attempts reach an actual finite-heap failure; `B8A30` then returns `NONE` immediately after allocation failure. |
| Release one pressure body | Complete original `BD1F0` frees one real raw allocation. A fresh base allocation returns `E5FB0389`; the third `BD270` extension (descriptor `124`, size `60`) genuinely fails. `B8A30` invokes `BD1F0` and returns `NONE`. |

The pressure bodies are raw allocations, not constructed or live Units.
Source capacities and allocator results remain unchanged. Earlier v2 traces
and the interrupted Python observer experiment are preserved as draft
observations; v3 is the final four-case contract.

The native test compares 83 reached lower-call identities, their defined
owning registers/arguments and 194 ordered retained stores. Final snapshots
contain 2,158,568 bytes; 2,158,504 are compared after the 64 saved-register
bytes described below are excluded. Every callback boundary and final state checks the full
object-pool header, 2048 object headers, world record, request, all allocated bodies
and private frame. Only private frame bytes `00..0F` (saved registers and
call-stack residue) are excluded; `10..C7`, including the temporary manager,
are checked. Callback before/after/EAX/MXCSR replay exists **only in the
test**, and is not a production provider or proof that every lower owner has
been ported. Missing production callbacks throw/stop.

Normal optimized, strict warning, Clang ASan/UBSan and registered production
normal/GCC ASan/UBSan checks pass. Sixty additional declared ISA controls run
the exact original `COMISS xmm1,xmm0` opcode against zeros, normals,
subnormals, infinities and NaNs with masked source SSE contexts. They compare
the returned predicate and full MXCSR, including DAZ/status behavior. These
are instruction controls, not new world-construction cases. Unmasked
exceptions stop before subsequent stores; no Xbox exception-delivery ABI is
claimed.

## Owning state and exact ordering

The input is an actual retained `C4` request and pointer slots for the
source-owned object pool, tag table, object-world record and clock, plus a
private `C8` invocation frame. No initial frame/body zero fill is supplied.
The provider resolves every opaque 32-bit token to bounded owned bytes;
original signed component offsets are read from the constructor-produced
body. The lower allocation/growth owner may relocate it. This caller
rereads pool/header/body pointers at the original refresh points and retains
the other pointers for exactly the original portions of the call.

The direct sequence is:

1. Gate/prepare the request (`10CBD0`), resolve definition/model/type, invoke
   `148360` for the original type mask `1883`, then request the literal type
   record's unsigned `+8` word through `BD170`.
2. Initialize the header, identity/counter, authored root/bases/velocities,
   flags/owner fields and sentinels. `C0890` receives actual output locals;
   its defined AL controls bit 31, independently of its output datums.
3. Run the optional old active-object cleanup and the temporary animation
   manager constructor/bind/reset. Derive counts from actual model metadata.
4. Append `11C,120,124,114,118,110,10C,128` in that order. Sizes use original
   16-bit truncation and 32-bit wrapping; `118` is **count × 10 decimal**.
   Matrix banks use alignment 4. Any failure skips later appends/admission,
   records failure and calls `BD1F0`.
5. Run `1C7290`, refresh the body, reset/rebind its genuine appended manager,
   clear the original `11C` bank, then invoke `10CC20`. False calls `10CD80`
   before `BD1F0`; this branch is implemented but was not a natural v3 outcome.
6. Preserve bit 17 and run owner/color/region setup, object data query,
   current-node/bounds/location/world hooks, update marking and type post
   construction. The `D7160` scalar words are copied without arithmetic;
   each original COMISS is evaluated in order with the supplied actual
   mutable SSE context, before its following stores.
7. Run the optional authored creation effect, `2E1C0`, restore bit 17,
   `10D520`, and the conditional `B9710` / `B9430` activation. Return the full
   salted datum. Failed selection/allocation returns `FFFFFFFF`.

The direct owner does not add a salted admission gate. Its original low16
indexing is retained with explicit host bounds/overflow checks. Required
metadata and memory errors stop; they never become `NONE` or success. The
test also checks unavailable-owner, wrapped-frame and truncated-backing stops.

## Immutable selected-image operands

`u5/operand-packet.json`, SHA-256
`4a4c5e5b2a11ae5006350e29f5fa339e275c49a5641f01b52fc7c9aa9de6026e`,
contains only the two literal sentinel bytes. Original `46FB64` maps to u5
`474C24`, raw `FFFF`. The byte references are `B8B33` / `B9C73`; the word
references are `B8C68` / `B9DA8`. The existing original one-word import covers
`465030` / `46A0D0`. Production receives these as
`ObjectConstructionOperands{one_bits,sentinel_word}`; no payload is hardcoded
in the native owner. Type base extents remain a mandatory actual selected
image service, not `sizeof(Unit)` or inferred body sizes.

TU1.1 executable SHA-256:
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`.
Authenticated u5English SHA-256:
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
The full u5 comparison is static original-byte correspondence, not u5 world
execution. `REPRODUCE.md` records exact commands and dependency scope.

## Next live composition boundary

The required lower callback interface carries only actual owning inputs and
defined EAX/AL. It shares mutable SSE state with lower owners. Undefined
caller-saved registers are not invented. Complete native `B8820/BBCB0`, type
dispatch, `C0890`, manager and unit append/constructor owners can now be
composed against genuinely allocated storage. This needs a distinct test
which calls those owners and stops at its first unavailable dependency;
the upper-caller replay is not that composition.

In particular, no complete native `DDB40` or `1C7050` biped physical-component
constructor was available at this checkpoint. Existing physics body helpers
start with an already allocated component and do not supply one. Genuine
header/body heap allocation/free ownership, preparation/resource admission,
type constructors, current-node/pose/world insertion and activation providers
must return their actual outcomes. A decoded animation cache, final body
copy, invented offset, successful callback or active byte cannot replace
these owners. Q delivery and projectile/world lifecycle remain unconnected
until that composition exists. No runtime, renderer, shader, release or GPU
work was performed in this scope.
