#!/usr/bin/env python3
"""Actual TU1.1 sword/Needler row construction and equipped pickup fields.

Inherits the explicitly bounded original equipped-player fixture. Exact
weapon animation resource bytes are supplied as resident backing, not as a
resource scheduler result. No full live physics/startup/power evolution proof.
"""
import hashlib,json,struct
from pathlib import Path
from oracle_mp_weapon_attachment_1_1 import AttachmentTrace,ROOT,PIN,CACHE_PIN,HEADERS
from unicorn.x86_const import UC_X86_REG_EIP

class Trace(AttachmentTrace):
    def hook(self,g,a,size,user):
        if a==0x15c110 and not getattr(self,'covenant_resident',False):
            self.covenant_resident=True;self.covenant_resources=[]
            g.mem_map(0x8000000,0x800000);at=0x8000000;c=self.cache
            with (ROOT/'assets/disc/maps/shared.map').open('rb') as shared:
                for tag in c.tags.values():
                    if tag['group']!='jmad' or tag['name'] not in (
                        'objects\\weapons\\pistol\\needler\\needler',
                        'objects\\weapons\\melee\\energy_blade\\energy_blade'):continue
                    for i,off in enumerate(c.block(tag['offset']+0xac,20)):
                        size,pointer=c.u(off+4),c.u(off+8);location=pointer>>30;offset=pointer&0x3fffffff
                        if not size:continue
                        source=('assets/disc/maps/lockout.map','assets/disc/maps/mainmenu.map',
                            'assets/disc/maps/shared.map','assets/disc/maps/single_player_shared.map')[location]
                        if location:
                            with (ROOT/source).open('rb') as bank:bank.seek(offset);raw=bank.read(size)
                        else:raw=bytes(c.data[offset:offset+size])
                        assert len(raw)==size and at+size<=0x8800000
                        g.mem_write(at,raw)
                        resource={'resident_pointer':at,'index':i,'size':size,'file_offset':offset,
                            'file':source,
                            'location':location,'descriptor_hex':bytes(c.data[off:off+20]).hex(),
                            'raw_sha256':hashlib.sha256(raw).hexdigest()}
                        self.resident[off+c.magic]=resource
                        self.covenant_resources.append({'graph':tag['name'],**resource});at+=(size+4095)&~4095
        super().hook(g,a,size,user)
    def shader_state(self):
        p=self.target_pointer
        return {'datum':f'{self.target_datum:08x}','definition':f'{self.u(p):08x}',
            'parent':f'{self.u(p+0x14):08x}','item_flags':f'{self.u(p+0x12c):08x}',
            'weapon_flags':f'{self.u(p+0x16c):08x}','power_bits':f'{self.u(p+0x184):08x}',
            'loaded':struct.unpack('<h',self.g.mem_read(p+0x22c,2))[0]}

def main():
    rows=[]
    for row in (2,6):
        t=Trace.__new__(Trace)
        try:
            t.__init__(row);before=t.shader_state()
            t.seeded_duration(2,232);t.poll('original_covenant_233ms_pickup',{2:255},milliseconds=1)
            after=t.shader_state()
            result={'authored_row':row,'before':before,'after':after,
                'original_parent_attached':after['parent']!='ffffffff','completed':True}
        except Exception as error:
            result={'authored_row':row,'completed':False,'phase':getattr(t,'active',None),
                'first_stop_instruction':f'{t.g.reg_read(UC_X86_REG_EIP):08x}',
                'error':str(error),'supplied_missing_owner_result':False}
        result.update(resources=getattr(t,'covenant_resources',[]),
            used_services=sorted({e['service']for e in t.events if 'service' in e}))
        rows.append(result);print(json.dumps({k:v for k,v in result.items()if k not in ('resources','used_services')}),flush=True)
    path=ROOT/'analysis/multiplayer/integration/covenant-pickup-shader-state-20261011.json'
    path.parent.mkdir(parents=True,exist_ok=True)
    path.write_text(json.dumps({'scope':__doc__,'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'cases':rows},indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
