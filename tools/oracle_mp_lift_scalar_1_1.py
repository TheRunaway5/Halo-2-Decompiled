#!/usr/bin/env python3
"""Untouched original phantom scalar kernel, with explicit contact/velocity inputs.

This independently measures the scalar helper; no original contact, object,
activation or Havok/world producer is fabricated or claimed complete.
"""
import argparse,csv,hashlib,json,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,ROOT,PIN,_Code,_trace
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import *
from inspect_mp_sandbox_source import inspect,sha
TIME,OUT,STACK,STOP=0x1000000,0x1001000,0x200d000,0x3000000
def fb(f):return struct.unpack('<I',struct.pack('<f',f))[0]
def main():
    a=argparse.ArgumentParser();a.add_argument('--output',type=Path,required=True);args=a.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
    x=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');assert sha(x.path)==PIN
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
    for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
    for p in (TIME,STACK&~0xffff,STOP):g.mem_map(p,0x10000)
    def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
    w(0x518444,TIME);w(TIME+4,fb(1/30))
    executed=set();g.hook_add(UC_HOOK_CODE,lambda _g,p,_s,_u:executed.add(p))
    cases=[]
    for name in ('lockout','colossus','cyclotron','foundation','headlong','midship'):
        m=inspect(ROOT/'assets/disc/maps'/f'{name}.map')
        for o in m['objects']:
            if not o['placements']or not o['model']or 'physics_data'not in o['model']:continue
            for r in o['model']['physics_data']['phantom_types']:
                flags=int(r['flags00'],16)
                for component,ao,mo in (('axis','28','2c'),('direction','30','34')):
                    acceleration=r['fields'][ao]['value'];limit=r['fields'][mo]['value']
                    if not acceleration or not limit:continue
                    for distance in ([10000.]if component=='direction'else [-1.,0.,0.01,1.,10.]):
                        for velocity in (-limit*2,-limit,-1.,0.,1.,limit-0.01,limit,limit*2):
                            inputs=[(flags>>1)&1,fb(1.),fb(distance),fb(acceleration),fb(velocity),fb(limit),int(r['fields']['1c']['bits'],16),int(r['fields']['18']['bits'],16),(flags>>25)&1,OUT]
                            w(STACK,STOP)
                            for i,v in enumerate(inputs):w(STACK+4+4*i,v)
                            w(OUT,0xcdcdcdcd)
                            for reg,value in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_EBX,1),(UC_X86_REG_FPCW,0x027f),(UC_X86_REG_MXCSR,0x1f80),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):g.reg_write(reg,value)
                            g.emu_start(0x1d5c70,STOP,count=5000);assert g.reg_read(UC_X86_REG_EIP)==STOP
                            cases.append({'map':name,'object_tag':o['tag'],'phantom_rule':r,'component':component,'explicit_contact_distance':distance,'explicit_projected_velocity':velocity,'explicit_mass_argument':1.,'allow_reverse_bl':1,'clock_step_bits':f'{fb(1/30):08x}','argument_words':[f'{v:08x}'for v in inputs],
                                'result_xmm0_bits':f'{g.reg_read(UC_X86_REG_XMM0)&0xffffffff:08x}','ratio_bits':bytes(g.mem_read(OUT,4))[::-1].hex(),'stack_cleanup':g.reg_read(UC_X86_REG_ESP)-STACK})
    listings=args.output.parent/'instructions';listings.mkdir(exist_ok=True)
    code=_Code(x,'.text');starts={int(r['address'],16)for r in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    for address in (0x11d1c0,0x11d2b0,0x11d330,0x1d45a0,0x1d56f0,0x1d5830,0x1d5a60,0x1d5c70,0x1d5e90):
        f=_trace(code,address,starts);nodes=[code.at(p)for p in sorted(f.code)];p=listings/f'{address:08x}.asm';p.write_text('\n'.join(f'{n.address:08x} {n.bytes.hex()} {n.mnemonic} {n.op_str}'for n in nodes)+'\n')
    r={'oracle_sha256':sha(Path(__file__)),'xbe_sha256':PIN,'cases':cases,'executed_addresses':[f'{p:08x}'for p in sorted(executed)],'original_epsilon_4650e0_hex':x.read(0x4650e0,4).hex(),
        'boundaries':{'contact_membership_and_source_activation_executed':False,'Havok_body_mass_marker_direction_and_projected_velocity_producers_executed':False,'complete_original_lift_or_collision_result_proven':False,'supplied_scalar_input_rule_provenance':'source map rules; projected velocity/contact distance/mass and30Hz clock explicitly supplied; no original activation or contact records written'},'native_gameplay_update_applied':False}
    args.output.write_text(json.dumps(r,indent=2)+'\n');print(json.dumps({'cases':len(cases),'executed':len(executed),'report_sha256':sha(args.output)}))
if __name__=='__main__':main()
