# Retained constructor tag metadata

The production cache now exposes complete resident tag payloads through
`Cache::load_source_tag_metadata`. `SourceTagMetadataStore` retains the cache
and all payloads in explicitly selected groups. Its definition, address and
bounded read callbacks survive destruction of both caller facades and release
their backing after the last callback is released. Source VA words remain
literal; no resource activation or host pointer fixup is applied.

The verified constructor-tail packets require `bipd`, `hlmt`, `udlg` and the
existing separate `jmad` provider. In the measured Spartan path, model+90 and
variant+34 refer to dialogue (`udlg`), and the source copies dialogue+14.
These references do not select a collision tag. Collision metadata and live
collision bodies have separate owners.

The original multiplayer Spartan model also forwards its node-bank pointer
outside its own tag allocation. Lockout's `masterchief_mp` model `E9F3086E`
has VA `802E9BF8`, extent 700, count 33 at+78, and pointer `802AE1AC` at+7C.
The full 33×5C bank resides inside `masterchief` model `E9060781`, VA
`802ADBE8`, extent 4,532. The store resolves the unique actual payload
containing a read across the owned group set. Unknown or ambiguous reads throw;
they do not become NONE/default models. A single record's `bytes_at` remains
bounded to that record. BSP/ltmp tags require their separate mappings and are
explicitly rejected by this ordinary resident-tag provider.

Independent exports authenticate the earlier owned-map inventory and every
map SHA-256, then copy complete literal tag bytes and original hlmt node banks.
Both exports reproduce fixture SHA-256
`c419a3365ccacd6e578bda5e0dedb4336e830c4f00025b1389db1ce89f50e7c7`.
Production CTest `original_source_tag_metadata_store` passes in normal 0.29
seconds and GCC AddressSanitizer/UndefinedBehaviorSanitizer 1.89 seconds:

- 23 owned maps, including both latest DLC maps.
- 2,642 complete relevant tag records, 3,229,316 exact payload bytes.
- 2,498 complete model-node banks, including 250 cross-tag banks.
- Retained provider lifetime, final release, group exclusion, missing inputs,
  full-record bounds, huge extent rejection and valid empty end reads.

This provides immutable metadata for the source constructors. It does not
create a live unit, assign a local player, allocate grenade components, bind a
current channel, construct a pose or produce a projectile. Source instructions,
mutable owners and their real services remain required before live integration.
The canonical launcher is unchanged.

Sources, binaries, logs, literal exports, input pins and prior evidence are
preserved in
`analysis/multiplayer/integration/source-tag-metadata-provider-20261008/frozen/manifest.json`.
The shared CMake/cache snapshots are observed in full; this change owns only
the metadata API/module and its production test registration.
