# Energy-blade bitmap partition evidence

`tools/oracle_source_bitmap_partitions.py` reads the literal Zanzibar
`energy_blade_bottom` shader bitmap bank. Its four bindings select record0
of `E2E1016C`, `E2E2016D`, `E2E3016E`, and `E2E4016F`. No captured texture
dimensions, fitted mip offsets, GPU, or guest are inputs to this proof.

Complete unchanged `12D6D0` constructs each original 20-byte D3D texture
header. At `12D75A–12D781`, the partition argument selects the literal float
0, 1, or 2, converted to the authored base mip. The constructor shifts
authored dimensions at record `+4/+6/+8` by that value, subtracts it from
the authored additional-mip maximum at `+14`, and calls unchanged `359E0`.
The header's data word is runtime `+40[partition] & 0x0FFFFFFF`. The source
2D depth1 shifts to0 for partitions1/2, while the encoded header retains
depth1; the native comparison uses the actual decoded 2D depth.

The original headers establish these dimensions and total resident mip
counts. BC chains stop when their largest side reaches4, so the final
rectangular mip is4×1.

| Bitmap | Partition0 | Partition1 | Partition2 |
| --- | --- | --- | --- |
| Energy blade noise1 | 64×64,5 mips | 32×32,4 mips | 16×16,3 mips |
| Jackal shield noise | 128×128,6 mips | 64×64,5 mips | 32×32,4 mips |
| Blade shield offset | 256×64,7 mips | 128×32,6 mips | 64×16,5 mips |
| Blade shield mask | 256×64,7 mips | 128×32,6 mips | 64×16,5 mips |

The disk record has separate resource pointers at `+1C/+20/+24` and byte
sizes at `+34/+38/+3C`. Noise1 sizes are0xB00/0x300/0x100, with88 padding
bytes per payload after the header-admitted mip tail. The other three use
0x5580/0x1580/0x580. Jackal noise has48 padding bytes per partition; the
rectangular offset/mask have16. Each partition's admitted BC bytes exactly
equal the corresponding authored mip tail in partition0. The report retains
each pointer, payload hash, padding count, and per-mip dimensions/byte hashes.

The same tool independently executes `12D5A0` through genuine `13D370`
cache allocation and `16B910` image-pool allocation. It initializes their
metadata with unchanged `16B5F0`, `16B7A0`, `13D170`, and `13D8B0`, using
the original `12C0D0` counts/strides:512 image entries of40 bytes and512 cache
entries of24 bytes, with a12-bit allocation shift. Preallocated CPU storage
and an empty4MiB cache bank at declared address0x08100000 are explicit caller
inputs. Their capacities/addresses are not a whole-title budget producer or
an observed GPU allocation.

For every partition, `12D65D` writes the source salted cache handle to
`+28[partition]`, `12D661` writes the allocated data address to
`+40[partition]`, and partition0 also writes `+54`. Authored `+34` sizes
remain unchanged. `12D66E` calls the complete header constructor. The trace
then stops exactly at `213760`, before asynchronous resource I/O, and records
the original resource pointer and all six request arguments. The requested
read size is authored size rounded up to512 bytes. No allocator, math,
header, readiness, or I/O routine is replaced and no I/O result is supplied.
This proves the allocation prefix and the complete pure header constructor;
it does not prove original resource I/O completion or residency readiness.

The frozen fixture contains12 complete headers, totaling60 original words.
`tests/source_bitmap_partition_tests.cpp` independently compares existing
native `retail_mip_count`, bitmap resource loading, and compressed texture
preparation:48 metadata words and63 authored BC mip tails totaling89,736
bytes match exactly. The independent test is registered in CMake as
`retail_source_bitmap_partitions` and can also be built against a private
`halo2_cache` library and passed to the oracle with `--native`.
Bitmap request policy, cache selection, runtime residency scheduling, GPU
filtering, and framebuffer parity remain separate scopes.
