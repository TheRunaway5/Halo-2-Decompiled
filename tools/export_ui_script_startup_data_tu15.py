#!/usr/bin/env python3
"""Export literal cold storage/metadata for unchanged prior UI constructor and startup slots51..58."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_ui_effect_startup_tu15 import COLD
READONLY=[(0x465c14,0xe4),(0x461af0,0x20),(0x462210,0x18),(0x4621e8,20),(0x462240,12)]
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
