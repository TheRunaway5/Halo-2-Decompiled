# MCC data import and Xbox TU1.5 conversion

MCC installation discovery, private asset copying, a native v13 reader,
geometry/model/bitmap/animation/audio adapters, compiled immutable TU1.5 engine
operands, preserved native shader libraries and the two recovered TU1.5
multiplayer postprocessors are implemented. **Full
MCC-to-Xbox gameplay conversion and native MCC playback are unfinished.** This
command prepares an inactive cache; it does not remove the current game's Xbox
asset dependency or claim that MCC tags are interchangeable with Xbox tags.

```sh
./Halo2-Launcher --prepare-mcc-gameplay
# Nonstandard installation, or the halo2 / h2_maps_win64_dx11 directory:
./Halo2-Launcher --prepare-mcc-gameplay --mcc-source '/path/to/MCC'
# Works without a built runtime or first-run Vista/Xbox setup:
python3 tools/setup_assets.py --prepare-mcc-gameplay
```

Preparation requires installed MCC Halo 2 Classic data, Python and enough disk
space. It requires no Xbox ISO, Vista installer, running MCC, mod download or
downloaded format schemas. The release target requires **MCC only**. The OG Xbox ISO and TU1.5 patch are reference-only material for recovering and validating behavior and visuals; they must not become onboarding or normal runtime inputs. The existing executable-backed gameplay runtime still violates that target and remains unfinished.
The native Vulkan shader selection is unchanged.

## Converted data is vanilla

The requested target is the engine's official internal Xbox TU1.5 data set.
MCC is an offline donor. Once fully normalized and validated, the converted
definitions must use the normal vanilla loading path, with their authored
group/path identities. They must not be registered as an MCC mod, mounted as
an override, or given source-specific tag prefixes. Donor provenance belongs
in conversion records, rather than changing gameplay tag identity.

TU1.5 equivalence concerns authored data and observed behavior. The requested
native 64-bit representation can differ from the packed Xbox binary layout;
that does not authorize changing gameplay precision, timing or visuals.

The current raw v13 reader is a conversion/inspection foundation. It is not a
claim that raw MCC structures are ready for vanilla admission. Preparation
still preserves the working launch selection until that conversion is complete.

## Native 64-bit addresses

The native executable already runs as a 64-bit process. The engine-facing tag
module now retains actual 64-bit addresses of its immutable owned backing for
tag definitions, roots, embedded structures and nested block elements.
`NativeTagAddress` is a separate type with no implicit conversion to a 32-bit
integer. Native reads check overflow, allocation ownership and residency.
Views retain their owner across closing the tag system or changing BSPs.

Serialized directory pointers, block/data descriptors and salted handles keep
their specified file-format widths. Reading them resolves an owned native
address; it does not turn a source pointer into a host pointer by casting.
The explicit original-code boundary resolves a native address back through
its owner to the original 32-bit source token, checking that it identifies the
same bytes. This preserves the recovered Xbox routines' packed structures
and arithmetic. Gameplay values, floating-point precision, update timing,
shader programs and texture contents are unchanged by this address migration.

Live engine world, object/player pool, object-body and clock interfaces now also use `NativeRuntimeAddress`. Datum-pool views, object lifetime tracking, scene publication, vehicle readback and death-camera bindings use checked native addresses. A separate allocation index checks owner identity, permissions, commitment holes and bounds. Checkpoint files retain source tokens; restored worlds rebind native addresses to their new allocations. Focused checks exercise actual backing above 4 GiB and reject cross-world pointers.

This covers the native tag and live-state interfaces. It does not widen
the recovered Xbox execution ABI or remove its physical cache reservations,
tag-count limits or remaining MCC physics/layout conversion requirements.
Large native mod banks and their gameplay admission still require those
remaining consumers to be ported or explicitly adapted; replacing an integer
type alone would not make those banks playable. Full gameplay and framebuffer
parity remain separate validation requirements.

The checked 64-bit tag layer passes four focused native test groups and all
13 import tests. Complete payload comparison covers 84 original/Vista maps,
469,014 resident payloads, 995,569,592 bytes and 184 BSP selections. All 43 MCC
maps pass native 64-bit BSP ownership/geometry checks, and all 114,765 animation
clip occurrences pass decoding and the owned resource I/O boundary.

A fixed six-tick original Lockout run using the English TU1.5 executable
retains the same player position, first-person pose, ammo and RNG state as the
pinned pre-migration native build. Its 1280x720 captures match all 921,600 RGB
pixels exactly. This is a bounded native-before/native-after comparison; it
does not establish physical Xbox framebuffer parity or complete gameplay
coverage. The frozen tested executable and source/build pins live under
`.local/mcc-native-20261013/native64-snapshot/`. The detailed evidence and
remaining work are in the continuation's `status.json`.

The live-state interface continuation is separately built under
`.local/mcc-native-20261013/runtime64-snapshot/`. All four TU1.5 runtime groups
pass: live constructors/controller movement, 120 physics frames, fatal cleanup
and a full headless Match death/ragdoll/observer/respawn cycle. Native memory,
scene handoff, tag, datum-pool and original asset/oracle checks also pass.
A legacy TU1.1 fixture still lacks source owner `2DD1D0`; the same failure
reproduces in the source32 reference and is not covered by the TU1.5 passes.

For address-only validation, a second build uses identical frozen sources with
only four interface files restored to source32 world/pool/body/clock tokens.
Both execute the same six fixed TU1.5 reference ticks with identical captured
state and zero differing RGB pixels. This supersedes comparison against a
moving checkout and does not establish physical Xbox framebuffer parity.
Evidence is in `runtime64-fixed-comparison.json`; all 26 migration source/test
files match that tested snapshot. The current importer passes 16 tests.

## Native immutable TU1.5 operands

MCC caches now select `OriginalData::native_tu15()` before their load-time
postprocessors run. This factory uses compiled typed initializers for all
named immutable fields recovered by the existing TU1.5 `OriginalData` reader:
61 record types, 377 fields and 12,261 scalar operands. It creates native
container/shared ownership and performs no file I/O. Every scalar bit pattern,
container extent and optional/shared presence matches the authenticated
English TU1.5 reference (12,638 projection entries). A traced factory run opens
only system libraries, and a traced MCC content load opens no Xbox executable.

`tools/generate_native_tu15_operand_schema.py --check` verifies the complete
declared-field inventory. `generate_native_tu15_operands` is an offline
reference tool; rebuilding its output from the reference reproduces
`src/native_tu15_operands.generated.cpp` byte for byte. Executable headers,
sections, object padding, host pointers, mutable runtime banks and the input
path are excluded. Source routine/vtable identities remain opaque source IDs,
not callable host pointers. This is the already classified immutable data
surface, not a substitute for recovering the remaining startup globals.

The frozen build under `.local/mcc-native-20261013/native-operands-snapshot/`
passes all eight selected native test groups, including the four TU1.5 live
world/physics/death groups. All 43 MCC maps pass content loading with no Xbox
executable argument; all 25 multiplayer maps apply 41 verified combat stores.
The cut Falcon RPG bank still reports its missing authored barrel. Reading
content and applying those stores does not establish complete tag semantics,
native gameplay startup or physical Xbox parity. The current MCC gameplay
attempt stops at the explicit native world-startup/physics conversion gate.

The campaign checks also exposed a raw-chunk decoding error. Installed MCC
negative-size cache entries start their literal payload at the table's file
offset and have no two-byte zlib header. The old retained Reclaimer reader's
skip shifted those raw chunks and borrowed bytes from the following chunk.
Both port readers now retain their complete literal extents. The three
affected maps pass geometry, models and animation/resource checks after the
fix; synthetic adjacent raw/zlib/raw regression fixtures verify both chunk
ends, including raw data beginning with zlib magic and truncated final data.
No collision normal is clamped or regenerated. Floodlab's 37,509 script
expressions are retained for inspection within the serialized 16-bit datum
index space; its Xbox reference has 36,670. That difference remains conversion
work, rather than being treated as Xbox-equivalent scripts.

Evidence: `native-operands-tests-final.json`,
`native-operands-allmaps-fixed/report.json`,
`native-operands-raw-chunk-evidence.json` and
`native-operands-raw-affected/report.json` in
`analysis/asset-setup/mcc-native-20261013/continuation/`.

## Discovery and cache ownership

`tools/mcc_content.py` searches Steam's Linux, Flatpak, Windows and macOS paths,
Steam's configured external libraries and its MCC application manifest. On
Windows it also reads Steam's installation registry entries and checks common
XboxGames installation paths. These Windows paths are implemented but have not
been exercised on a physical Windows machine. Nonstandard installations can
use `--mcc-source`, the CLI's `--mcc-root`, or `HALO2_MCC_ROOT`.

The imported data lives in `assets/imported/mcc-<id>/halo2`. All regular content
files under the installed `halo2` folder are copied, including Classic maps,
`textures.dat`, sound banks, compiled fonts, variants, videos, terminals and
opaque anniversary asset packages. Standalone program/debug binaries and
`prebuild/shaders_2_cache` are excluded. Shader/material metadata embedded in
maps and opaque packages remains original source data for inspection; it is
never selected as native Vulkan shader code.

Relative filenames, case and directories are retained, including empty
directories and any `tags/` or `data/` trees inside the selected donor.
Derived bitmap payloads and reports are separate cache artifacts. They do not
rename or flatten the source trees, and preserve the original tag namespace.
Installed mods are not automatically activated as vanilla content.

The source is read only. Copying streams SHA-256 and rereads the private result;
the importer rejects source-file or installation-set changes during preparation.
It rejects symlinks, overlapping source/output directories, malformed caches,
excessive decompression, bad full salted tag IDs and invalid texture extents.
Each failed preparation removes only its new staging directory. Successful
preparation publishes a new directory with an explicitly inactive provenance
report. It never writes `asset-sources.tsv` or the selected gameplay executable.

`bitmap-resources.json` retains every resident bitmap bank, frame descriptor,
dimensions, format, flags, authored mip count and all three resource partitions.
Content-addressed `bitmap-resources/*.bin` contain unchanged decoded partition
bytes. This covers diffuse/detail/bump maps, cube/volume textures and UI without
assuming that every format is an ordinary RGBA image. It performs no resizing,
normal-vector regeneration, channel swapping or guessed palette substitution.

## TU1.5 scope and remaining runtime work

`tag-conversion.json` records the ordered stores of the previously recovered
`0011F060` netgame and `00165B30` combat routines. Their source executable is
pinned to SHA-256
`21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd`.
Exact shared datum/group/path identities and resident-allocation bounds remain
mandatory. Campaign and menu maps are inventoried without merging multiplayer
shared metadata into their independent virtual address spaces.

Modified private caches retain version 13 and their original virtual pointers;
they are serialized as uncompressed MCC caches and independently reopened.
Untouched caches remain byte-identical to their copied source. Relabeling a
version-13 cache as Xbox version 8 is not a conversion.

The native reader now resolves v13 compressed/raw chunks, exact local/shared
owners, player starts, BSP geometry, model geometry and bitmap partitions.
It retains the PC floating vertex layouts and handles PC cube face/mip order
and linear RGBA volumes without selecting MCC shader programs.

Compiled physics/collision and remaining consumer normalization are still
needed. The animation adapter now deep-copies shared banks and expands MCC's
0x60 entries into the original 0x6C entries with owned resource groups. All
114,765 clip occurrences across all 43 maps decode through the existing Xbox
animation codec. This verifies layout/resource decoding, rather than proving
that MCC's additional or changed authored clips equal TU1.5's animation sets.

The generated animation resources now own complete 512-byte sectors,
including zero padding, and use aligned file offsets. The native original-code
I/O boundary resolves only actual graph descriptors and checks their complete
owned resource extent, instead of trying to read generated resources from the
original donor file. All 114,765 clip occurrences across all 43 maps also pass
the owned sector-read check with unchanged codec bytes.

The sound adapter resolves each sound's exact map/shared gestalt owner,
localized permutation records and neutral/English/remastered bank selection.
The installed original MCC executable (`halo2.dll`, SHA256
`de65b4f4fdbf3f0a5eab7431fe530da17dd815599182dfd6ae9b7e21cf171946`)
establishes the signed Opus packet framing, 240-frame decode calls and chunk
allocation rounding. Opus, ADPCM and serialized WMA now use native PCM owners.
Lockout decodes all 4,532 Classic permutations; the tutorial decodes all 2,678
payloads. The latter still exposes the existing sound-variation reader's
unsupported count above 31 for `marine_shortmove`. Sound gestalts/classes
remain source-format metadata behind adapters, rather than fully converted
vanilla Xbox structures. Audio decoding does not establish original Xbox
mixing, timing, waveforms or physical device behavior.
Two postprocessors do not resolve the broader
semantic differences across all gameplay tag groups. The existing
[PC tag comparison](PC_GAMEPLAY_TAG_COMPARISON.md) records those differences and
its explicit schema/opaque-data exclusions. Reports therefore retain
`runtime_active: false` and `full_tu15_parity: false`.

## Graphics restoration reference

The user supplied [Halo 2 Reformation's patch notes](https://www.nexusmods.com/halothemasterchiefcollection/mods/2660)
as investigation leads. They describe changed image content, launch/DLC asset
collisions and material parameter differences as well as optional enhancements.
Restoration must use each original Xbox map and TU1.5 behavior as authority.
Higher resolution alone does not identify a defect or justify downsampling.
The mod's assets have not been downloaded, redistributed or applied.

Initial direct Zanzibar checks confirm different compiled vertex-constant
arrays for `metal_roof`, `basic_concrete`, `zanzibar_seawall` and
`massive_concrete`. The last also uses different template/parameter ordering;
blindly copying the same constant slot would be incorrect. The currently
installed MCC BR bump map remains 256x256 but uses format 11; the Xbox one is
256x256 format 17 (P8 bump). Identical dimensions therefore do not establish
equivalent pixels or sampling semantics.

The first base-image check expands Xbox P8 with the fingerprinted retail
executable's actual palette and unswizzles its texels. BR's resulting RGBA
pixels exactly match MCC's decoded format-11 base image (zero differing
texels). Zanzibar massive concrete differs at 258,506 of its 262,144 base
texels. This corroborates an actual image change for that asset; it does not
attribute every rendering difference to its pixels or compare all mip levels.
Hashes, descriptors and material arrays are retained in
`analysis/asset-setup/mcc-gameplay-20261013/visual-source-check.json`.

The graphics work still requires broader bitmap/palette/channel comparisons,
launch/DLC separation, and rendered TU1.5 validation. The bounded restoration
below is available in a separate inactive converted cache.

An installed Workshop copy of Reformation was located at item `3361280895`.
It is reference evidence, not an automatically mounted runtime mod. The
current restoration work must still distinguish legacy restoration from
enhancements and independently check all affected mip levels and bindings.

The native `inspect_bitmap` tool independently checks Zanzibar's
`massive_concrete_bump` through the engine's ordinary bitmap decoders. The
installed Reformation bank's 512x512 base mip matches the original Xbox bank
exactly; all nine smaller decoded mips differ. Stock MCC differs at 258,506
base-mip texels. The report in
`analysis/asset-setup/mcc-native-20261013/concrete-native/comparison.json`
retains all ten levels and source-map hashes. This is decoder evidence; it
does not establish physical Xbox sampling or justify replacing the whole bank
with Reformation's regenerated mip chain.

`tools/preserve_native_shaders.py` preserves existing Xbox shader metadata,
programs and function tables as owned relocatable libraries. It binds bitmap
references by exact group/path identity and relocates pointers and StringIDs;
it imports no MCC shader program. The selected library is carried into future
MCC preparations independently of the disc. Converted definitions occupy the
normal tag directory under their existing names and handles. No shader mod
overlay or source-specific tag namespace is used. Existing Vulkan shader
source files are unchanged.

The preserved catalog contains 39 original Xbox counterpart map/menu libraries.
Both retained MCC imports now include their verified copies. All 43 caches
still load native BSP geometry after preservation. The two PC-exclusive maps
and shared resource containers have no separate original-map shader library.
That catalog is not a claim of complete original rendering support.

`tools/restore_mcc_zanzibar_bump.py` creates a separate private converted tree,
retaining the complete MCC folder structure and donor provenance. It changes
only Zanzibar's exact bitmap descriptor and appends an owned BGRA resource to
the private texture bank. Its ten mips come from the independently decoded
original bank; Reformation independently corroborates the base image.
Terminal and other maps retain their original references. Its output is
`assets/imported/mcc-zanzibar-restored-20261013`; it is inactive and does not
change the installed MCC files or the game launch selection.

The selected owned correction profile is also applied automatically to future
MCC preparations. It requires the measured MCC map fingerprint, verifies the
original complete pixel bank, appends the converted texture partition and
records donor/converted hashes. It needs the preserved correction profile,
rather than rereading the disc or Workshop mod. A profile-application replay
produces exactly the same map and texture-bank bytes as the native-verified
restoration above. Unrecognized source revisions fail preparation explicitly.

`native_mcc_preserved_shaders` compares Zanzibar's `metal_roof`,
`basic_concrete`, `zanzibar_seawall` and `massive_concrete` against the original
cache. Shader templates, constants, switches, bitmap identities, assembled
pixel programs, vertex programs and existing rendering limitations match.
All ten restored concrete bump mips match the original decoded pixels.
This preserves the existing shader pipeline and its current limitations; it
does not establish final Xbox framebuffer parity or a native MCC playthrough.

MCC also splits a bitmap's old depth word into depth8 and creation flags8.
The native reader and new import inventories now keep these fields separate;
sampling flag 8 no longer makes a 2D image appear to have depth 2049.

## MCC loading assets and authored textures

The current requirement supersedes the earlier Vista loading-bank selection.
MCC-only preparation does not read `loading.bin`, `halo2.exe`, the selected
Vista loading registry, an Xbox ISO or the TU1.5 executable. The old Vista
capture and extracted bank remain historical reference material, rather than
inputs to new MCC onboarding.

`tools/mcc_loading_assets.py` prepares all seven Classic loading bitmap banks
from MCC `mainmenu.map`/`textures.dat`: 13 frames, including the backdrop,
sweep, spinners, logos and tickers. DDS exports retain their source formats,
mips and normal tag/frame identities under `loading/mcc/bitmaps/`. Original
MCC files retain their exact structure under `halo2/`. The exporter does not
invent a correspondence with Vista's twelve executable texture slots.

The two existing hand-authored mechanical-ring SVGs and their rendered DDS
textures are retained in `assets/native-loading-custom/authored-rings-20261013`
and copied to `loading/custom/`. Their hash-pinned profile contains authored
geometry and its rasterizations only. They are review studies, not admitted
replacement textures. Rendering/presentation mapping and actual loading audio
remain separate work; no old Vista ambience is imported.

Both private MCC preparations now contain these 13 MCC frames and four
custom source/texture files. Historical copied Vista files are ignored by this
preparation path. Their prior video is retained for reference only at
`analysis/asset-setup/loading-screen-assets-20261013/final-video-with-sound/vista-match-loading-with-sound.mp4`.

## Requested MCC menu inventory

The importer verifies all 287 requested menu banks, 648 frame owners and 561
unique DDS pages against `tools/reference/mcc-vista-menu-inventory.tsv`.
645 owner rows are Vista-identical; three retain MCC's measured server-icon
alpha variants. The unavailable fourth server-icon state remains explicit.
The twelve original Vista executable loading pages are outside the revised
scope and are never requested as dependencies.

All 40 MCC font/table files retain their exact filenames and case under
`halo2/h2_fonts`, including `MSLCD-14` and the CJK banks. The original map
caches and `textures.dat` retain the images; `vista-menu-inventory.json`
records checked identities and hashes without changing their namespace.
Both private preparations pass complete frame and copied-font checks.

## Native reader verification, 2026-10-13

The separately configured Release build completes the native reader, cache
tests, inspection tool and onboarding launcher. Seven focused CTest groups
pass. They cover native chunk/directory/resource boundaries, cube face/mip
mapping, volume texels, the existing Vista ownership regression, import
transactions, original Vista loading preservation and launcher routing.

All 43 copied MCC caches pass native BSP geometry loading. Lockout additionally
loads 181 drawable model banks (329,292 expanded vertices), its BR texture and
12,882 collision triangles. Its broad content inventory reports an additional
Falcon RPG definition whose primary trigger has no authored barrel; this has
not been hidden or counted as a verified vanilla definition. Full compiled
physics normalization, authored gameplay semantics and a native MCC gameplay
run remain outside this geometry result. The animation, audio and bounded
shader/bitmap checks above are separate evidence.

The layout coverage report inventories 79 source tag groups, with explicit
differences including 36 animation-layout entries, 218 physics-layout entries,
41 collision-layout entries and 163 sound-gestalt-layout entries. This is an
inventory of needed work, not a successful semantic conversion. Evidence is
under `analysis/asset-setup/mcc-native-20261013/`.

## Local verification, 2026-10-13

The installed MCC copy prepared 883 asset files (29.10 GiB), 43 Classic caches,
25 multiplayer maps, 19,686 resident bitmap-bank occurrences and 50,283 bitmap
frames. It decoded 19,518 distinct texture partitions (1,773,777,471 bytes).
All 1,025 combat destinations already contained their expected values; there
were zero numeric changes. These are import/field checks, not a playthrough or
Xbox framebuffer comparison.

The retained import is
`assets/imported/mcc-4c5bac9f9a4549f295603709972b148a`.
Evidence lives in `analysis/asset-setup/mcc-gameplay-20261013/`.
Focused synthetic tests cover discovery, compressed and raw chunks, malformed
data, exact salted references, texture segments, private map serialization,
source mutations, publication/rollback, shader exclusion and cache ownership.
Relocated launcher tests cover the MCC preparation path before first-run setup.
Independent verification rereads all 883 original/cached file hashes, all 43
directories through the preexisting PC cache reader, and all 19,518 texture
partitions directly from the original segmented stream. It passes in
`verification.json`. Both rebuilt installed launchers pass 27 relocated routing
checks each and retain the previously selected native runtime and TU1.5 XBE.
`analysis/asset-setup/mcc-native-20261013/continuation-verification.json` chains
that historical verification to the current provenance, rehashes all 883
copied files, confirms all 22 source directories and verifies the 14 selected
loading-bank files. The complete gameplay conversion remains inactive.

The continuation builds the native game executable and passes seven focused
CTest groups, including the standalone movie/audio build. The engine-facing
`TagSystem` now uses MCC's checked resident metadata, just as the converted PC
owner does, instead of requiring an original Xbox disk BSP header. All 43 maps
pass this tag-system BSP handoff and native geometry loading.

The earlier reference-executable Zanzibar attempt stopped at
`Original runtime BSP mapping differs from its cache owner`. Raw MCC metadata
is now explicitly rejected at the original-ABI cache boundary, instead of
being delivered as packed Xbox structures. MCC-only loading also refuses an
implicit `default.xbe` fallback. Classified immutable TU1.5 operands now use
the native factory described above; mutable world startup still must replace
the remaining executable-backed banks. The physical cache mapping,
compiled collision/physics and authored semantic conversion remain unfinished.
This is a concrete gameplay admission failure, not a successful map-loading
run. It must be resolved together with full metadata/layout and authored
gameplay conversion; bypassing its original cache contract would not establish
the requested vanilla TU1.5 dataset. The reports and logs are retained under
`analysis/asset-setup/mcc-native-20261013/continuation/`, including `status.json`.
