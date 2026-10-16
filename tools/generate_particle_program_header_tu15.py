#!/usr/bin/env python3
"""Recover original programmable-header kind without claiming full SDK boot."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_trace
from generate_lens_query_tu15 import QueryCode
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=QueryCode(image)
 call=code.at(0x3ff37b);assert call.mnemonic=='call' and call.operands[0].imm==0x405cf0
 body=_trace(code,0x405cf0,{0x405cf0});sites=(0x406517,0x406519,0x40651e,0x406523,0x406525);assert all(p in body.code for p in sites)
 expected=('eax, eax','ecx, 0x46','edi, 0x4125f8','dword ptr es:[edi], eax','dword ptr [0x4125fc], 0x10')
 assert tuple(code.at(p).op_str for p in sites)==expected
 header=Path('include/halo2/source_particle_program_header_tu15.hpp')
 header.write_text('#pragma once\n#include <cstdint>\nnamespace halo2 {\n// Original405CF0 literal406525 after its70-word header initialization.\n// Only the programmable-kind fragment is admitted by the native resource\n// binding. No zero declaration, input mask or original boot history is exposed.\ninline constexpr std::uint32_t source_particle_program_flags_tu15=0x10;\n}\n')
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(dict(original_sha256=NEW_PIN,constructor=0x405cf0,authenticated_caller=0x3ff37b,header=0x4125f8,words=70,flags=16,literal_sites={f'{p:08x}':bytes(code.at(p).bytes).hex()for p in sites},full_sdk_constructor_or_boot_history_claimed=False,header_sha256=hashlib.sha256(header.read_bytes()).hexdigest()),indent=2)+'\n')
if __name__=='__main__':main()
