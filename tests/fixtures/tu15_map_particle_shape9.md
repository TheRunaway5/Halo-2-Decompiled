# TU1.5 map particle shape 9

Generate the local, owned-asset fixtures from the repository root:

```sh
.tools/python/bin/python tools/oracle_particle_birth_shape9_tu15.py \
  --map assets/map-pack/maps/warlock.map \
  --output analysis/map-world-features/attachment-continuation-20261014/shape9-warlock
.tools/python/bin/python tools/oracle_particle_birth_shape9_tu15.py \
  --map assets/map-pack/maps/deltatap.map \
  --output analysis/map-world-features/attachment-continuation-20261014/shape9-deltatap
```

Configure `HALO2_TU15_GAMEPLAY_XBE` with the owned English TU1.5 executable.
The generator pins SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`
and runs whole entry `252480`, including its actual property helpers and
random helper `259B0`. There are no instruction patches or replaced services.
The tests skip with code 77 when their local fixtures or executable are absent.

All actual shape 9 emitters in both caches participate: 11 in Warlock and
13 in Deltatap, with 128 cases each. The 17-value property cache, initial
64-byte particle and RNG seed are declared inputs. Values include all zero,
all one and reproducible random vectors. Tests compare all 64 particle bytes
and the resulting seed, including unchanged fields.

The original branch at `252BD8` takes two angular draws and one line-position
draw. Polar is stored before cosine, while sine consumes the retained x87
intermediate. Its radial sine is stored before the azimuth products. The third
draw places Y between `0.f - radius` and `radius`, keeping X and Z positive
zero. The native implementation preserves these rounding and zero-sign steps.

This comparison uses Unicorn's existing RSQRTSS policy, matching the native
float-square-root/reciprocal boundary used by the existing birth tests. It does
not establish physical Xbox estimate bits, original allocation/emission timing,
attachment eligibility, completed value-cache production or final Xbox pixels.

The independent attachment seed-location bank is generated with:

```sh
.tools/python/bin/python tools/oracle_map_effect_seed_locations_tu15.py \
  --output analysis/map-world-features/attachment-continuation-20261014/seed-locations
```

It authenticates `C0E2E -> 179540 -> 1791C0` and compares 2,048 complete
`17B580` matrices, including its actual `11F9C0` callee. The seed helper runs
unchanged and supplies the actual `gravity`/`up` vectors and names. Node
matrices and completed world origins are declared inputs. Null, zero-scale,
unit-scale and scaled nodes are covered, with all matrix bytes compared.
The complete constructor and object origin producer remain separate owners.
