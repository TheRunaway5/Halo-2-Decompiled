# Bounded native grenade callback kernels

`include/halo2/unit_grenade.hpp`, `src/unit_grenade.cpp` and `tests/unit_grenade_tests.cpp` translate original type22 callback state and construction-request logic. They do not install a live Q handler. The original begin callback still stops at its effect-pool dependency, as recorded in [the immutable begin freeze](MP_GRENADE_BOUNDED_PHASE_SOURCE_RESEARCH.md). No effects, animation admission, allocation, attachment, voice, graphics, release or hardware success is supplied.

The dynamic authority is user-owned disc1.1 XBE SHA `aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377`, with original Lockout SHA `14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9`. Complete code/operand correspondence of the grenade owners to1.5 is independent earlier evidence; this new callback composition dynamically executes1.1 only.

## Owning context and execution boundary

`tools/oracle_mp_grenade_callbacks_1_1.py` reproduces the original source-created player/unit/SMG and actual partial throw begin unchanged, then retains the complete guest memory and CPU state. Player `EC700000`, unit `E26F0000` and SMG `E2700001` remain genuine allocator/join/loadout outputs. `E95E0` has selected actual clip219 and written phase1/counter0/release8, but its effect callback has not returned and active-word bit22 has not been set.

The new experiment separately invokes actual callback ABIs `E99B0` and `E8D60` against that produced state. This is a **declared direct callback composition**, not a normal admitted action or proof that original dispatch would invoke these callbacks before begin returns. One unmodified `E99B0` call returns AL1 and increments counter0→1. An unmodified direct `E8D60` prefix debits the real frag count2→1 and builds an original projectile request. Separately named one-byte phase/counter/timer/current/count controls exercise signed boundaries; they are not claimed naturally produced gameplay state.

Actual original marker and generic-descriptor producers execute. At original `B8A30` entry, the observer records the complete C4 request and stops without replacing its return or changing instructions. This intentionally leaves real allocation/attachment and any resulting phase2/held-object state unexecuted. Other inherited fixed-clock, local-session, resource, feedback, raw-storage and platform boundaries remain explicit.

## Native contract

- The full44h action component is independent of the native player State. Initialization mirrors source `C52C0`/`BD270`: zero storage, DWORDNONE at+10/+18/+2C and **WORD**NONE at+14. Unrelated bytes are retained thereafter.
- `source_grenade_begin_channel_prefix` ports only `E9896..E98C6`, after a real channel/duration provider has returned. Its tick argument is the actual `1CA8D0`/`148160` result, not a default8. It does not admit begin, deliver feedback/scope/effects, set the active bit or imply callback completion.
- `source_grenade_update` ports complete `E99B0` counter/phase branching. Raw counter+0E always increments with byte wrapping first. Phase and counter comparisons are signed. Phase1 requests `E8D60` when the incremented signed counter>=2; phase2 requests `E8F30` when counter>=signed release timer; phase3 queries `115800` identifiers `0D000021` then, only after an actual false result, `1000006C`. A true result retains the phase and returns1. Both actual false replies clear phase0 and return0. An unavailable reply is distinct from false. The dynamically tested phase3 case retains the real active throw channel; the false-result branch has complete original instruction evidence but is not claimed dynamically covered here.
- `source_grenade_release_empty` ports only the complete original held-NONE path: phase becomes3. A held object returns unavailable to the caller rather than clearing ownership or inventing a detach/launch result.
- `source_grenade_prepare_prefix` ports `E8D60` through the original marker request. CurrentNONE gives phase3. With actual unit DWORD+12C=NONE and component byte+0D=0, the count comparison is signed; positive counts decrement before `A87E0(unit,400000)` and before `B9C20(unit,09000100,1)`. Count0/negative give phase3. A known absent original globals definition can still follow a prior debit; an unavailable globals read stops before debit. No refund is inferred for later allocation failure. Provider inputs are the source fields, not a guessed local/remote role or native grenade mode.
- `patch_source_grenade_creation_request` starts with the **actual B8820-produced C4 descriptor** and actual marker point. It modifies only DWORD flags+18 (`|4`), forward+28 from unit+168, perpendicular/normalized up+34 and marker position+1C. These offsets are verified against the whole request and exact caller stack operands. Every other byte is retained, including source-owned location/scratch bytes whose values differ between original call contexts; no fabricated zero-filled descriptor substitutes for original generic initialization.
- Original `11E720` selects the least-magnitude aim component using its ordered comparisons, then forms a perpendicular vector with separate float32 subtracts. Original `30BD0` accumulates squared components and square root under the measured53-bit x87 environment, spills the length to float32, divides original one by that spill and applies separate float32 multiplies. Constants are explicit provider inputs: one at original1.1 `465030`, epsilon at `46504C`. The three dynamically checked request cases use actual source Spartan aim/marker data; other orientations and host FP environments are not claimed independently measured by this suite.

The request contains actual frag projectile `E7A80623`; it is an allocation request rather than an allocated grenade. The native module neither manufactures its datum nor launches it. Real owner/parent, location, attachment, region/world motion, fuse, impact/explosion and destruction consumers remain separate integration requirements.

## Independent comparison and usage

Canonical reports are `analysis/multiplayer/integration/grenade-callbacks/measured/report.json` and `repeat/report.json`. Both independent original runs match byte-for-byte. Fifteen callbacks include signed counter wrap, signed timer comparisons, actual throw-channel retention, selected count debit and NONE/zero/negative-count exits. Three original `B8A30` request boundaries retain complete196-byte records.

`tools/export_mp_grenade_callback_fixtures.py` exports20 rows: constructor, actual partial begin,15 callback transitions and3 construction requests. Native tests compare all44 action bytes, actual counts, defined AL returns, callback/query order and all196 descriptor bytes. `E8D60` is void; incidental AL is explicitly excluded from its return contract. Tests run with `-fno-fast-math -ffp-contract=off -frounding-math`, normally and under AddressSanitizer/UndefinedBehaviorSanitizer. No shared CMake/runtime/Simulation/frontend/shader files change.

```sh
.tools/python/bin/python tools/oracle_mp_grenade_callbacks_1_1.py --output analysis/multiplayer/integration/grenade-callbacks/measured/report.json
.tools/python/bin/python tools/oracle_mp_grenade_callbacks_1_1.py --output analysis/multiplayer/integration/grenade-callbacks/repeat/report.json
.tools/python/bin/python tools/export_mp_grenade_callback_fixtures.py --output analysis/multiplayer/integration/grenade-callbacks/measured/fixtures.tsv
g++ -std=c++20 -O2 -fno-fast-math -ffp-contract=off -frounding-math -Iinclude src/unit_grenade.cpp tests/unit_grenade_tests.cpp -o /tmp/halo2-unit-grenade-tests
/tmp/halo2-unit-grenade-tests analysis/multiplayer/integration/grenade-callbacks/measured/fixtures.tsv
```

The next live caller must distinguish a pending/unreturned begin from an admitted action, preserve request order and supply genuinely owned animation/effect/marker/generic-descriptor/allocation/attachment services. The module makes these boundaries explicit; passing this suite does not close them or prove working grenades in a native match.
