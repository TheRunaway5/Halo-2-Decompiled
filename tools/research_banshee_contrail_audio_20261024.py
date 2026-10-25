#!/usr/bin/env python3
"""Read Banshee audio function inputs and curves from authenticated original/MCC metadata."""
import csv,hashlib,json,os,struct
from pathlib import Path
from inspect_multiplayer import Cache
from validate_mcc_weapon_vehicle_sound_resources import Bank,merge,schema,SCHEMAS
from audit_weapon_vehicle_sound_presence_tu15 import schema as original_schema
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
ROOT=Path(__file__).resolve().parents[1]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    maps=ROOT/'assets/imported/mcc-dire-valley-polish-20261023/halo2/h2_maps_win64_dx11'
    a=Bank(maps/'coagulation.map');b=Bank(maps/'shared.map');x=Cache(ROOT/'assets/disc/maps/coagulation.map')
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-scenery-audio/contrail-function-research';out.mkdir(exist_ok=True)
    rows=[]
    try:
        for mcc,bank,tag in [(True,owner,t) for owner,t in merge(a,b).values() if t['group']=='vehi' and t['name'].endswith('banshee\\banshee')]+[(False,x,t) for t in x.tags.values() if t['group']=='vehi' and t['name'].endswith('banshee\\banshee')]:
            if tag['offset'] is None:continue
            def name(at):
                value=bank.u(at);index=value&65535
                count,strings,size,indices=(bank.u(q) for q in ((48,52,56,60) if mcc else (356,368,360,364)))
                assert index<count and bank.u(indices+index*4)<size
                pos=strings+bank.u(indices+index*4);end=bank.data.find(b'\0',pos,strings+size);assert end>=pos
                return {'id':value,'name':bank.data[pos:end].decode()}
            fields=(schema if mcc else original_schema)('vehi');root=tag['offset'];functions=[];attachments=[]
            f=next(q for q in fields if q.attrib.get('name')=='Functions');att=next(q for q in fields if q.attrib.get('name')=='Attachments')
            for at in bank.block(root+int(f.attrib['offset'],0),int(f.attrib['elementSize'],0)):
                n=bank.u(at+0x14);pointer=bank.u(at+0x18);assert n<65536
                start=pointer-bank.magic;bank.data[start:start+n]
                functions.append(dict(flags=bank.u(at),imports=name(at+4),exports=name(at+8),turn_off=name(at+12),minimum=struct.unpack_from('<f',bank.data,at+16)[0],scale=name(at+28),curve=bank.data[start:start+n].hex()))
            for at in bank.block(root+int(att.attrib['offset'],0),int(att.attrib['elementSize'],0)):
                datum=bank.u(at+4);attachments.append(dict(type=bank.data[at:at+4].hex(),datum=datum,marker=name(at+8),primary=name(at+16),secondary=name(at+20)))
            rows.append(dict(mcc=mcc,path=str(bank.path),sha256=sha(bank.path),functions=functions,attachments=attachments))
        assert sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        starts={int(r['address'],16) for r in csv.DictReader((ROOT/'analysis/multiplayer/title-update-1.5/decompiled/functions.tsv').open(),delimiter='\t')}
        # Whole authenticated vehicle function callback contains both wingtip names.
        entry=0xf9270
        inventory=json.loads((ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols/inventory.json').read_text())
        assert any(row['entry']==entry and 1 in row['types'] for row in inventory['bodies'])
        body=_trace(code,entry,starts|{entry})
        lines=[]
        for extra in (0xf9e10,0xf9e40):
            more=_trace(code,extra,starts|{entry,0xf9e10,0xf9e40});body.code.update(more.code)
        for pc in sorted(body.code):
            i=code.at(pc);lines.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}')
        p=out/'vehicle-function.asm';p.write_text('\n'.join(lines)+'\n')
        report=dict(scope=__doc__,rows=rows,xbe_sha256=NEW_PIN,callback_entry=entry,callback_sha256=sha(p),metadata_only=True,runtime_modified=False)
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        print(json.dumps(dict(rows=len(rows),contrail_curves_equal=next(f['curve'] for f in rows[0]['functions'] if f['exports']['name']=='contrail_audio')==next(f['curve'] for f in rows[1]['functions'] if f['exports']['name']=='contrail_audio'))))
    finally:a.close();b.close();x.data.close();x.stream.close()
if __name__=='__main__':main()
