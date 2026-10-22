#!/usr/bin/env python3
"""Export the original writable seven-entry camera-effect request type table.

The typed retail declaration supplies the bound; TU1.5 supplies every byte.
Preserve authored writable storage, the two-byte following gap and the separate
director-name table. No live request, timer or completed effect is published.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--report',type=Path,required=True)
    args=ap.parse_args();assert args.output.resolve().is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve())
    root=Path(__file__).resolve().parents[1]
    reference=root/'tools/reference/halo2-decompiled/src/unknown_153950.cpp';source=reference.read_bytes()
    match=re.search(r'short g_468cec\[(\d+)\] =\s*\{([^}]+)\}',source.decode())
    values=[int(n)for n in re.findall(r'-?\d+',match[2])];assert len(values)==int(match[1])==7
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    ins=_Code(image,'.text').at(0x1578c9);assert bytes(ins.bytes).hex()=='66833c4d4c52470000'
    data=image.read(0x47524c,14);assert list(struct.unpack('<7h',data))==values
    section=next(s for s in image.sections if s.va<=0x47524c<0x47524c+14<=s.va+s.rsize)
    assert section.name=='.data'
    text='''// Original authored writable camera-effect request type table; no live state.
{
constexpr std::array<std::uint8_t,14> types{'''+','.join(f'0x{b:02x}'for b in data)+'''};
memory.mount(0x47524cu,std::as_bytes(std::span(types)),SourceRuntimeMemory::Access::ReadWrite,
    "Original TU1.5 authored camera-effect request type table");
}
'''
    args.output.write_text(text)
    report=dict(original_xbe_sha256=NEW_PIN,address=0x47524c,bytes=len(data),elements=7,element_bytes=2,
        access='read-write',original_section=section.name,following_gap=[0x47525a,2],original_read_pc=ins.address,
        original_read_hex=bytes(ins.bytes).hex(),typed_reference=str(reference.relative_to(root)),
        typed_reference_sha256=hashlib.sha256(source).hexdigest(),original_data_sha256=hashlib.sha256(data).hexdigest(),
        generated_data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),runtime_original_reads=False,live_state_supplied=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
