"""Read actual initialized kernel export data in an isolated owned xemu boot.

Run inside GDB with HALO2_KERNEL_DATA_OUTPUT pointing at a new directory. This
tool reads PE export metadata and data only. It neither patches code, writes
guest memory/registers nor supplies a game/platform return. A modified Complex
BIOS in xemu is a named emulator reference, not physical retail Xbox proof.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import gdb

out = Path(os.environ['HALO2_KERNEL_DATA_OUTPUT'])
out.mkdir(parents=True, exist_ok=False)
inferior = gdb.selected_inferior()
read = lambda p, n: bytes(inferior.read_memory(p, n))
word = lambda p: struct.unpack('<I', read(p, 4))[0]
base = 0x80010000
dos = read(base, 64)
assert dos[:2] == b'MZ'
pe = base + word(base+0x3c)
assert read(pe, 4) == b'PE\0\0'
coff = read(pe+4, 20)
assert struct.unpack_from('<H', coff)[0] == 0x14c
optional_bytes = struct.unpack_from('<H', coff, 16)[0]
optional = read(pe+24, optional_bytes)
assert struct.unpack_from('<H', optional)[0] == 0x10b
image_bytes = struct.unpack_from('<I', optional, 56)[0]
export_rva, export_bytes = struct.unpack_from('<II', optional, 96)
assert 0 < export_rva < image_bytes and export_bytes >= 40
exports = read(base+export_rva, 40)
ordinal_base, count, name_count, table_rva = struct.unpack_from('<IIII', exports, 16)
assert ordinal_base <= 322 < ordinal_base+count and ordinal_base <= 324 < ordinal_base+count
table = read(base+table_rva, count*4)
records = {}
for ordinal, label in ((322, 'XboxHardwareInfo'), (324, 'XboxKrnlVersion')):
    rva = struct.unpack_from('<I', table, (ordinal-ordinal_base)*4)[0]
    assert rva+8 <= image_bytes and not export_rva <= rva < export_rva+export_bytes
    pointer = base+rva
    raw = read(pointer, 8)
    (out/(label+'.bin')).write_bytes(raw)
    records[str(ordinal)] = dict(name=label, address=pointer, bytes=8,
        hex=raw.hex(), sha256=hashlib.sha256(raw).hexdigest())
metadata = dos+read(pe, 24)+optional+exports+table
(out/'export-metadata.bin').write_bytes(metadata)
report = dict(schema=1, tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    scope=__doc__, kernel_base=base, kernel_image_bytes=image_bytes,
    export_ordinal_base=ordinal_base, export_count=count, export_name_count=name_count,
    metadata_sha256=hashlib.sha256(metadata).hexdigest(), records=records,
    pc=int(gdb.parse_and_eval('$eip')) & 0xffffffff,
    guest_memory_writes=[], guest_register_writes=[], instruction_patches=[],
    supplied_callee_returns=[], physical_xbox_proof=False, title_execution_proof=False)
(out/'report.json').write_text(json.dumps(report, sort_keys=True, indent=2)+'\n')
print(json.dumps(dict(records=records, pc=report['pc'], physical_xbox_proof=False)))
