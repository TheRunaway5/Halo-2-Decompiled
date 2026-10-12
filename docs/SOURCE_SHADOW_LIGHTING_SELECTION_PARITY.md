# Shadow root lighting-cache selection

`source_shadow_lighting.hpp/.cpp` implements complete original `3DC90` and
`BAF80` selection over explicit current object slots and cache-service inputs.
It does not activate gameplay shadows or create object/cache lifetime state.

The authority is the supplied `assets/disc/default.xbe`, SHA256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`, and the
immutable same-phase `32410-cached-opacity` object/cache bank in
`analysis/multiplayer-shading/shadow-live-phase-20261008/capture-opengl`.
No guest reads, writes, advancement or breakpoints are required by the oracle.

## Literal selection

`3DC90` indexes the actual object table by the handle's low16, without checking
salt or occupancy. Current object flag24 (`object+7` lowbit0) immediately returns
`[4DE2F4]+1C`; that branch does not read the parent or call the cache producer.
Otherwise, a non-NONE parent runs complete `BAF80` to the root and restarts
`3DC90`. Intermediate default-lighting flags are not tested during that traversal.

At the unparented root, `3DDD0` can acquire or rebind a cache record. A NONE result
returns no lighting record. Otherwise `3DC90` reads literal record byte `+3` at
`[509434]+44 + (token&FFFF)*100`. Any nonzero byte admits the record's lighting
at `+54`; zero returns no record. The original does not add a token-generation,
occupancy, material, geometry, distance or current-frame check at this point.
The cache acquisition service's separate full-owner reuse check still applies.

The new helper returns the selected storage, actual root/current handle and
cache token. Its caller supplies actual `3DDD0` through a callback and the
post-callback validity bytes. A selected global default bank must be explicitly
present. No default colors or substitute light records are fabricated. Missing
selected inputs, duplicate/out-of-range slots and parent cycles reject. Unread
parent/flag/cache fields remain optional on the original early branches.
The slot limits correspond to the independently recovered 2,048-object and
256-render-record allocations.

## Independent proof

`tools/differential_source_shadow_lighting.py` executes unchanged original
`3DC90/BAF80` for 2,090 cases and freezes 10,450 selection/callback words.
Its 2,048 controlled rows supply only the external `3DDD0` token at that service
boundary; the unchanged caller performs the actual validity-byte read. These
controlled rows are not a new full allocation proof.

The remaining 42 rows use all 202 actual object records and the complete 256
cache records from the same `32410` phase. Their existing tokens match full
root-owner identities, so unchanged original `3DDD0` executes its full reuse
path without a service override. Chief's selected slot9 pointer independently
matches original `32410` EAX `82EBAA94`; all 84 selected lighting bytes match the
separately captured lighting record. Cache allocation/pressure remains the
independent proof in [SOURCE_RENDER_CACHE_PARITY.md](SOURCE_RENDER_CACHE_PARITY.md).

All 10,450 words match the standalone native implementation. The regression
also checks 12 missing/invalid inputs, current default early return,
complete-root traversal past intermediate default flags, low16 salt semantics,
unread fields, NONE tokens, zero validity and noncanonical nonzero validity.
Normal and ASAN/UBSAN/leak checks pass. CTest registration is
`retail_source_shadow_lighting`. Reports and hashes are retained in
`analysis/multiplayer-shading/source-shadow-lighting-original.json` and the
fixture's adjacent provenance file.

Live native owner registration, actual cache acquisition/query cadence and
lighting conversion, source secondary admission and ordered caster/receiver
draw units remain required before normal-gameplay shadow activation.
