# Native TU1.5 entity contact matching

Native1D8A20 matches actual contact material, other-object datum and signed
body-slot fields, retaining source loop order and fresh count/backing reads.
Its1D77D0 service receives only real manager/row data, the side byte, owning
datum and slot. The original constant-zero parameter remains explicit. No old
caller GP/SIMD postimage or control-frame image enters the kernel.

[Physical comparison](../build/test-results/native-contact-entity-processor-tu15-20261101/report.json)
passes146 cases under both compilers:107 complete returns,18 genuine child
entry stops and21 read-fault prefixes, with3,255 ordered reads and three
rejected mutations. The [source audit](../analysis/physics/native-contact-processor-context-20261101/report.json)
identifies the child's actual incoming EAX manager, ECX row and stack parameters;
its local source reads no incoming XMM before definition. This does not infer
transitive child postimages or qualify the whole child implementation.

The [world model](../build/test-results/native-world-contact-processor-binding-20261101/report.json)
executes two child boundaries while changing row backing/count and event
subject. Both survive private-callback-frame poison. The complete110 original
world cases,96 native force compositions and world mutation also pass.

Imported-MCC multiplayer v1464/04 driving passes its native processor gate:
1,355 parent calls and2,089 actual matched/updated rows, with old1D8A20 absent.
The actual driver camera renders349 frames, and the selected original frame
matches89,729,945 bytes. Rocket04 records7,289 actual contact updates and passes
its114,887,921-byte original comparison. Its actual vehicle death/debris
submission proof is recorded separately.
[Live report](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-04-summary.json).

1D77D0 remains a genuine lifted numeric/callback dependency and is explicitly
counted. The complete goal and several other activity gates stay open. The
earlier03 live refusals are preserved and not counted as successful coverage.
Disposable component and world builds were removed; small reports and source
pins remain in the [binding snapshot](../analysis/physics/native-contact-processor-live-binding-20261101/source-pins.json).
