#!/usr/bin/env python3
"""Publish the original35-WORD allied-team difficulty remapping table.

Original1F19D0 reads450210 +2*row. The recovered typed retail declaration
supplies the array bound; every value is authenticated against TU1.5 bytes.
This is immutable engine metadata, not converted tag data or live outcomes.
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
    reference=root/'tools/reference/halo2-decompiled/src/unknown_1e9700.cpp';source=reference.read_bytes()
    match=re.search(r'const short g_445580\[(\d+)\] =\s*\{([^}]+)\}',source.decode())
    values=[int(n)for n in re.findall(r'-?\d+',match[2])];assert len(values)==int(match[1])==35
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    ins=_Code(image,'.text').at(0x1f1a3c);assert bytes(ins.bytes).hex()=='668b044d10024500'
    data=image.read(0x450210,70);assert list(struct.unpack('<35h',data))==values
    text='// Original35-WORD allied-team difficulty remapping; no live state.\n'
    text+='literal(memory,0x450210u,std::array<std::uint8_t,70>{'+','.join(f'0x{b:02x}'for b in data)+'});\n'
    args.output.write_text(text)
    report=dict(original_xbe_sha256=NEW_PIN,address=0x450210,bytes=len(data),elements=35,element_bytes=2,
        original_read_pc=ins.address,original_read_hex=bytes(ins.bytes).hex(),typed_reference=str(reference.relative_to(root)),
        typed_reference_sha256=hashlib.sha256(source).hexdigest(),original_data_sha256=hashlib.sha256(data).hexdigest(),
        generated_data_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),runtime_original_reads=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
