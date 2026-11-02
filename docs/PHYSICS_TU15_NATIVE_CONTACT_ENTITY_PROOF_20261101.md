# Native TU1.5 contact-entity notification

Native `281E10` now owns contact-entity selection and its closed `1D7530/1D75B0`
property queries. It follows the actual payload parents, classifies the two
entities, reads their datum/slot properties in original order, observes the
current event subject and component flags, and dispatches the genuine typed
`1D8A20` service. The latter remains an unconverted child, explicitly counted
as residual work.

Both compilers pass231 physical original cases:170 full returns,25 genuine
contact-processor entry stops and36 read-fault prefixes. The cohort compares
3,500 ordered reads, persistent state and FP, with three rejected mutations.
[Component report](../build/test-results/native-contact-entity-notification-tu15-20261101/report.json).
The world model verifies that the actual processor receives implicit ESI event
data plus five parameters, and poisons its private stack while the event and
cached body metadata remain live.
[World report](../build/test-results/native-world-contact-entity-binding-20261101/report.json).
[Source context audit](../analysis/physics/native-contact-entity-notification-context-20261101/report.json).

Imported-MCC multiplayer v1464/00 exercises1,347 native calls/5,388 property
queries/1,347 genuine child dispatches while driving, and2,422/9,688/2,422 in
the grenade case. Missing-pair/disabled counts are zero and the old281E10 owner
is absent. Actual driver-camera frames number424 and419 respectively, with
the declared0.6-radian look turn. The selected strict original frames match
89,729,945 and114,887,921 bytes.
[Live results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-00-summary.json).

v1464/01 additionally passes explicit entity and dispatch gates with3,096
native calls/12,384 properties/3,096 genuine child dispatches and a strict
tick1200 original comparison. Its typed contact observer runs48 times but
captures no eligible dynamic pair; that separate gate remains unexercised.
[Results](../analysis/multiplayer/physics-live-tu15-20261012/mcc-native-contact-entity-recovery-original-v1464-20261101/iteration-01-summary.json).
These positive boundaries do not make the whole multiplayer suite green:
full destruction, several activity gates and native-only closure remain open.
