#!/usr/bin/env python3
"""Original motion/island/world advancement, with no supplied solver results.

Declared raw-input controls are CPU differential tests, not live world creation.
RSQRTSS observations belong only to the emulator comparison, not production.
"""
import argparse, csv, hashlib, json, random, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

BASE, SIZE, SP, STOP = 0x1000000, 0x5000, 0x200f000, 0x3000000
WORLD, ISLAND, BODY, MOTION = BASE, BASE+0x400, BASE+0x800, BASE+0x1000
STEP, POSITION, QUATERNION, OUTPUT = BASE+0x3000, BASE+0x3100, BASE+0x3120, BASE+0x3140
ROOTS = (0x317650, 0x316200, 0x316440, 0x31cfe0, 0x31d0f0, 0x321760, 0x321600, 0x321530)
LISTINGS = ROOTS + (0x31ed30, 0x31ec90, 0x31ee50, 0x31edc0, 0x31f8b0, 0x322690, 0x321320)

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--image',choices=('1.1','1.5'),default='1.1')
    ap.add_argument('--correspondence',type=Path)
    a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=False)
    latest=a.image=='1.5';mapping={};site_map={};operands={}
    if latest:
        assert a.correspondence
        correspondence=json.loads(a.correspondence.read_text())
        assert correspondence['disc_1_1_sha256']==PIN and correspondence['later_owned_xbe_sha256']==NEW_PIN
        assert correspondence['complete_bodies']==len(correspondence['entries'])==17
        mapping={int(k,16):v for k,v in correspondence['entries'].items()}
        site_map={v:int(k,16)for k,v in correspondence['instruction_map'].items()}
        operands={int(k,16):v['later'] for k,v in correspondence['operand_map'].items()}
    selected=lambda p:mapping.get(p,p)
    operand=lambda p:operands.get(p,p)
    path=NEW_DEFAULT if latest else ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(path)==(NEW_PIN if latest else PIN)
    image=Xbe(path);code=_Code(image,'.text')
    starts={int(q['address'],16) for q in csv.DictReader((ROOT/'analysis/map-pack-update/functions.tsv').open(),delimiter='\t')}
    listings=[];directory=a.output/'instructions';directory.mkdir()
    for canonical in LISTINGS:
        entry=selected(canonical)
        ins=[code.at(p) for p in sorted(_trace(code,entry,(set(mapping.values()) if latest else starts)|set(LISTINGS)).code)]
        p=directory/f'{entry:08x}.asm'
        p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
        listings.append(dict(entry=entry,file=p.name,sha256=sha(p),instructions=len(ins)))
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0,0x810000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    g.mem_map(BASE,0x10000);g.mem_map(0x2000000,0x10000);g.mem_map(STOP,0x1000)
    w=lambda p,x,n=4:g.mem_write(p,(int(x)&((1<<(n*8))-1)).to_bytes(n,'little'))
    f=lambda p,x:g.mem_write(p,struct.pack('<f',x))
    vec=lambda p,x:g.mem_write(p,struct.pack('<4f',*x))
    read=lambda p:int.from_bytes(g.mem_read(p,4),'little')
    cases=[];writes=[];estimates=[];active=False;boundary=None;stops={}
    def call(entry,receiver,args):
        w(SP,STOP)
        for i,x in enumerate(args):w(SP+4+i*4,x)
        for register,value in [(R.UC_X86_REG_ESP,SP),(R.UC_X86_REG_ECX,receiver),
            (R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80)]:g.reg_write(register,value)
        g.emu_start(selected(entry),STOP,count=100000)
        assert boundary or g.reg_read(R.UC_X86_REG_EIP)==STOP,hex(g.reg_read(R.UC_X86_REG_EIP))
    def on_write(_g,_access,p,n,x,_data):
        if active and BASE<=p and p+n<=BASE+SIZE:
            site=g.reg_read(R.UC_X86_REG_EIP)
            writes.append(dict(instruction=site_map[site] if latest else site,address=p,
                before=bytes(g.mem_read(p,n)).hex(),written=(x&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
    pending_estimate=None
    def on_code(_g,p,_size,_data):
        nonlocal boundary,pending_estimate
        if not active:return
        canonical=site_map.get(p,p)
        if canonical==0x1dc782:
            pending_estimate=g.reg_read(R.UC_X86_REG_XMM0)&0xffffffff
        elif canonical==0x1dc786:
            estimates.append([pending_estimate,g.reg_read(R.UC_X86_REG_XMM1)&0xffffffff]);pending_estimate=None
        if p in stops:
            esp=g.reg_read(R.UC_X86_REG_ESP);n,receiver=stops[p]
            boundary=dict(entry=p,ecx=g.reg_read(R.UC_X86_REG_ECX) if receiver else None,
                entry_esp=esp,arguments=[read(esp+4+i*4) for i in range(n)])
            g.emu_stop()
    g.hook_add(UC_HOOK_MEM_WRITE,on_write);g.hook_add(UC_HOOK_CODE,on_code)
    def setup(seed=0):
        nonlocal active
        active=False;g.mem_write(BASE,bytes(SIZE));g.mem_write(0x2000000,bytes(0x10000))
        w(operand(0x487674),0);w(operand(0x487678),0)
        vec(POSITION,(1.5,-2,4,0));vec(QUATERNION,(0,0,0,1))
        # Actual original dynamic motion constructor; no constructed pose supplied.
        call(0x321320,MOTION,[POSITION,QUATERNION])
        w(WORLD+0xcc,BASE+0x3200);w(WORLD+0x17c,8);f(WORLD+0x180,.125)
        f(WORLD+0x30,.2);f(WORLD+0x34,10)
        f(STEP,1/30);f(STEP+4,30);f(WORLD+0x150,1/30);f(WORLD+0x154,30)
        vec(WORLD+0xe0,(0,0,-4.1712594,0));w(ISLAND+0x20,WORLD)
        w(ISLAND+0x3c,BASE+0x3400);w(BASE+0x3400,BODY);w(ISLAND+0x40,1);w(BODY+0x3c,MOTION)
        w(WORLD+8,BASE+0x3440);w(BASE+0x3440,ISLAND)
        rnd=random.Random(seed)
        for off in (0x40,0x50,0xf0,0x100):vec(MOTION+off,tuple(rnd.uniform(-2,2) for _ in range(3))+(0,))
        f(MOTION+0x2c,.25);vec(MOTION+0xe0,(.5,.75,.875,.25))
    def execute(name,entry,receiver,args,count=1):
        nonlocal active,writes,estimates,boundary
        before=bytes(g.mem_read(BASE,SIZE));writes=[];estimates=[];boundary=None;active=True
        for _ in range(count):
            call(entry,receiver,args)
            if boundary:break
        active=False
        # Native owners consume immutable executable operands through source storage.
        constants=[]
        for p,n in [(0x465030,4),(0x465048,4),(0x465044,4),(0x46502c,4),(0x41b760,16),
            (0x41b8f0,36),(0x487674,8)]:
            if latest and p in (0x41b8f0,0x487674):continue
            constants.append([operand(p),bytes(g.mem_read(operand(p),n)).hex()])
        cases.append(dict(name=name,image=a.image,entry=entry,receiver=receiver,args=args,entry_esp=SP,
            iterations=count,fp=[0x27f,0x1f80],before=before.hex(),after=bytes(g.mem_read(BASE,SIZE)).hex(),
            writes=writes,estimates=estimates,constants=constants,boundary=boundary))
    # Force/damping/torque controls execute both branches and clamp boundaries.
    for seed in range(24):
        setup(seed);f(STEP,(0,1/30,1/60,.5)[seed%4]);f(MOTION+0x14,(0,.1,100,-1)[seed%4]);f(MOTION+0x18,(.2,100,0,-1)[seed%4])
        f(MOTION+0x10c,(0,1,-1)[seed%3])
        execute(f'forces_{seed}',0x321760,MOTION,[STEP,WORLD+0xe0])
    for seed in range(16):
        setup(seed);f(STEP,(0,1/30,1/144,.125)[seed%4])
        execute(f'integrate_{seed}',0x321600,MOTION,[STEP],count=4)
    for seed in range(8):
        setup(seed);vec(BASE+0x3160,(.1,-.2,.3,0))
        execute(f'angular_impulse_{seed}',0x321530,MOTION,[BASE+0x3160])
    if latest:
        # Only motion owners have production TU1.5 bindings in this extension.
        p=a.output/'fixture.json'
        result=dict(schema=1,original_sha256=NEW_PIN,tool_sha256=sha(__file__),base=BASE,size=SIZE,cases=cases,
            instruction_listings=listings,source_instruction_patches=[],supplied_solver_or_contact_results=[],
            correspondence_sha256=sha(a.correspondence),native_expected_data_used=False,scope=__doc__)
        p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
        report=dict(cases=len(cases),ordered_persistent_writes=sum(len(c['writes'])for c in cases),fixture_sha256=sha(p))
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report));return
    for seed in range(12):
        setup(seed);f(WORLD+0x150,(1/30,1/60,1/144)[seed%3]);f(MOTION+0x10c,1)
        execute(f'island_{seed}',0x31cfe0,ISLAND,[WORLD+0x140],count=4)
    for count in (2,3):
        setup(count);w(ISLAND+0x40,count);w(ISLAND+0x3c,BASE+0x3600)
        for i in range(count):
            body=BODY+i*0x100;motion=MOTION+i*0x200
            if i:call(0x321320,motion,[POSITION,QUATERNION])
            w(BASE+0x3600+i*4,body);w(body+0x3c,motion)
            vec(motion+0x40,(i+1,.5*i,-.25*i,0));vec(motion+0x50,(.1*i,-.2*i,.3*i,0))
        execute(f'island_{count}_bodies',0x31cfe0,ISLAND,[WORLD+0x140],count=4)
    for count in (2,3):
        setup(count);w(WORLD+12,count);w(WORLD+8,BASE+0x3700)
        for i in range(count):
            island=ISLAND+i*0x100;body=BODY+i*0x100;motion=MOTION+i*0x200;bank=BASE+0x3600+i*0x20
            if i:call(0x321320,motion,[POSITION,QUATERNION])
            w(BASE+0x3700+i*4,island);w(island+0x20,WORLD);w(island+0x3c,bank);w(island+0x40,1)
            w(bank,body);w(body+0x3c,motion);vec(motion+0x40,(i+1,-.5*i,.25*i,0))
        execute(f'world_{count}_islands',0x316440,WORLD,[STEP],count=4)
    # Declared empty-world schedules, no contacts/solver callbacks supplied.
    for seed in range(8):
        setup(seed);w(ISLAND+0x40,0);w(WORLD+12,0)
        w(WORLD+0x94,3);w(WORLD+0x90,BASE+0x3480)
        w(WORLD+0x88,3);w(WORLD+0x84,BASE+0x34a0)
        f(STEP,(0,1/30,1/144,1)[seed%4]);f(STEP+4,(0,30,144,1)[seed%4])
        execute(f'world_{seed}',0x317650,WORLD,[STEP])
    for seed in range(8):
        setup(seed);w(WORLD+12,1);f(MOTION+0x10c,1)
        # Integration owns no collision phase and does not claim one happened.
        execute(f'world_integration_{seed}',0x316440,WORLD,[STEP],count=4)
    # Actual controller semantics, including signed byte counter and strict clocks.
    for seed in range(24):
        setup(seed);w(ISLAND+0x40,0 if seed%2 else 1)
        f(ISLAND+0x34,(0,.2,.3)[seed%3]);f(ISLAND+0x38,(0,10,11)[seed%3])
        w(ISLAND+0x2c,(0,4,5,0x80)[seed%4],1)
        execute(f'deactivation_{seed}',0x31d0f0,ISLAND,[OUTPUT,STEP])
    # Required boundaries are stopped before executing them; no return supplied.
    for entry,field,owner,n,receiver in [(0x31cfe0,ISLAND+0x5c,0x322690,9,False),
        (0x316440,WORLD+0x23c,0x314c70,0,True),
        (0x316200,WORLD+0xf4,0x316180,0,True),
        (0x316440,WORLD+0x10c,0x313310,0,True)]:
        setup();w(field,1,1 if field==WORLD+0x23c else 4);stops={owner:(n,receiver)}
        execute(f'required_{owner:x}',entry,ISLAND if entry==0x31cfe0 else WORLD,[WORLD+0x140 if entry==0x31cfe0 else STEP]);stops={}
    # Declared unbound virtual destinations observe the original caller ABI;
    # no instruction or result is supplied for the destination itself.
    marker=STOP+0x100;receiver=BASE+0x3800;vtable=BASE+0x3900
    for kind in ('world_collision','world_integration','island_integration','action','short_deactivator','long_deactivator'):
        setup();w(receiver,vtable);w(vtable+4,marker);w(vtable+20,marker);w(vtable+24,marker)
        stops={marker:(1 if kind=='action' else 2,True)}
        if kind=='world_collision':
            w(WORLD+0x90,BASE+0x3480);w(WORLD+0x94,1);w(BASE+0x3480,receiver)
            execute('unbound_'+kind,0x316200,WORLD,[STEP])
        elif kind in ('world_integration','island_integration'):
            offset=0x84 if kind=='world_integration' else 0x9c
            w(WORLD+offset,BASE+0x3480);w(WORLD+offset+4,1);w(BASE+0x3480,receiver)
            if kind=='island_integration':w(WORLD+12,1)
            execute('unbound_'+kind,0x316440,WORLD,[STEP])
        elif kind=='action':
            w(ISLAND+0x4c,BASE+0x3480);w(ISLAND+0x50,1);w(BASE+0x3480,receiver)
            execute('unbound_'+kind,0x31cfe0,ISLAND,[WORLD+0x140])
        else:
            w(BODY+0x54,receiver)
            f(ISLAND+0x34,.3 if kind=='short_deactivator' else 0)
            f(ISLAND+0x38,0 if kind=='short_deactivator' else 11)
            execute('unbound_'+kind,0x31d0f0,ISLAND,[OUTPUT,STEP])
        stops={}
    result=dict(schema=1,original_sha256=PIN,tool_sha256=sha(__file__),base=BASE,size=SIZE,cases=cases,
        instruction_listings=listings,source_instruction_patches=[],supplied_solver_or_contact_results=[],
        native_expected_data_used=False,scope=__doc__)
    p=a.output/'fixture.json';p.write_text(json.dumps(result,sort_keys=True,separators=(',',':'))+'\n')
    report=dict(cases=len(cases),complete=sum(not c['boundary'] for c in cases),
        required_stops=sum(bool(c['boundary']) for c in cases),
        ordered_persistent_writes=sum(len(c['writes']) for c in cases),
        reciprocal_estimates=sum(len(c['estimates']) for c in cases),fixture_sha256=sha(p))
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
