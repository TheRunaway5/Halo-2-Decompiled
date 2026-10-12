#!/usr/bin/env python3
"""Static whole-body correspondence of original vehicle and collision owners.

No vehicle construction/update is executed; direct callbacks, metadata and
mutable operands stay separate from instruction topology correspondence.
"""
import json,struct,sys
from pathlib import Path
import research_unit_pose_requests_u5 as paired

ENTRIES=(0xf01c0,0xf1420,0xeff50,0xefff0,0xf8350,0xf2590,
         0xf3310,0xf35c0,0xf39a0,0xf4650,0xf5e40,0x2090a0,
         0x16a490,0x16af90,0x1f3500,0x1f3900,0x10e340,0x10e480,0x10e1c0)

def main():
 paired.ENTRIES=ENTRIES
 paired.main()
 # Reuse only the complete original-byte comparator, preserving its own pins.
 option=sys.argv.index('--output-dir')
 out=Path(sys.argv[option+1]);report=json.loads((out/'report.json').read_text())
 report['scope']=__doc__.strip()
 report['wrapper_sha256']=paired.sha(Path(__file__))
 report['comparator_sha256']=paired.sha(Path(paired.__file__))
 images={}
 packet=paired.ROOT/'analysis/multiplayer/integration/spawn-source/object-type-construction/u5/metadata-packet.json'
 metadata=json.loads(packet.read_text())
 for label,path in [('old',paired.OLD),('u5',paired.NEW)]:
  image=paired.Xbe(path);rows=[]
  for kind in (1,6):
   record=next(r for r in metadata['images'][label]['types']if r['type']==kind)
   va=int(record['address'],16);raw=image.read(va,0x94)
   rows.append(dict(type=kind,record_va=f'{va:08x}',raw94_hex=raw.hex(),
    word_offsets={f'{offset:02x}':f'{struct.unpack_from("<I",raw,offset)[0]:08x}'for offset in range(0,0x94,4)},
    inheritance_pointers=record['bank']))
  images[label]=dict(image_sha256=paired.sha(path),type_records=rows)
 report['type_metadata']={'prior_packet_sha256':paired.sha(packet),'images':images,
  'scope':'Literal complete type-1/type-6 records from original image; field roles require actual consumer evidence. No method call return supplied or executed here.'}
 paired.save(out/'vehicle-report.json',paired.encoded(report))
 print(json.dumps(dict(final_report_sha256=paired.sha(out/'vehicle-report.json'))))

if __name__=='__main__':main()
