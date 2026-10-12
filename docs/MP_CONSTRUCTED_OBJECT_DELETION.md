# Constructed object deletion and shared storage release

The original TU1.1 deactivation and deletion chain now returns for actual
original-created Lockout bipeds. Ten complete deletions include reconstruction
after retirement and multi-body heap unlinking. The original object count,
physics-component count and physical-body counter all return to zero. This
closes the earlier missing-context stops for this controlled source flow.

The native library now implements the final body-storage release owner and its
datum-pool retirement. Complete native object/world/physics/model teardown,
current-unit ownership, full authored round reset and later-round admission
remain unfinished. This work admits no additional variant or editable setting.

## Original flow and evidence

[The probe](../tools/oracle_constructed_object_deletion.py) uses the retained
whole compatibility thread producer and actual original process/game-arena,
object and physics storage owners. Selected original initializer callbacks run
in table order:19 players,20 view/control,25 physics constants,27 objects,
39 contrails,40 effects,58 full AI storage and59 physical storage. Their selected
map-reset callbacks run before the retained original activation/mask/world
sequence. These are selected API calls; the complete title/session startup and
savegame/thread-I/O producers are excluded.
Game type, session/options and clock inputs retain the earlier declared fixture
values; their normal session producers are also excluded.

Original `B8A30` creates the bodies from retained original normal-spawn
descriptors at authored Lockout starts0 and32. The player pool is source-created
and empty, and original cluster writers produce zero active/visible masks.
These are unowned inactive bipeds with real components, Havok bodies and world
membership. They do not establish an owned player's loadout, motion/contact,
damage/death or an ordinary multiplayer match.

The retained creation boundary still supplies unavailable optional attachment
effects (`178120`) and looping sound sources (`C10E0`), and supplies actual
cache/resource delivery and raw platform backing under its documented ABI.
The pinned compatibility lock/thread code is a host adaptation. It is not
original Xbox kernel execution or hardware parity.

During teardown, a guard stops every inherited supplied game/resource return
before its hook executes. Successful teardown uses no such service. Complete
original deactivation enters the registered callbacks; complete deletion removes
and destroys the actual Havok body, retires its component/model storage, unlinks
the body heap and frees the original object datum. The whole guest `.text`
section remains byte-identical to the supplied original executable.
The instruction trace records observed original addresses, including stopped
entries and retained platform boundaries; it is not an instruction-retirement
trace from Xbox hardware.

The two negative cases retain their actual source faults:

- Without the view/control owner, `18949A` reads NULL+58.
- With view/control and multiplayer AI reset but without effect storage, the
  final registered callback `17CE30` reaches `16D688`, reading NULL+38 from
  effect pool global4F1F0C. Original callback40 supplies that pool.

Successful flows delete and reconstruct each single biped, then repeat with
three bodies and different deletion orders. They cover linked heap head,
middle and tail retirement and pool high-water shrink across holes. They do
not execute the complete scenario/placement/map-reset tail of `15B460`.

## Shared native owner

| Owner | Retail | TU1.1 | TU1.5 |
|---|---|---|---|
|Deactivation |`B83B0`|`B92A0`|`BA3E0`|
|Deletion |`B8460`|`B9350`|`BA490`|
|Body-storage release |`BC300`|`BD1F0`|`BE330`|
|Datum retirement |`16BA40`|`16D4C0`|`16E8A0`|

[The native implementation](../src/object_storage_release.cpp) preserves the
header flag/pointer writes, heap free-byte count, previous/next links, root/tail
replacement and NULL-body branch. Pool retirement preserves the original
stride, optional BABA debug poison, availability bitmap, salt clear, signed
cursor/high-water checks and occupied count. Its memory provider is required;
missing backing throws. It does not substitute a handle clear for full teardown.

The fixture captures actual `BD1F0` entry/return storage during the complete
TU1.1 deletion flow, including the full1MB heap,2048 headers, bitmap and pool/heap
metadata. The ten normal records are replayed as explicit raw API inputs
against all three original images, with and without the debug-poison control.
Two more cases per image use the actual original datum allocator to create
header-only storage before calling the release owner. The native check compares
all66 complete raw outputs and every observed write's address, width, value
and order. Cross-image raw-bank reuse is API composition; full retail/TU1.5
biped construction and teardown are not established by it.

## Reproduction and remaining binding

```sh
.tools/python/bin/python tools/oracle_constructed_object_deletion.py
.tools/python/bin/python tools/oracle_constructed_object_deletion.py \
  --output analysis/multiplayer/game-variants/constructed-object-deletion/reproduced/report.json \
  --fixture analysis/multiplayer/game-variants/constructed-object-deletion/reproduced/fixture.json \
  --verify-fixture tests/fixtures/object_storage_release_original.json
cmake --build build --target object_storage_release_tests object_construction_tests \
  physics_body_disposal_tests round_lifecycle_tests halo2_native -j2
ctest --test-dir build \
  -R '^(original_object_storage_release|original_object_construction|original_physics_body_disposal|original_round_lifecycle)$' \
  --output-on-failure
.tools/python/bin/python tools/verify_constructed_object_deletion.py
```

Two independent complete original runs produce byte-identical reports and
fixtures. The four focused checks pass. Both new C++ translation units also
pass all66 cases when compiled directly with ASan/UBSan and leak detection.
The rebuilt production binary passes four-player90-tick Slayer startup; the
new release owner is unbound in that flow. The retained verifier checks every
raw blob hash, complete source correspondence, literal instruction byte and
artifact pin in `analysis/multiplayer/game-variants/constructed-object-deletion/verification.json`.

The native match still retains detached corpses and lacks the complete source
teardown callback binding. The shared release owner is available for that chain
and the constructor's failure cleanup; it is not installed as a successful
later-round reset service. Normal sound initialization, full world/scenario
reconstruction, preparation events, replication and actual subsequent spawning
must be bound and verified before admitting Elimination or other round settings.
All39 variants and supported editable tag/settings flows through complete local
and network matches remain the operational goal.
