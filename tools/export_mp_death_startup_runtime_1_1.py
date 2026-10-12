#!/usr/bin/env python3
"""Package unchanged original startup stores and complete-bank comparison pins."""
import argparse,gzip,hashlib,json
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT,Xbe,PIN

CAPTURE_ORACLE="oracle_mp_death_startup_1_1.py"

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--report',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 j=json.loads(a.report.read_text());assert j['source_sha256']==PIN
 assert j['oracle_sha256']==sha(ROOT/'tools'/CAPTURE_ORACLE)
 assert not j['instruction_patches'] and not j['supplied_gameplay_results']
 image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 listing=a.report.parent/'executed-original.asm';assert sha(listing)==j['instruction_listing_sha256']
 for row in listing.read_text().splitlines():
  pc,raw,*_=row.split();raw=bytes.fromhex(raw);assert image.read(int(pc,16),len(raw))==raw
 stores=a.report.parent/j['store_file'];assert sha(stores)==j['store_sha256'] and stores.stat().st_size==25*j['stores']
 packed=a.output.with_suffix('.stores.gz');a.output.parent.mkdir(parents=True,exist_ok=True)
 packed.write_bytes(gzip.compress(stores.read_bytes(),compresslevel=9,mtime=0))
 j.update(source_report_sha256=sha(a.report),exporter_sha256=sha(Path(__file__)),store_file=packed.name,compressed_store_sha256=sha(packed),native_expected_data_used=False)
 a.output.write_text(json.dumps(j,indent=2)+'\n');print(json.dumps(dict(stores=j['stores'],compressed_bytes=packed.stat().st_size,fixture_sha256=sha(a.output))))
if __name__=='__main__':main()
