#!/usr/bin/env python3
"""Export literal cold storage/metadata for unchanged startup slots33..44."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
COLD=[(0x4c6854,0x40),(0x4ce268,0x48),(0x4ce3f0,4),(0x491878,0x6c),
      (0x515f0c,4),(0x515f14,4),(0x515f24,8),(0x4f9b50,4),(0x4f722c,4),
      (0x4f9330,0x20),(0x4f5504,0x510),(0x51df6c,12),(0x4f7208,0x1c),
      (0x52d714,12),(0x50ebf0,4),(0x4d4fb8,0x1000)]
READONLY=[(0x46a0f0,8),(0x4489c8,0x468),(0x45c5e0,28),(0x45c600,20),
          (0x45f864,0x4c),(0x4662c8,0x38)]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 lines=['// Authenticated TU1.5 literals. Do not read a retail executable at runtime.'];rows=[]
 for address,size in COLD:
  raw=x.read(address,size);assert not any(raw),(hex(address),raw.hex())
  lines.append(f'cold(memory,0x{address:x}u,0x{size:x}u);')
  rows.append(dict(address=address,bytes=size,access='read-write',sha256=hashlib.sha256(raw).hexdigest()))
 for address,size in READONLY:
  raw=x.read(address,size)
  lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{size}>{{'+','.join(f'0x{b:02x}' for b in raw)+'});')
  rows.append(dict(address=address,bytes=size,access='read-only',sha256=hashlib.sha256(raw).hexdigest()))
 a.output.write_text('\n'.join(lines)+'\n');a.manifest.parent.mkdir(parents=True,exist_ok=True)
 a.manifest.write_text(json.dumps(dict(original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),data_cpp_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),regions=rows),indent=2)+'\n')
 print(f'{len(rows)} literal regions; {sum(row["bytes"] for row in rows)} bytes')
if __name__=='__main__':main()
