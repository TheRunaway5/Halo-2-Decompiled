#!/usr/bin/env python3
"""Execute unchanged falling/support bodies on declared diagnostic input banks.

Authored Lockout definitions/globals are retained verbatim before explicitly
listed field overlays. Object, support, hierarchy, velocity and CPU banks are
standalone inputs, not a live admitted biped or original physics producer.
Original BB0C0, 10DF90 and BACC0 execute; no lower return/damage value is supplied.
Every other reached owner stops with its actual pending full ABI and frame.
"""
import argparse,hashlib,itertools,json,random,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_READ,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_vehicle_update_20261011 import capture,declared_json
from export_mp_physics_contact_lifecycle import segments

BASE=ROOT/'analysis/damage-shields-goal-20261011/falling-producer'
POOL,HEADERS,BODY,SUPPORT_BODY,TAGS,DEFINITION,PARENT_DEF,OPTIONS,GLOBALS,DAMAGE,MOTION,OUTPUT,SP,STOP=(
    0x1000000+n for n in (0,0x1000,0x2000,0x3000,0x4000,0x5000,0x6000,0x7000,0x8000,0x9000,0xa000,0xc000,0x1f000,0x20000))
UNIT,SUPPORT=0xabad0000,0xbeef0001
GP=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def rows(threshold):
    result=[]
    def add(entry=0xe4d30,**values):result.append(dict(entry=entry,**values))
    word=struct.unpack('<I',struct.pack('<f',-threshold))[0]
    velocities=[0x00000000,0x80000000,0x3f800000,0xbf800000,word-1,word,word+1,0xc1200000,
                0xc2c80000,0x7f800000,0xff800000,0x7fc00001]
    for definition,mode,velocity in itertools.product(range(2),(0,1,2),velocities):
        add(definition=definition,mode=mode,velocity_word=velocity)
    for option,flags134,flags10a,def_or,player in (
            (2,0,0,0,-1),(0,512,0,0,-1),(0,0,128,0,-1),(0,0,0,4,-1),
            (0,0,4,0,-1),(0,0,0,0,7)):
        add(velocity_word=0xc2c80000,option=option,flags134=flags134,flags10a=flags10a,
            definition_or=def_or,player=player)
    for parent_type,parent_flags in itertools.product((0,1,2),(0,64)):
        add(velocity_word=0xc2c80000,parent=True,parent_type=parent_type,parent_flags=parent_flags)
    for depleted,retire in itertools.product((False,True),(False,True)):
        add(velocity_word=0xc2c80000,flags134=512 if not depleted else 0,flags10a=4 if depleted else 0,
            flags4=(1<<18) if retire else 0)
    add(velocity_word=0xc1200000,buoyancy=True)
    for count,active,match,filter_byte,filter_flag in (
            (-1,True,True,0,0),(0,True,True,0,0),(1,False,True,0,0),(1,True,False,0,0),
            (1,True,True,0,0),(2,True,True,0,0),(33,True,True,0,0),
            (1,True,True,1,0),(1,True,True,1,128)):
        add(0x10df90,support_flags=2,support_count=count,support_active=active,support_match=match,
            filter_byte=filter_byte,filter_flag=filter_flag)
    for flags in (0,2,4):
        for linear in (-20.,0.,20.):
            add(velocity_word=0xc1200000,support_flags=flags,support_count=2,support_active=True,
                support_match=True,support_linear_z=linear)
    add(0x10df90,support_flags=0)
    for scale in (0.,0.5,1.,2.,-1.):
        add(0x10df90,parent=True,support_flags=2,support_count=1,support_active=True,
            support_match=True,node_scale=scale)
    for mode,action in itertools.product((0,4),(0x800001e,0xa00003e,0xd00003f)):
        add(0xe5700,option_byte=mode,action=action,logical=True)
    add(0xe5700,option_byte=4,definition_or=8)
    add(logical=True,velocity_word=0xbf800000)
    add(logical=True,option=2)
    for bsp,tag in itertools.product((-1,0,3),(0xeae90964,0xffffffff,0)):
        add(0xd7c60,bsp_word=bsp,damage_tag=tag)
    for kind in ('none','saved','player','ai','other_type','redirect'):
        add(0xd7cd0,attribution=kind)
    return result

def execute(path,latest,paired,vehicle,source,declared):
    assert sha(path)==(NEW_PIN if latest else PIN)
    image=Xbe(path);code_image=_Code(image,'.text')
    original_code=_Code(Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),'.text')
    selected={int(p,16):q for name in ('address_map','instruction_map','entries') for p,q in paired[name].items()}
    sites={int(p,16):(q if latest else int(p,16)) for p,q in paired['instruction_map'].items()}
    # Authenticate the reused complete velocity owner from its paired report.
    for name in ('address_map','instruction_map','entries'):
        for p,q in vehicle[name].items():
            p=int(p,16)
            if p in selected:assert selected[p]==q
            selected[p]=q
    listing=ROOT/'analysis/multiplayer/integration/inventory-source/instructions/000bb0c0.asm'
    assert sha(listing)=='0512e57c9277146228970a274b440d2aafc71f88fcb1f1e79a0d11acccbf9007'
    old_sites=[]
    for line in listing.read_text().splitlines():
        p,raw,*_=line.split();p=int(p,16);raw=bytes.fromhex(raw)
        actual=vehicle['instruction_map'][f'{p:08x}'] if latest else p
        x,y=original_code.at(p),code_image.at(actual)
        assert bytes(x.bytes)==raw and x.mnemonic==y.mnemonic and len(x.operands)==len(y.operands)
        for left,right in zip(x.operands,y.operands):
            assert left.type==right.type and left.size==right.size
            if left.type==1:assert x.reg_name(left.reg)==y.reg_name(right.reg)
            if left.type==2:assert (selected[left.imm&0xffffffff] if latest and left.imm!=right.imm else left.imm)==right.imm
            if left.type==3:
                assert (x.reg_name(left.mem.base),x.reg_name(left.mem.index),left.mem.scale,x.reg_name(left.mem.segment))==(
                    y.reg_name(right.mem.base),y.reg_name(right.mem.index),right.mem.scale,y.reg_name(right.mem.segment))
                if latest and left.mem.disp!=right.mem.disp:assert selected[left.mem.disp&0xffffffff]==right.mem.disp
                else:assert left.mem.disp==right.mem.disp
        old_sites.append(p)
    for p in old_sites:sites[p]=vehicle['instruction_map'][f'{p:08x}'] if latest else p
    inverse={q:p for p,q in sites.items()}
    def address(p):return selected[p] if latest else p
    cache=Path(source['cache']['path']);assert sha(cache)==source['cache']['sha256']
    with cache.open('rb') as f:
        definitions=[]
        for u in source['cache']['units']:
            t=u['tag'];f.seek(t['offset']);definitions.append(f.read(t['size']))
        f.seek(source['cache']['damage_block_file_offset']);damage=f.read(104)
    assert damage.hex()==source['cache']['raw104']
    g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000);g.mem_map(POOL,0x21000)
    for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
    state={};rng=random.Random(0xe4d30)
    def w(p,value):g.mem_write(p,struct.pack('<I',value&0xffffffff))
    def on_code(_g,p,_n,_u):
        if p not in inverse:
            state['exit_entry']=p;g.emu_stop();return
        state['instructions'].append(inverse[p])
    def on_read(_g,_a,p,n,_x,_u):
        state['events'].append(dict(kind='R',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],
            address=p,raw=bytes(g.mem_read(p,n)).hex()))
    def on_write(_g,_a,p,n,value,_u):
        state['events'].append(dict(kind='W',instruction=inverse[g.reg_read(R.UC_X86_REG_EIP)],address=p,
            before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(8*n))-1)).to_bytes(n,'little').hex()))
    g.hook_add(UC_HOOK_CODE,on_code);g.hook_add(UC_HOOK_MEM_READ,on_read);g.hook_add(UC_HOOK_MEM_WRITE,on_write)
    cases=[]
    for index,row in enumerate(declared):
        g.mem_write(POOL,rng.randbytes(0x21000))
        for p,value in ((0x4e78d0,POOL),(0x4eb114,TAGS),(0x4edf18,OPTIONS),(0x4e791c,GLOBALS),(0x517fe0,MOTION)):
            w(address(p),value)
        w(POOL+0x44,HEADERS);w(HEADERS+8,BODY);w(HEADERS+20,SUPPORT_BODY)
        w(TAGS+8,DEFINITION);w(TAGS+24,PARENT_DEF);w(GLOBALS+0x144,DAMAGE)
        g.mem_write(DAMAGE,damage);g.mem_write(DEFINITION,definitions[row.get('definition',0)])
        g.mem_write(PARENT_DEF,definitions[0]);w(PARENT_DEF+0x1ec,row.get('parent_flags',0))
        raw=bytearray(0x600);struct.pack_into('<I',raw,0,0xabcd0000)
        struct.pack_into('<I',raw,4,row.get('flags4',0));struct.pack_into('<I',raw,0x14,SUPPORT if row.get('parent') else 0xffffffff)
        struct.pack_into('<3f',raw,0x64,3.,-2.,5.);struct.pack_into('<3f',raw,0x88,1.,2.,0.)
        struct.pack_into('<I',raw,0x90,row.get('velocity_word',0xbf800000))
        struct.pack_into('<I',raw,0xb4,0xffffffff);struct.pack_into('<I',raw,0xb8,SUPPORT)
        struct.pack_into('<H',raw,0xc0,row.get('support_flags',0));raw[0x10a]=row.get('flags10a',0)
        struct.pack_into('<h',raw,0x12a,0x480);struct.pack_into('<I',raw,0x134,row.get('flags134',0))
        struct.pack_into('<I',raw,0x12c,0xffffffff);struct.pack_into('<I',raw,0x248,0xffffffff)
        struct.pack_into('<H',raw,0x138,5)
        struct.pack_into('<I',raw,0x13c,row.get('player',-1)&0xffffffff)
        struct.pack_into('<H',raw,0xc2,7);struct.pack_into('<II',raw,0xc4,0x12345678,0x23456789)
        attribution=row.get('attribution')
        if attribution=='player':struct.pack_into('<I',raw,0x13c,0xf00d0003)
        if attribution=='ai':struct.pack_into('<I',raw,0x12c,0xbead0004)
        if attribution=='other_type':raw[0xaa]=2
        if attribution=='redirect':struct.pack_into('<I',raw,0x248,SUPPORT)
        struct.pack_into('<H',raw,0x348,0x800 if row.get('buoyancy') else 0)
        raw[0x3dc]=row.get('mode',1);struct.pack_into('<I',raw,0x4fc,row.get('action',0x800001e))
        g.mem_write(BODY,bytes(raw));parent=bytearray(raw)
        struct.pack_into('<I',parent,0,0xabcd0001);struct.pack_into('<I',parent,0x14,0xffffffff)
        parent[0xaa]=row.get('parent_type',0);g.mem_write(SUPPORT_BODY,bytes(parent))
        w(SUPPORT_BODY+0x248,0xffffffff)
        if 'node_scale' in row:
            g.mem_write(SUPPORT_BODY+0xc0,struct.pack('<H',row['support_flags']))
            g.mem_write(SUPPORT_BODY+0x116,struct.pack('<h',0x500))
            g.mem_write(SUPPORT_BODY+0x500,struct.pack('<13f',row['node_scale'],1.,0.,0.,0.,1.,0.,0.,0.,1.,2.,3.,4.))
        flag=struct.unpack('<I',bytes(g.mem_read(DEFINITION+0x1f0,4)))[0]
        w(DEFINITION+0x1f0,flag|row.get('definition_or',0));g.mem_write(PARENT_DEF+2,bytes((row.get('filter_flag',0),)))
        w(OPTIONS+8,row.get('option',0));g.mem_write(OPTIONS+12,bytes((row.get('option_byte',0),)))
        count=row.get('support_count',0)
        for n in range(max(count,0)):
            w(MOTION+0x1d00+4*n,SUPPORT if row.get('support_match',True) else SUPPORT^0x10000)
            record=MOTION+0xe8*n
            g.mem_write(record+0xc,struct.pack('<3f',0.,1.,2.))
            g.mem_write(record+0x24,struct.pack('<3f',4.,-3.,row.get('support_linear_z',-5.)+n))
            g.mem_write(record+0x3c,struct.pack('<3f',0.25,-0.5,0.75))
            g.mem_write(record+0xe5,bytes((int(row.get('support_active',True)),)))
        w(MOTION+0x1d80,count)
        w(SP,STOP)
        if row['entry']==0xe4d30:args=[]
        elif row['entry']==0xe5700:args=[UNIT,OUTPUT]
        elif row['entry']==0x10df90:args=[UNIT,OUTPUT,row.get('filter_byte',0)]
        elif row['entry']==0xd7c60:
            args=[row['damage_tag']];g.mem_write(address(0x46fb64),struct.pack('<H',row['bsp_word']&0xffff))
        else:assert row['entry']==0xd7cd0;args=[0xffffffff if attribution=='none' else UNIT,OUTPUT]
        for n,value in enumerate(args):w(SP+4+4*n,value)
        for n,reg in enumerate(GP):g.reg_write(reg,0 if row.get('logical') else rng.getrandbits(32))
        g.reg_write(R.UC_X86_REG_ESP,SP);g.reg_write(R.UC_X86_REG_EAX,UNIT if row['entry']==0xe4d30 else 0 if row.get('logical') else rng.getrandbits(32))
        if row['entry']==0xd7c60:g.reg_write(R.UC_X86_REG_EDX,OUTPUT)
        for n in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{n}'),0 if row.get('logical') else rng.getrandbits(128))
        g.reg_write(R.UC_X86_REG_EFLAGS,2)
        g.reg_write(R.UC_X86_REG_FPCW,0x37f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
        g.reg_write(R.UC_X86_REG_FPSW,0 if row.get('logical') else 3<<11)
        tag=0xffff
        for n in range(0 if row.get('logical') else 3):
            physical=(3+n)&7;tag&=~(3<<(physical*2));g.reg_write(getattr(R,f'UC_X86_REG_FP{physical}'),(0x8000000000000000,0x3fff+n))
        g.reg_write(R.UC_X86_REG_FPTAG,tag)
        retained_regions=[(BODY,0x600)]
        if row['entry']==0xd7c60:retained_regions.append((OUTPUT-4,0x90))
        if row['entry']==0xd7cd0:retained_regions.append((OUTPUT,12))
        diagnostic_before={p:bytes(g.mem_read(p,n)) for p,n in retained_regions}
        state.update(events=[],instructions=[],exit_entry=None);entry_abi=capture(g)
        try:g.emu_start(address(row['entry']),0,count=10000)
        except Exception as error:raise RuntimeError(dict(case=index,input=row,eip=hex(g.reg_read(R.UC_X86_REG_EIP)))) from error
        assert state['exit_entry'] is not None,(index,row,state['instructions'][-12:])
        first={};current={}
        for event in state['events']:
            p=event['address'];raw=bytes.fromhex(event['raw'] if event['kind']=='R' else event['before'])
            for n,value in enumerate(raw):
                if p+n not in current:first[p+n]=current[p+n]=value
                assert current[p+n]==value
            if event['kind']=='W':
                for n,value in enumerate(bytes.fromhex(event['written'])):current[p+n]=value
        for p,raw in diagnostic_before.items():
            after=bytes(g.mem_read(p,len(raw)))
            for n,value in enumerate(raw):
                first.setdefault(p+n,value);assert first[p+n]==value
                current.setdefault(p+n,after[n]);assert current[p+n]==after[n]
        complete=state['exit_entry']==STOP
        pending=None
        if not complete:
            last=state['instructions'][-1];instruction=code_image.at(sites[last])
            assert instruction.mnemonic=='call'
            pending=dict(instruction=last,target=state['exit_entry'],tail=False,
                frame_esp=g.reg_read(R.UC_X86_REG_ESP),stack32=bytes(g.mem_read(g.reg_read(R.UC_X86_REG_ESP),32)).hex())
            # Retain the diagnostic call-frame snapshot, including untouched
            # stack cells. These are not additional original instruction reads.
            for n,value in enumerate(bytes.fromhex(pending['stack32'])):
                at=pending['frame_esp']+n
                if at not in current:first[at]=current[at]=value
                assert current[at]==value
        cases.append(dict(id=index,entry=row['entry'],entry_abi=entry_abi,exit=capture(g),exit_entry=state['exit_entry'],
            return_address=STOP,complete=complete,pending=pending,logical=row.get('logical',False),arguments=args,
            initial_banks=[dict(address=p,hex=b.hex()) for p,b in segments(first)],
            final_banks=[dict(address=p,hex=b.hex()) for p,b in segments(current)],instructions=state['instructions'],
            writes=[e for e in state['events'] if e['kind']=='W'],estimates=[],timestamps=[],declared_input=declared_json(row)))
    return cases

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
    authority=BASE/'manifest.json';assert sha(authority)=='ff7e9a352ac5c4ee3e6fcd58f99e2288dcc5cb450209e8c124e1d9159dee7c83'
    normalized=BASE/'generated/normalized-correspondence.json';paired=json.loads(normalized.read_text())
    vehicle_path=ROOT/'analysis/multiplayer/sandbox-source/vehicle-update-20261011/correspondence-verified/report.json'
    assert sha(vehicle_path)=='4c6d1a4b5733144af7109feaa34d0f06a69b47f1eeabb6123db866734c6f488f'
    source=json.loads(authority.read_text());vehicle=json.loads(vehicle_path.read_text())
    declared=rows(source['cache']['fields']['5c']['binary32']);a.output.mkdir(parents=True,exist_ok=False);stats={}
    for name,latest,path in (('disc',False,ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'),('u5',True,NEW_DEFAULT)):
        cases=execute(path,latest,paired,vehicle,source,declared)
        fixture=dict(schema=1,image='1.5' if latest else '1.1',cases=cases,tool_sha256=sha(__file__),
            correspondence_sha256=sha(normalized),authority_sha256=sha(authority),instruction_patches=[],
            native_expected_data_used=False,live_falling_damage_proof=False)
        p=a.output/f'{name}.json';p.write_text(json.dumps(fixture,sort_keys=True,separators=(',',':'),allow_nan=False)+'\n')
        stats[name]=dict(sha256=sha(p),cases=len(cases),complete=sum(c['complete'] for c in cases),
            instructions=sum(len(c['instructions']) for c in cases),writes=sum(len(c['writes']) for c in cases),
            covered_instructions=len({p for c in cases for p in c['instructions']}),
            stop_entries=sorted({c['exit_entry'] for c in cases if not c['complete']}))
    report=dict(tool_sha256=sha(__file__),authority_sha256=sha(authority),correspondence_sha256=sha(normalized),
        vehicle_correspondence_sha256=sha(vehicle_path),capture_helper_sha256=sha(ROOT/'tools/oracle_vehicle_update_20261011.py'),
        segments_helper_sha256=sha(ROOT/'tools/export_mp_physics_contact_lifecycle.py'),fixtures=stats,boundary=__doc__,
        instruction_patches=[],native_expected_data_used=False,live_falling_damage_proof=False)
    (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
