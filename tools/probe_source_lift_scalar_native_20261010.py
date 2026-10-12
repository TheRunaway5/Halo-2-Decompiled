#!/usr/bin/env python3
"""Run unchanged lift scalar instructions; explicit kernel inputs, not contacts."""
import argparse, hashlib, itertools, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import (UC_X86_REG_ESP, UC_X86_REG_EBX, UC_X86_REG_EIP,
    UC_X86_REG_XMM0, UC_X86_REG_FPCW, UC_X86_REG_MXCSR, UC_X86_REG_FPSW, UC_X86_REG_FPTAG)
from oracle_smg_firing_1_1 import Xbe, ROOT, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

TIME, OUT, STACK, STOP = 0x1000000, 0x1001000, 0x200d000, 0x3000000
OLD = ROOT/'analysis/multiplayer/sandbox-source/lift-scalar/measured/report.json'
OLD_PIN = 'df8b9921a0ab60bf8ee7bfb1168b365ef8cb4123cbd0984e7111d952d02e89b8'
def digest(p):
    with Path(p).open('rb') as f: return hashlib.file_digest(f, 'sha256').hexdigest()
def bits(f): return struct.unpack('<I', struct.pack('<f', f))[0]
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);args=ap.parse_args()
    args.output.mkdir(parents=True,exist_ok=False)
    assert digest(OLD)==OLD_PIN
    prior=json.loads(OLD.read_text()); inputs=[]
    old_image=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
    assert digest(old_image.path)==PIN
    epsilon=struct.unpack('<f',old_image.read(0x4650e0,4))[0]
    for row in prior['cases']:
        a=[int(v,16) for v in row['argument_words']]
        inputs.append([row['allow_reverse_bl'],a[0],a[8],*a[1:8],int(row['clock_step_bits'],16)])
    # These are declared helper controls, not measured world/body/phantom inputs.
    for rev,scale,mass,force,accel,velocity,target,limit,dt in itertools.product(
        (0,1),(0,1),(-1.,0.,epsilon,2.),
        (0,1),(-10.,-0.,0.,epsilon,10.),(-20.,-1.,-0.,0.,1.,20.),
        (-1.,0.,.01,10.),(0.,epsilon,10.,-10.),(0.,1/30)):
        inputs.append([rev,scale,force,bits(mass),bits(target),bits(accel),bits(velocity),
            bits(limit),bits(.01),bits(0.),bits(dt)])
    specs=[('1.1',ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN,
        0x1d5c70,0x518444,(0x4650e0,0x465030,0x46502c,0x447020,0x452044)),
        ('u5',NEW_DEFAULT,NEW_PIN,0x1da530,0x51df54,(0x46a180,0x46a0d0,0x46a0cc,0x44aac8,0x45695c))]
    fixture=['H2_LIFT_SCALAR_V1','2'];banks=[];reference=None
    for version,path,pin,entry,clock,constants in specs:
        assert digest(path)==pin
        x=Xbe(path);g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
        for s in x.sections:g.mem_write(s.va,x.section_bytes(s))
        for p in (TIME,STACK&~0xffff,STOP):g.mem_map(p,0x10000)
        def w(p,v):g.mem_write(p,struct.pack('<I',v&0xffffffff))
        w(clock,TIME);constant_words=[struct.unpack('<I',x.read(p,4))[0] for p in constants]
        fixture.extend((' '.join(f'{v:08x}'for v in constant_words),str(len(inputs))))
        outputs=[]
        for n,a in enumerate(inputs):
            # Arguments: scale,mass,target,acceleration,current,limit,position,exponent,force,ratio.
            arguments=[a[1],*a[3:10],a[2],OUT]
            g.mem_write(STACK,struct.pack('<11I',STOP,*arguments));w(TIME+4,a[10]);w(OUT,0xcdcdcdcd)
            for r,v in ((UC_X86_REG_ESP,STACK),(UC_X86_REG_EBX,a[0]),(UC_X86_REG_FPCW,0x027f),
                (UC_X86_REG_MXCSR,0x1f80),(UC_X86_REG_FPSW,0),(UC_X86_REG_FPTAG,0xffff)):
                g.reg_write(r,v)
            g.emu_start(entry,STOP,count=5000)
            assert g.reg_read(UC_X86_REG_EIP)==STOP and g.reg_read(UC_X86_REG_ESP)==STACK+44
            result=g.reg_read(UC_X86_REG_XMM0)&0xffffffff;ratio=struct.unpack('<I',g.mem_read(OUT,4))[0]
            if n<len(prior['cases']):
                old=prior['cases'][n];assert result==int(old['result_xmm0_bits'],16)and ratio==int(old['ratio_bits'],16)
            outputs.append((result,ratio));fixture.append(' '.join(f'{v:08x}'for v in (*a,result,ratio)))
        if reference is None:reference=outputs
        else:assert outputs==reference
        banks.append({'version':version,'image_sha256':pin,'entry':f'{entry:08x}',
            'constant_addresses':[f'{v:08x}'for v in constants],
            'constant_words':[f'{v:08x}'for v in constant_words],'case_count':len(outputs),
            'output_pairs_sha256':hashlib.sha256(b''.join(struct.pack('<II',*v)for v in outputs)).hexdigest()})
    target=args.output/'fixture.txt';target.write_text('\n'.join(fixture)+'\n')
    report={'tool_sha256':digest(__file__),'prior_fixture_sha256':OLD_PIN,'fixture_sha256':digest(target),
        'banks':banks,'authored_rule_cases_per_image':len(prior['cases']),
        'explicit_additional_scalar_controls_per_image':len(inputs)-len(prior['cases']),
        'image_results_equal':True,'original_instructions_unpatched':True,
        'contact_body_activation_world_force_executed':False,'positive_exponent_owner_executed':False,
        'original_clock_producer_executed':False,'host_fp_control':'x87 precision53, nearest even; MXCSR1f80'}
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'fixture':str(target),'cases':len(inputs)*2,'fixture_sha256':digest(target)}),flush=True)
if __name__=='__main__':main()
