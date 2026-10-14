# Tag system

The engine's shared tag module lives in `include/halo2/tags/` and `src/tags/`.
Include `halo2/tags.hpp` for normal engine/tool access. `Cache` owns file-format
loading and the original bytes; `TagSystem` supplies group queries, residency,
references, nested blocks, data, strings and shared resources over that owner.
Xbox and converted Vista caches use the same interface.

## Read a tag

```cpp
#include "halo2/cache.hpp"
#include "halo2/tags.hpp"

auto cache = std::make_shared<halo2::Cache>(map_path, shared_maps_path);
halo2::TagSystem tags(cache);

auto bitmap_id = tags.find(halo2::tag_groups::bitmap, bitmap_path);
auto bitmap = tags.get(bitmap_id, halo2::tag_groups::bitmap);
auto images = bitmap.block(0x44, 0x74); // Original bitmap image block.
if (images.size()) {
    auto image = images.element(0);
    auto width = image.u16(4);
    auto height = image.u16(6);
}

// The same interface works at every nesting level.
auto sequences = bitmap.block(0x3c, 0x3c);
if (sequences.size()) {
    auto sprites = sequences.element(0).block(0x34, 0x20);
}
```

Put group-specific field names and layouts in their existing decoder modules.
The tag module owns the common binary operations. No caller needs to translate
a virtual address, reinterpret a disk pointer as a host pointer, or multiply a
block index into an unchecked address. Numeric offsets above are the recovered
bitmap layout already used by `material.cpp` and the bitmap oracle.

## Identity and groups

- `TagHandle` contains the complete 32-bit salted datum. `index()` is a table
  slot, never a replacement handle. Lookup checks the complete datum and the
  retail signed-short slot range.
- `TagGroup` contains exactly four bytes, including spaces and capitalization.
  `tag_groups` declares all 120 known group identities and `tag_group_name`
  supplies their developer names. A cache can declare additional groups.
- Every cache's actual 12-byte group rows supply its parent and grandparent.
  Retail caches declare 118 groups; Vista declares 120. The catalog never
  fabricates inheritance or inserts missing groups into a loaded cache.
- `get(handle, group)` and `list(group)` include the authored ancestors.
  `find(group, path)` uses the exact group and an ASCII case-insensitive full
  cache path, matching Vista `tag_loaded`. Backslashes are preserved.
- `list()` includes unloaded entries and zero-size aliases, following retail
  iteration. `try_get()` returns an empty optional for an unavailable identity,
  group or residency. Invalid binary ranges raise `FormatError`.
- `Cache::find_tag` and `Cache::next_tag` provide the same directory operations
  to decoder modules. Directory validation runs for every loaded cache.

The title scene owns a `TagSystem` alongside its cache. Multiplayer's
`MultiplayerPreparedWorld` owns one for the lifetime of the match and selects
its BSP before content initialization. Title object classification and the
multiplayer biped/model/marker chain use the same owning views as tools.
Group decoders share directory lookup and binary range operations while keeping
their recovered layouts and original runtime bank ownership.

## Engine decoder access

All host engine consumers use the shared directory. Use `query_tag` for a single
exact match and `select_tags` for an allocation-free range in original directory
order. These preserve the decoder's original selection rules; inherited group
lookup remains `find_tag(handle, group)` or `TagSystem::get`.

```cpp
const auto* weapon = cache.query_tag({
    .handle = weapon_handle,
    .group = halo2::tag_groups::weapon,
    .require_address = true,
    .minimum_size = 0x31c,
});
for (const auto& shader : cache.select_tags({
         .group = halo2::tag_groups::shader,
         .require_address = true,
     })) {
    // Decode this shader through its existing layout module.
}
const auto* owner = cache.query_tag({
    .contains = halo2::TagAddressRange{address, byte_count},
    .require_address = true,
});
```

Queries check complete salted handles, match groups exactly, and match names
case-sensitively unless `TagNameMatch::ignore_ascii_case` is requested. A query
without filters includes sparse slots. `require_address` retains zero-size
aliases; request a minimum size when the decoder requires a payload. A query
returns the first matching row. Consumers that require unique ownership still
check every selected row. `tag_at_index` is the checked raw-slot adapter for
original routines whose input is an index rather than a salted handle.

`read_tag_bytes(address, size)` validates the cache mapping and borrows its
immutable bytes. `tag_payload(handle)` additionally checks residency and the
complete payload; separately mapped Xbox BSP/lightmap roots use owning
`TagSystem` views. The cache must outlive these spans, directory ranges, and
directory pointers. Any name passed into a range query must also outlive its
iteration. Use an owning view when a result needs an independent lifetime.

`TagAddressSpace` supplies checked address conversion and block decoding for an
explicit borrowed allocation, including BSP and source-decoder buffers. A block
returns `TagBlockRange { count, offset }`; `read_tag_block_descriptor` and
`TagBlock::extent` share serialized count and overflow validation. Cache-aware
decoders use `metadata_block` to retain Vista resident-region checks. Recovered
per-field count limits remain at the decoder call sites.

Rendering, content, UI/HUD, animation, sound, particles, world effects, device,
weather, geometry and source-runtime imports use these operations. Private
read-only metadata copies formerly used by effects, footsteps, props and menu
pass readers have been removed. Source postprocess stores and provider
interfaces that explicitly own mutable or copied source images retain that
ownership. Literal Xbox/Vista executable adapters still operate on their
original memory banks and pointer layouts; those banks are populated through
the shared cache interface.

`Cache::tags()` is retained for directory inspection/export and tests. Engine
code accesses the underlying host table only inside the cache loaders and tag
module. `tools/check_engine_tag_access.py` enforces that boundary in CTest.

## References and dependencies

`view.reference(offset)` reads the original eight-byte group/datum pair.
`tags.resolve(reference)` checks the complete target identity and its authored
group relationship; a null datum returns an empty optional. Group decoders
identify their actual reference fields and follow them through this operation.
Cross-tag block/data pointers resolve through the resident metadata allocation.
This preserves compiled references and shared banks without copying each
referenced tag into a separate allocation or scanning arbitrary words for
possible datums. Reference cycles are allowed, as in the compiled caches.

Compiled cache loading already delivers the complete resident tag bank.
Dependencies therefore share the cache owner. Resource activation continues
through the geometry, bitmap, sound and animation owners, with their existing
original startup and postprocess routines.

## Ownership and BSP residency

`Cache` owns a stable initial file image through one private byte accessor.
On Linux, `CacheFileSnapshot` creates an independent anonymous disk file and
maps it privately. It uses a filesystem clone when available and a bounded
streaming copy otherwise. Source overwrites, truncation and unlinking cannot
invalidate the image; original title-update writes stay private. Decoders and
tag views use the same bytes and public API as before. Vista canonicalization
keeps its resizable owned buffer, and unavailable snapshot backing falls back
to that existing buffer on other paths and platforms. The file owner contains
the platform details; group decoders do not select or manage backing storage.

`TagSystem` retains a `shared_ptr<const Cache>`. A `TagView`, embedded structure
or block view retains its immutable backing independently. Existing views
remain valid after another BSP is selected or the facade is closed. This makes
asynchronous engine/tool reads safe from dangling metadata. Borrowed byte spans
must not outlive their owning view. The spans returned directly by `read()` and
`groups()` borrow the `TagSystem` backing.

`load_bsp(index)` validates the actual scenario mapping, resource signature,
tag salts/groups and both resource roots, then publishes a new BSP view
atomically. Xbox `sbsp`/`ltmp` directory entries remain unloaded until that
selection. `unload_bsp()` removes their current lookup bindings. Previously
returned views are deliberate owning snapshots. Vista's converted metadata is
already resident. Failed loads leave the prior selection intact.

BSP rows originally have zero sizes. Their views start at the published root
and are bounded by the containing BSP allocation, retaining shared child
banks. Main Xbox metadata reads are bounded by the actual metadata allocation;
Vista reads are bounded by its converted resident regions. Empty block/data
descriptors do not dereference their unused pointers. Negative counts, invalid
indices, overflows and escaped pointers fail before access.

`resource(pointer, size)` uses the original upper-two-bit file selection for
local, mainmenu, shared and single-player shared resources. `string_id` uses the
24-bit index and preserves the length byte separately. The mutable source tag
store remains an explicit private copy for original postprocess consumers;
zero-size aliases are omitted from its owned payloads.

Keep load/unload/close on the owning engine thread. Immutable views may be read
by workers while their backing owner is retained. The native original runtime
bank still receives literal cache/tag/BSP images through
`source_runtime_cache_tags.cpp`; host view ownership does not replace original
game-world constructors or resource-manager state.

## Source evidence and verification

The executable authority is the supplied retail `default.xbe`, SHA-256
`03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d`.
The recovered `unknown_122870.cpp` and `cache_tag_instances.cpp` describe
`122C10` (salted group lookup), `122C70` (iteration), `122D00` (group lookup),
`122B40` (BSP root publication), `122BC0` (BSP unbinding), and cache disposal.
The pinned Vista tag headers supply its two additional group names and
reference/block/data layouts. Assembly core layouts and the supplied cache
bytes establish cache pointer conversion and serialized widths.

`tools/export_tag_system_oracle.py` executes unchanged retail lookup/iterator
instructions over actual mainmenu, Zanzibar and Cairo Station directories.
It relocates only directory-owner pointers and supplies no function results.
The comparison covers 354 group lookups, 76,079 salted/group lookups and 31,069
iterator results, including ancestor queries, wrong salts, invalid slots,
unloaded tags and exhaustion state. The local report is
`analysis/tags/retail-oracle.json`.

The original-asset sweep covers 83 supplied retail/map-pack/Vista cache files,
all 120 group types, 467,229 valid table rows, 464,279 complete resident payloads
(991,356,196 exact bytes), and 183 BSP loads. It also checks local/shared
resource selection. Contract tests cover nested structures, cross-tag blocks,
data/references, immutable view lifetime, mutable copy isolation and malformed
cache directories. These tests establish tag-runtime behavior and byte
preservation; gameplay consumers and framebuffer fidelity have their own
validation requirements.

The Release engine and both launchers build with this module. All 100 selected
engine migration checks pass with no skips, covering tag/cache ownership,
rendering, geometry, UI/HUD, animation, sound, material effects, weather,
particles, devices and original multiplayer postprocess stores. The selected
test names are in `analysis/tags/engine-migration-tests.txt`; the JUnit result is
`analysis/tags/engine-migration-20261012.xml`. The runtime hash, launcher
selection and validation scope are recorded in
`analysis/tags/engine-migration-20261012.json`. Targeted AddressSanitizer and
UndefinedBehaviorSanitizer checks of the tag implementation also pass.
Native Vulkan smoke checks rendered three title frames and three Lockout
multiplayer frames; a separate Lockout run advanced three simulation ticks.
These are startup checks, not a complete match or a framebuffer-parity claim.
At that migration checkpoint, both installed launchers were verified with
`--launcher-info` to select `build-tags-20261012/halo2_native`.
Subsequent live gameplay checks, complete matches, exact fixed-build provenance
and the remaining campaign blockers are recorded in
[gameplay validation](TAG_GAMEPLAY_VALIDATION.md).
The memory-bounded continuation also retains complete old/new tag byte digests
for 20 supplied multiplayer maps and a passing strict sixteen-player Slayer
path on all 21 maps. The subsequent Vista/Xbox owner continuation fixes the
TurfFFA model-region transition and GeminiTeamUDP automatic doors, and passes
strict sixteen-player FFA on all21 maps with those fixes. The door and region
reducers retain explicit world/effect boundaries; consumers share the model
decoder and completed region snapshots instead of reconstructing choices.
Original CPU comparisons, CPU prop submission, UDP packet-fault matches and
compatible captured source cohorts are recorded in the dated continuation of
that validation record. Original tag data and concurrent launcher work remain
preserved; these checks do not establish every gameplay/platform parity gate.


```sh
cmake --build build-tags-20261012 --target \
  tag_system_tests tag_system_asset_tests tag_system_oracle_tests inspect_tags
ctest --test-dir build-tags-20261012 --output-on-failure -R '^tag_system_'
ctest --test-dir build-tags-20261012 --output-on-failure -j 4 \
  --tests-from-file "$PWD/analysis/tags/engine-migration-tests.txt"
.tools/python/bin/python tools/export_tag_system_oracle.py
build-tags-20261012/inspect_tags assets/disc/maps/zanzibar.map weap
python3 tools/install_launcher.py --build build-tags-20261012
python3 tools/install_launcher.py --build build-tags-20261012 --title-first
```

The executable-derived fixture is generated locally from the user's files.
Missing original files/fixtures produce CTest skip code 77. The asset test
does not require generating an oracle fixture.
