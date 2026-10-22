#!/usr/bin/env python3
"""Declare original creation epoch/latch and bounded per-object stamp storage.

The real object-header constructor supplies2048 slots. Valid object lookup
precedes index*4 stamp access; the original allocator bounds high-water growth
by capacity. These exact8197 bytes are loader-zero writable fields. Preserve
existing mutable owners and their live values; never reset an epoch, stamp or
allocation outcome, and never admit surrounding alignment bytes.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    args = ap.parse_args()
    assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, '.text')
    capacity = code.at(0xb8811)
    assert capacity.mnemonic == 'mov' and capacity.op_str == 'eax, 0x800'
    assert code.at(0xb8816).mnemonic == 'call' and code.at(0xb8816).operands[0].imm == 0x16e3d0
    assert code.at(0xb881b).operands[0].mem.disp == 0x4ecbd0
    assert code.at(0x16e4da).op_str == 'dword ptr [esi + 0x20], ecx'
    assert code.at(0x16e6db).op_str == 'edi, dword ptr [edx + 0x20]'
    assert code.at(0x16e724).op_str == 'dword ptr [edx + 0x38], eax'
    assert code.at(0xbce0c).op_str == 'ecx, eax' and code.at(0xbce0e).mnemonic == 'jge'
    stamp = code.at(0x273bab).operands[0]
    assert stamp.type == X86_OP_MEM and stamp.mem.disp == 0x4eabd0 and stamp.mem.scale == 4 and stamp.size == 4
    fields = ((0x4eabc8, 1), (0x4eabcc, 4), (0x4eabd0, capacity.operands[1].imm * stamp.size))
    writes = []
    for pc, address, size in ((0x27446c, 0x4eabc8, 1), (0x274466, 0x4eabcc, 4), (0x273bb8, 0x4eabd0, 4)):
        ins = code.at(pc)
        assert ins.operands[0].mem.disp == address and ins.operands[0].size == size
        writes.append(dict(pc=pc, instruction_hex=bytes(ins.bytes).hex(), address=address, bytes=size))
    for address, size in fields:
        section = next(s for s in image.sections if s.va <= address < address + size <= s.va + s.vsize)
        assert address >= section.va + section.rsize and image.read(address, size) == bytes(size)
    source = '''// Original creation epoch/latch and per-object stamp fields; no live state.
{
constexpr std::array<std::array<std::uint32_t,2>,3> fields{{
    {0x4eabc8u,1u},{0x4eabccu,4u},{0x4eabd0u,8192u}}};
auto owned=[&](std::uint32_t address) {
    try {(void)memory.view(address,1);return true;}
    catch(const std::out_of_range&){return false;}
};
// Check every existing owner before admitting missing fields. Existing
// mutable values and backing lifetimes belong to their original producers.
for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;++i)
    if(owned(address+i) && memory.is_read_only(address+i,1))
        throw std::invalid_argument("Original property creation storage is read-only");
for(const auto& [address,count]:fields)for(std::uint32_t i=0;i<count;) {
    if(owned(address+i)){++i;continue;}
    const auto first=i++;
    while(i<count && !owned(address+i))++i;
    memory.mount(address+first,std::vector<std::byte>(i-first),SourceRuntimeMemory::Access::ReadWrite,
        "Original TU1.5 loader-zero property creation epoch/latch/object stamps");
}
}
'''
    args.output.write_text(source)
    pcs = (0xb8808, 0xb880a, 0xb880c, 0xb8811, 0xb8816, 0xb881b, 0x16e4da,
           0x16e6db, 0x16e724, 0xbcdfb, 0xbce01, 0xbce06, 0xbce0c, 0xbce0e, 0x273b87, 0x273b94, 0x273bab)
    report = dict(scope=__doc__, original_xbe_sha256=NEW_PIN, fields=fields, writable_bytes=sum(n for _, n in fields),
                  object_capacity=2048, per_object_stamp_bytes=4, original_writes=writes,
                  bounds_instructions=[[pc, bytes(code.at(pc).bytes).hex(), code.at(pc).mnemonic, code.at(pc).op_str] for pc in pcs],
                  existing_mutable_values_and_lifetimes_preserved=True, existing_owners_checked_before_missing_fields=True,
                  padding_excluded=True, runtime_results_supplied=False, full_property_creation_qualified=False,
                  data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
