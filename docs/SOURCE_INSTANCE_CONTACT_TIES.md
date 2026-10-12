# Instance contact ties and source ownership

The native source-BSP lighting query now uses the original authored MOPP
accelerator order. The retained 17 real Headlong/Triplicate tied rays select the
same owner as unchanged original `1697C0`; they no longer require an unresolved
ownership result. See `SOURCE_INSTANCE_ACCELERATOR_CONTACTS.md` for the complete
21-map contact/callback proof. The former one-ULP false-admission diagnosis is
retained below because it explains why membership order was insufficient.

## Literal source path

Retail `1697C0` first traverses the main collision BSP using `1DE630`. It visits
that traversal's leaf/cluster order, then calls `244980` for each eligible
cluster. The cluster's `+A8/+AC` collision MOPP data drives
`2DF720 -> 301800 -> 3009E0`. Each accelerator shape callback runs `244C10`:

- Its shape payload `+4` selects the signed instance index.
- `4E7C1C/4E7C20` deduplicate instances before querying.
- `1691A0` performs `1689B0` admission, `11E5E0` sphere rejection,
  `141590` inverse placement and `1DE630` collision traversal.
- The shared output fraction is the next instance's limit.
- `244C10` updates its collector fraction only for a strictly smaller fraction.
  An equal-limit collision is rejected before overwriting the shared output in
  the retained actual and controlled cases.

The native main-BSP/individual-instance arithmetic is independently validated.
Its instance enumeration now uses the literal cluster `+A8/+AC` MOPP bank;
cluster `+98` membership alone does not prove `3009E0` traversal order.
Dynamic registered object
contacts, active breakable state and successful registration chronology remain
separate explicit producer boundaries.

## Real authored counterexample

A read-only census covered all 21 supplied multiplayer maps: 53,334 rays,
including every authored spawn in the six original lighting-query directions
and sampled authored instance collision faces. It found 29 minimum-distance
instance tie observations, representing 17 unique rays: Headlong instances
`5/27` and Triplicate `46/49`. No sampled spawn ray hit this tie boundary.
This census is finite coverage, not a proof that other ties cannot occur.

The old one-adjacent-float secondary probe falsely admitted one unique Headlong
ray (observed from both overlapping faces):

```
origin = (-35.758461, -63.1607742, 55.4565697)
ray    = (2.99996328, 0.0000364449406, -0.0147798629)
fraction bits = 0x3EAAABAD (0.333341032)
instance5:  definition2, surface6, leaf3
instance27: definition8, surface3, leaf2
```

Both unchanged original instance queries return this same fraction at limit1.
They both miss at the exact fraction and at its next adjacent float, and hit
at the second adjacent float. The endpoint plane expression rounds the first
increment onto the same side. Therefore one ULP is insufficient for tie
identification; increasing a tolerance or choosing either owner is unsupported.

Unchanged original `244C10` preserves the first owner in both explicitly
supplied callback orders. The native source-attributed sample diagnostics also
differ: diffuse `0` versus `4868426`, lightmap `3618615` versus `3553337` and
slightly different interpolated incident vectors. These are native sampler
outputs, not original GPU color observations. They show why arbitrary owner
selection would affect lighting.

The preceding boundary fix performed a complete segment query only after a
clipped instance query
misses with an existing instance best. It compares the returned fraction
exactly with the retained best and marked equal ownership unresolved. A later
strictly closer hit cleared ambiguity. This temporary enumeration path has
been replaced by the original accelerator and its strict callback updates.

## Frozen validation

`tests/fixtures/retail_instance_contact_ties.tsv` contains 20 cases:
17 unique actual authored rays and three separately labeled controlled banks.
All expected collision/leaf/fraction words come from the unchanged retail
executable, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.

The controls copy an authored instance and explicitly move its placement
closer or farther; another uses the same authored collision bank as both a
static bank and an identity instance. They prove strict closer replacement,
farther-contact preservation and static equality. Their placement/membership
inputs are controlled test state, not claimed actual map registration.

The oracle executes original `1691A0/1689B0/11E5E0/141590/183FC0/1DE630` and
`244C10`, without replacement math or collision services. The breakable bank
is an explicit cold all-one source input. One read-only instruction observer
records the original local-vector arguments for the static control; it does
not change registers, memory or execution. Callback order is always an explicit
input and is never labeled actual accelerator order.

`instance_contact_tie_tests` / CTest
`retail_multiplayer_instance_contact_ties` checks 41 full individual-instance
queries and 69 endpoint controls. The 17 real rays additionally use the complete
unchanged original accelerator fixture to check 51 map queries over all three
admitted flag branches. The three controlled banks replay seven explicitly
ordered collision/strict-collector callbacks. They are not passed through an
authored MOPP that contains no such copied placement. The expected words in
the original control fixture remain unchanged.

The old one-ULP implementation failed `actual_headlong_8`. The complete original
accelerator chooses owner27 for this ray rather than the old diagnostic owner5.
No GPU, guest memory operation or guest advancement is needed for these tests.

The complete accelerator regression separately covers 13,229 source contacts
and 2,641 ordered callbacks across all 21 maps. It retains the Triplicate116
normal-query render-attribution miss even though its source collision succeeds.
Successful dynamic registration, live breakable state and physical Xbox
execution remain outside this static cold-bank proof.

## Reproduction artifacts

- `tools/audit_instance_contact_ties.cpp`: finite read-only map census.
- `tools/audit_instance_contact_ties_original.py`: unchanged retail single
  instance and ordered callback replay over the census rays.
- `tools/audit_instance_contact_tie_controls_original.py`: separately labeled
  source-controlled closer/farther/static equality replay.
- `tools/generate_instance_contact_tie_fixture.py`: freeze source results and
  provenance, including map and executable hashes.
- `analysis/multiplayer-shading/instance-tie-{census,original,controls-original}*`:
  detailed ray, raw word, native sample and callback reports.

The private census is compiled with `-ffp-contract=off` and linked against the
coherent private `halo2_cache` library. Run the original tools using the
workspace `.tools/python/bin/python` runtime, which provides Unicorn. They read
original assets and write host reports only. The regression reads frozen
source words and the two original map caches. Missing map assets return CTest
skip77.
