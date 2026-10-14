# Original melee contact presentation routing

Generate the ignored local fixture using user-owned Xbox TU1.1 and TU1.5
executables:

```sh
.tools/python/bin/python tools/oracle_melee_impact_routing.py \
  --tu11 '/path/to/owned/TU1.1/default.xbe.03' \
  --tu15 '/path/to/owned/u5English/default.xbe' \
  --fixture tests/fixtures/melee_impact_routing_original.tsv \
  --report tests/fixtures/melee_impact_routing_original.provenance.json
```

The oracle executes complete original `D0340`/`D13D0` and the original melee-only
predicate/root lookup. Object/header/tag backing, held-item parent/root and
vectors are explicit ABI inputs. Only lower material-effect and object-sound
creation requests are recorded; collision, sound admission/mixing, particles
and rendered appearance are not supplied or verified.

The fixture contains 364 actual Lockout Sword/SMG/global-material combinations,
which must agree across both images. Columns are weapon hex, material decimal,
request count, then ordered kind/tag hex pairs. `F` records a material-effect
bank request; `S` records a sound request. It does not imply that an `F` bank
contains particles. Lockout sword action 15 has twelve sound rows and zero
visual rows.

`native_melee_and_sword_contact_effects` consumes this fixture and skips with
code 77 if it has not been generated. Original executable bytes and generated
payloads are not committed.
