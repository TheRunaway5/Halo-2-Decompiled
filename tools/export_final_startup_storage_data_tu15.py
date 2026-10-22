#!/usr/bin/env python3
"""Export authenticated cold fields and metadata read by final startup59..68.

The reference trace discovers literal read extents; every exported byte is
re-read and authenticated against the pinned TU1.5 image. Full packed field
rows include their untouched fourth-byte padding. Existing arena/raw-storage
and CRC owners remain required inputs, never imported captured live values.
"""
import argparse, hashlib, json
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
COLD=[(0x48605c,4),(0x486070,4),(0x4f3010,4),
 # Original1B08B0 gathering: one context word,3*131 packed24-byte records,
 #131 byte counts and131 validity bits. Its actual source phase writes
 #before reading; record padding remains original prior storage.
 (0x4fafc0,4),(0x4fafc8,3*131*24),(0x4fd4a0,131),(0x4fd524,5*4),
 (0x4fd538,0x20fc),(0x4ff798,131*20*4),
 # Original1D2D90/1D2E30 produce two12-byte AI scratch leases, their
 # heap-use DWORD and initialization byte; padding retains prior contents.
 (0x5020a4,8),(0x50208c,24),(0x5020c8,4),
 # Original203C00 clears ceil(cluster_count/32) DWORDs, then publishes
 # the cluster key separately. BSP clusters are bounded to512. Original
 # constructor1CFB79 initializes the key toFFFF; never seed a cached result.
 (0x502200,0x40),(0x502240,2),(0x505e70,0x10),(0x50eedc,8),
 # Original262590 publishes actor/script context before211840 and clears it
 # after the callback. Preserve the eight loader-zero bytes, not live IDs.
 (0x50eee4,8),
 (0x50eeec,0x14),(0x510fa4,0x10),(0x510fb8,0x320),(0x51df48,1),(0x52be44,4),
 (0x52be5c,4),(0x52be54,4),(0x52be58,1),(0x52be70,4),(0x52be7c,8),(0x52d688,12),(0x52d87c,4),
 #26A380 publishes this query kind and retained-call context before140910
 # invokes the original269DF0 callback. Their two-byte gap stays unowned.
 (0x52d874,2),(0x52d878,4),(0x52d88c,4),
 # Actual1B08D9/1B08F1 supplies this loader-zeroed20-byte destination to
 # whole1B0720. Original poisoned-input runs prove all words precede reads.
 (0x56ae84,0x14),(0x56aeb0,4),(0x56aeb8,0x14),(0x56e080,12),
 # Original3826F0 publishes this separate AI geometry scalar before AI storage.
 (0x56e08c,4),
 # Whole164EF0 writes this one-byte time-extension announcement latch at
 #164FAF/164FBB. Startup's aligned read also covers it; that read does not
 # make this loader-zeroed mutable field an immutable literal.
 (0x56e224,1)]
SHARED=[(0x4f2950,8),(0x4f295c,4),(0x51df2c,4),(0x51df44,4),(0x56e225,1),(0x56e258,0x400)]

def regions(points):
 rows=[]
 for address in sorted(points):
  if not rows or address!=rows[-1][0]+rows[-1][1]:rows.append([address,1])
  else:rows[-1][1]+=1
 return rows

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--trace',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--manifest',type=Path,required=True);a=p.parse_args()
 x=Xbe(NEW_DEFAULT);assert hashlib.sha256(x.data).hexdigest()==NEW_PIN
 report=json.loads(a.trace.read_text());assert report['passed'] and report['original_xbe_sha256']==NEW_PIN and not report['instruction_patches'] and not report['supplied_game_results']
 baseline=report['cases'][0]['accessed_static_regions'];assert all(case['accessed_static_regions']==baseline for case in report['cases'])
 points=set()
 for row in baseline:
  address,size=row['address'],row['bytes'];assert x.read(address,size).hex()==row['initial_hex']
  # Optimized source copies read aligned words through each NUL. Group the
  # authenticated neighboring bytes into their actual four-byte storage unit.
  points.update(range(address&~3,(address+size+3)&~3))
 for address,size in COLD+SHARED:points.difference_update(range(address,address+size))
 lines=['// Authenticated final TU1.5 startup data; no executable is read at runtime.'];rows=[]
 for address,size in COLD:
  raw=x.read(address,size);assert not any(raw),hex(address)
  lines.append(f'cold(memory,0x{address:x}u,0x{size:x}u);');rows.append(dict(address=address,bytes=size,access='read-write',sha256=hashlib.sha256(raw).hexdigest()))
 for address,size in regions(points):
  raw=x.read(address,size)
  lines.append(f'literal(memory,0x{address:x}u,std::array<std::uint8_t,{size}>{{'+','.join(f'0x{b:02x}' for b in raw)+'});')
  rows.append(dict(address=address,bytes=size,access='read-only',sha256=hashlib.sha256(raw).hexdigest()))
 a.output.write_text('\n'.join(lines)+'\n');a.manifest.parent.mkdir(parents=True,exist_ok=True)
 a.manifest.write_text(json.dumps(dict(original_xbe_sha256=NEW_PIN,tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),reference_trace_sha256=hashlib.sha256(a.trace.read_bytes()).hexdigest(),data_inc_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),regions=rows,shared_inputs=SHARED),indent=2)+'\n')
 print(f'{len(rows)} literal/cold regions; {sum(row["bytes"] for row in rows)} bytes')
if __name__=='__main__':main()
