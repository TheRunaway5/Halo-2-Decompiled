#!/usr/bin/env python3
"""Trace a genuine Needler contact request to its original authored effect field."""
import argparse,hashlib,json,struct
from pathlib import Path
from inspect_multiplayer import Cache
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from validate_captured_weapon_feedback_tu15 import validate
BASE=ROOT/'analysis/weapons-tu15-goal-20261013'
WEAPON='objects\\weapons\\pistol\\needler\\needler'
SCOPE_PIN='6855b4f33585b332ca82e4b688b9e29c6a7a04eca5757c1bc8fe9b20640c6a1b'
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def banks(path):
    out=[]
    with path.open('rb') as f:
        while header:=f.read(8):
            assert len(header)==8
            address,size=struct.unpack('<II',header);raw=f.read(size);assert len(raw)==size
            out.append((address,raw))
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--actual',type=Path,required=True)
    p.add_argument('--api-evidence',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();assert not a.output.exists()
    api=json.loads(a.api_evidence.read_text());assert api['passed'] and all(api['checks'].values())
    assert api['original_entry']==0x178e30 and api['original_instructions']==34 and api['original_nonstack_writes']==0
    metadata=json.loads((a.actual/'native.json').read_text());memory=banks(a.actual/'before.bin');context=validate(a.actual,metadata,memory)
    def read(address,n):
        for base,raw in memory:
            if base<=address and address+n<=base+len(raw):return raw[address-base:address-base+n]
        raise AssertionError(hex(address))
    def word(address):return int.from_bytes(read(address,4),'little')
    gp=struct.unpack_from('<8I',(a.actual/'native-input.bin').read_bytes());sp=gp[4]
    request=word(sp+4);local_sp=sp+8;definition=word(local_sp+0x24)
    assert request==local_sp+0x60 and context['request']==request
    projectile=context['source_projectile'];tag_row=word(0x4f0414)+(projectile&65535)*16
    assert word(tag_row+4)==projectile and word(tag_row+8)==definition
    assert word(request+4)==word(definition+0xec)==0xffffffff
    assert int.from_bytes((a.actual/'native-exit.bin').read_bytes()[:4],'little')==0xffffffff
    image=Xbe(NEW_DEFAULT);assert sha(NEW_DEFAULT)==NEW_PIN
    code=_Code(image,'.text');body=_trace(code,0xffa60,{0xffa60})
    expected={0x100bac:'8b442424',0x100bb0:'8b88ec000000',0x100bbf:'894c2464',0x100c17:'8d542460',0x100c1b:'52',0x100c1c:'e80f820700'}
    instructions=[]
    for pc,raw in expected.items():
        assert pc in body.code;op=code.at(pc);assert op.bytes.hex()==raw
        instructions.append(dict(address=pc,bytes=raw,mnemonic=op.mnemonic,operands=op.op_str))
    scope_path=BASE/'authored-scope.json';assert sha(scope_path)==SCOPE_PIN
    scope=json.loads(scope_path.read_text());weapon=next(w for w in scope['weapons'] if w['name']==WEAPON)
    map_pins={m['path']:m['sha256'] for m in scope['maps']};rows=[]
    for occurrence in weapon['occurrences']:
        path=Path(occurrence['map']);assert sha(path)==map_pins[str(path)]
        c=Cache(path)
        try:
            tag=c.tags[int(occurrence['datum'],16)];assert tag['name']==WEAPON and tag['group']=='weap'
            row=dict(map=str(path),map_sha256=map_pins[str(path)],weapon_datum=occurrence['datum'],resident=tag['offset'] is not None,barrels=[])
            if tag['offset'] is not None:
                for index,at in enumerate(c.block(tag['offset']+0x2d0,0xec)):
                    datum=c.u(at+0x90);projectile_tag=c.tags[datum];assert projectile_tag['group']=='proj'
                    record=dict(barrel=index,projectile=projectile_tag,resident=projectile_tag['offset'] is not None)
                    if projectile_tag['offset'] is not None:
                        location=projectile_tag['offset']+0xe8;raw=bytes(c.data[location:location+8])
                        assert c.u(location)==0x65666665
                        record.update(effect_reference_file_offset=location,effect_reference_hex=raw.hex(),effect_datum=c.u(location+4))
                        if path==Path(context['map']):
                            assert datum==projectile and raw==read(definition+0xe8,8)
                            record['matches_actual_captured_definition']=True
                    row['barrels'].append(record)
            rows.append(row)
        finally:c.data.close();c.stream.close()
    references=[r for row in rows for r in row['barrels'] if r['resident']]
    report=dict(scope=__doc__,passed=True,actual_entry=0x178e30,actual_caller=0x100c1c,actual_tick=context['sequence_tick'],request=request,request_flags=context['request_flags'],request_effect=word(request+4),source_projectile=projectile,source_projectile_definition=definition,definition_reference_offset=0xe8,definition_effect_datum_offset=0xec,request_effect_offset=4,reference_bytes=read(definition+0xe8,8).hex(),actual_callee_result=0xffffffff,original_instruction_path=34,original_nonstack_writes=0,retail_request_producer_instructions=instructions,scoped_needler_occurrences=len(weapon['occurrences']),map_count=len(set(row['map'] for row in rows)),resident_barrel_references=len(references),null_barrel_references=sum(r['effect_datum']==0xffffffff for r in references),unresolved_nonresident_weapons=sum(not r['resident'] for r in rows),unresolved_nonresident_projectiles=sum(not r['resident'] for row in rows for r in row['barrels']),maps=rows,original_xbe_sha256=NEW_PIN,authored_scope_sha256=SCOPE_PIN,actual_API_evidence_sha256=sha(a.api_evidence),tool_sha256=sha(Path(__file__)),conclusion='The actual null contact descriptor is copied from the original Needler projectile effect reference; untouched TU1.5 code returns null for this same request. This is expected absence for the tested field and call, not proof of other visual producers.',other_visual_producers_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','actual_tick','map_count','resident_barrel_references','null_barrel_references','unresolved_nonresident_weapons','unresolved_nonresident_projectiles')}))
if __name__=='__main__':main()
