# Weapon routing and authored action lookup

The requested full live parity is **unfinished**. This change corrects the live
authored weapon-class lookup and compiles the original equip, action, reload,
firing-notification and drop routing graph. The live match still lacks the
source-created Unit body/manager/extension/local-pose ownership needed to run
that entire graph. It still uses the native terrain mover.

## Live change

`Cache::load_unit_weapon_action` and `NativeLocomotion` now apply the original
`1CFCE0` graph `+54` weapon-to-class mapping, followed by the original `29C6A0`
fallback iterator and signed `1E0F50` record lookup. The mapping preserves the
dual class `0400054B` and ignores authored NONE/any-class mapping values. Packed
inheritance remains signed and resolves the actual inherited graph. The pure
lookup module lives in `halo2_cache` so the cache does not depend back on systems.

The unchanged original TU1.1 executable supplies 1,584 queries over the actual
Spartan/Elite graphs, 33 weapons with an authored class, ordinary/dual classes,
six actions and base/alternate banks. The original constructs the query manager,
normalizes each request, then executes the lookup prefix through `1DEB60` or
`1DF670`. It stops before resource demand. Explicit query graph binding is a
fixture input; this is not a natural live Unit construction or resource test.

The native test calls the **actual Cache loader**, checks the decoded source
graph/index for all 258 available actions, and checks the genuine missing-action
result for the remaining 1,326 queries. Thirty-six queries require class
mapping. Linking the previous live Cache implementation against the same test
fails at query 936. Neither a default clip nor successful resource readiness is
added to make these comparisons pass.

## Original routing graph

`source_run_unit_weapon_routing_1_1` contains 57 complete TU1.1 bodies and 5,093
original instructions. Thirty-one bodies and 2,949 instructions extend the
earlier 26-body pickup graph. The common pickup bodies are static internals of
this independently callable graph; 57 is not a count of new owners.

The added graph includes:

- `CB060` weapon notifications and `E8A70` primary/secondary firing-layer requests.
- `E8740`, `E88B0`, `E8950` and their actual primary/secondary reload routing.
- `111500`, `115650` and `115800` identity/layer requests, with reached graph,
  transition, saved-local, resource and secondary-channel owners still required.
- `E9FE0`, `EA180`, `EA370` and the ordinary/secondary equip/put-away selectors.
- `CF8C0`, `CFB00`, `CFD80`, `FB490` and the original owner/parent/drop callers.
- The existing complete pickup, player carry-restriction and inventory selectors.

Production compiles native arithmetic, static calls, branches and verified
jump tables. It does not decode opcodes, execute the original executable, load
fixtures or install captured state. Reached external owners receive the actual
register/stack/FP ABI and the same retained storage. Missing owners throw;
mutations made before a lower-owner failure survive the failure.

The 165 declared original call cases return in 16 cases and stop at the first
required external owner in 149 cases. The comparison checks 2,659 ordered owning
stores, observed final memory, stack ownership, defined return bits and exact GP
ABI at each stop. There are **zero supplied successful lower results and zero
replayed lower calls** during the compared call. Original equipped-fixture
preparation remains explicitly declared; it is not live native construction.
The cases reach 831 unique instructions, not every branch in all 5,093 compiled
instructions. ASAN/UBSAN/leak checks also pass, including 149 absent-owner and
149 partial-owner ABI failure controls.

This does not prove completed admission, layer requests or drop physics: most
calls deliberately stop at an external owner. In particular, compiling `CFB00`
does not supply marker transforms, attachment changes, body motion or Havok.
Its newly compiled impulse arithmetic is not dynamically covered by a completed
live drop in this original-code comparison.

## Original images and validation

Authority is the owned TU1.1 executable, SHA-256
`aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`,
and Lockout cache, SHA-256
`14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`.
The owned TU1.5 executable is SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.

All 57 routing bodies and five normalization/lookup bodies have unique complete
normalized instruction/internal-branch matches in TU1.5. The comparison checks
the actually used routing jump tables and the byte-map indices selected by the
original action descriptors. Callees within the paired set have checked target
correspondence. External callees and mutable global bindings remain separate;
this static result does not execute a TU1.5 source-created Unit or prove a full
updated frame.

Both original exporters reproduce byte-identical fixtures. The CMake runtime
build is a coherent source freeze at `.local/weapon-routing-source-20261011`,
derived from the previous validated pickup freeze plus enumerated owned changes.
Its 9,564-file initial manifest is extended with this document and the validation
tools. It retains asset symlinks to owned source material and is not a portable
redistributable package. Build/runtime provenance is recorded in
`build-weapon-routing-20261011/weapon-routing-validation.json`.

Eight focused CTests pass: original action lookup/routing, original/retail
animation selection, native weapon actions/gameplay, original pickup and retail
inventory. Actual SDL probes exercise a map SMG pickup, independent dual-hand
firing/reload, grenade animation and offhand drop, plus a four-view grenade
prepare/release/debit/detonation sequence with authored world geometry in every
view. These prove bounded native behavior; they are not matched original
framebuffer or complete gameplay parity tests.

The shipping runtime also passes a ten-frame four-view TU1.5 run with the local
Vulkan validation layer and no validation messages. A separate standalone build
against the newer concurrent physics headers passes the 165 routing cases. That
bounded compatibility check does not validate every change in the shared dirty
checkout. The newer retained-call physics component cited below is separate
from this coherent runtime's source baseline.

Build-local launchers select the tested runtime. Shared root launchers can be
published by other active chats and are only observed, never overwritten or
claimed as validated by this report. The configured Discord SDK and its notices
remain with this build.

## Remaining live ownership

1. Compose actual object creation and type constructors into live Unit storage.
   The complete `B8A30`/`C52C0` owners already exist as components but still need
   the real allocator, object/world registration, model-node and lower-service
   chain. Derive appended offsets through that chain. Captured body bytes or
   observed offsets cannot replace it.
2. Run the source manager and extension against actual current/saved locals,
   source clock and resource admission. The extension overlay is `+9C`; it is
   separate from the manager's channels. Live presentation still bypasses the
   full original transition/local-pose path.
3. Route actual hand actions through those owners and compose original primary
   and secondary firing/reload layers, weights, events and node masks. Loading a
   fire/reload clip alone does not activate the third-person layer.
4. Replace the remaining native pickup/admission/drop decisions with the full
   source capability/action/inventory/world chain on that same real storage.
5. Connect live BSP/controller/body ownership to the source physics world. The
   newer physics component completes one retained `317650` world call; see
   `PHYSICS_WORLD_EVENT_ABI_RUNTIME_PORT_20261011.md`. It has not replaced the
   match mover. Full shape/motion/vehicle/ragdoll, multiframe and Xbox CPU
   estimate behavior remain unproven.
6. Complete grenade angular/contact/particle behavior and non-SMG
   charge/heat/lunge paths, then compare original rendered/action sequences
   across weapons, characters, maps, split-screen and LAN.

These are implementation and evidence gaps, not completed behavior. This change
does not mark weapons, grenades, pickups, dual wielding or Havok fully 1 to 1.
