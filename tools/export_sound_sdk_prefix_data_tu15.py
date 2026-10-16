#!/usr/bin/env python3
"""Export exact sound SDK cold/literal operands; original XBE is offline only."""
import argparse,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

# Cold counters, input/output/fault cells, singleton/context and CRT bindings.
# The constructors387BAA/387BB5, not this loader, bind the statistics pointers.
COLD=[(0x390bc0,0x1c),(0x390be4,4),(0x390bec,4),(0x3911ec,4),
      (0x391200,4),(0x391208,4),(0x391278,4),(0x3912a0,8),
      (0x39134c,8),(0x391360,0x100)]
CONFIG=[(0x390be8,4)]
READONLY=[(0x38f898,48),(0x38f8cc,60),(0x390b70,8),
          (0x4211c0,12),(0x42124c,12),(0x4212ac,4),(0x4212b0,12),
          (0x4212f0,12),(0x421374,12),
          (0x41b67c,4),(0x41b680,4),(0x41b79c,4),(0x41b7ec,4),(0x41b640,4),
          (0x41b7ac,4),(0x41b6fc,4)]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
 lines=['// Authenticated TU1.5 SDK operands; no retail executable is read at runtime.'];rows=[]
 for address,size in COLD:
  raw=image.read(address,size);assert not any(raw),(hex(address),raw.hex())
  lines.append(f'cold(memory,0x{address:x}u,0x{size:x}u);');rows.append(dict(address=address,bytes=size,access='read-write',hex=raw.hex(),kind='loader zero'))
 for group,writable in [(CONFIG,True),(READONLY,False)]:
  for address,size in group:
   raw=image.read(address,size)
   if writable:assert raw==b'\xff'*4
   lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{size}>{{'+','.join(f'0x{v:02x}'for v in raw)+'}'+(',true);'if writable else');'))
   rows.append(dict(address=address,bytes=size,access='read-write'if writable else'read-only',hex=raw.hex(),kind='explicit unselected input'if writable else'immutable operand'))
 a.output.write_text('\n'.join(lines)+'\n');a.manifest.parent.mkdir(parents=True,exist_ok=True)
 a.manifest.write_text(json.dumps(dict(original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),data_inc_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),regions=rows,genuine_prior_constructors=[0x387baa,0x387bb5,0x38a988,0x38a993],supplied_constructed_state=[]),indent=2)+'\n')
 print(f'{len(rows)} exact SDK literal/cold regions; {sum(r["bytes"]for r in rows)} bytes')
if __name__=='__main__':main()
