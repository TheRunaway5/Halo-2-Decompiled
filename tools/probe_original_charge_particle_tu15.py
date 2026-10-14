#!/usr/bin/env python3
"""Execute an untouched TU1.5 particle frame over a captured charge-effect bank.

The source world and charge callback produced the input records. The probe
supplies only a declared caller ABI and frame delta, and supplies no function
returns or game/platform service policies. This is original consumer evidence,
not a native implementation, a boot or a complete rendered weapon sequence.
"""
import argparse,hashlib,json,shutil,struct
from collections import Counter
from pathlib import Path
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code
from oracle_charged_effect_lifetime_bank_tu15 import regions,segment
from validate_charge_effect_bank_tu15 import validate
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
REGISTERS=[getattr(R,'UC_X86_REG_'+s) for s in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]

def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--case',type=Path,required=True)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--entry',type=lambda x:int(x,0),choices=(0x179df0,0x13a9b0,0x179cd0,0x22c440,0x22c480,0x17ead0,0x180520,0xd72f0,0x18ee50,0x219b90,0x1378e0,0x3f3e0,0x1770c0,0x1772c0,0x1e0f0,0x1373f0,0x136ea0,0x176b30,0x219080,0x2190c0,0x175f90,0x380fa0,0x108db0,0x1030f0,0x1e7e40,0x1e9000,0x1e8410,0x1d0670,0x1d03a0),default=0x179df0)
    ap.add_argument('--phase',choices=('before','after'),default='after')
    ap.add_argument('--previous',type=Path,help='Continue from an authenticated original hardware result')
    ap.add_argument('--lifetime-frame',type=int,choices=range(1,129),metavar='1..128',help='Replay a captured source-world particle API frame')
    ap.add_argument('--native-call',choices=['visibility-consumer','transparent-init','view-reset','view-admission','world-draw',*(f'active-frame{n}' for n in range(1,17))],help='Replay an authenticated native visibility or active-particle input bank')
    ap.add_argument('--queued-callback',type=int,help='Invoke this actual transparent-command callback from a preceding authenticated hardware bank')
    a=ap.parse_args();case=a.case.resolve();out=a.output.resolve();assert not out.exists();out.mkdir(parents=True)
    metadata=json.loads((case/'native.json').read_text())
    actor_constructor=a.entry in (0x1e7e40,0x1e9000,0x1e8410,0x1d0670,0x1d03a0)
    if actor_constructor:
        assert not a.native_call and not a.lifetime_frame
        if a.entry==0x1d0670 and metadata['entry']==0x1d0670:
            from validate_hunter_ai_frame_bank_tu15 import validate as validate_actor
            assert a.phase=='before' and not a.previous
        else:
            from validate_actor_constructor_bank_tu15 import validate as validate_actor
            if a.entry==0x1d0670:assert a.previous and a.phase=='after' and metadata['entry']==0x1e8410
            else:assert a.phase=='before' and not a.previous
        context=validate_actor(case,metadata,list(regions(case/'before.bin')))
        origin_hardware_sha256=None # This probe establishes the first original actor comparison.
    else:
        context=validate(case,metadata,list(regions(case/'before.bin')))
        hardware=json.loads((case/'hardware-verification/report.json').read_text())
        assert hardware['passed'] and not hardware['source_instruction_patches'] and not hardware['supplied_game_services']
        origin_hardware_sha256=sha(case/'hardware-verification/report.json')
    build=json.loads((case/'build.json').read_text());origin=case/(a.phase+'.bin');previous_evidence=None
    api_frame=None;assert sum(bool(v) for v in (a.previous,a.lifetime_frame,a.native_call))<=1
    if a.native_call:
        assert a.entry in ({'visibility-consumer':(0x3f3e0,),'transparent-init':(0x1e0f0,),'view-reset':(0x1373f0,),'view-admission':(0x136ea0,),'world-draw':(0x1770c0,)}.get(a.native_call,(0x179df0,0x1770c0,0x1772c0)))
        native_root=Path(build['command'][3]);name=a.native_call+'/before.bin';origin=native_root/name
        assert sha(origin)==build['active_banks_sha256'][name]
        after_name=a.native_call+'/after.bin';assert sha(native_root/after_name)==build['active_banks_sha256'][after_name]
        api_frame=dict(native_root=str(native_root),native_call=a.native_call,native_after=str(native_root/after_name),native_after_sha256=build['active_banks_sha256'][after_name],native_build_sha256=sha(case/'build.json'))
    elif a.lifetime_frame:
        assert a.entry==0x179df0
        native_root=Path(build['command'][3]);name=f'particle-frame{a.lifetime_frame}/before.bin';origin=native_root/name
        assert sha(origin)==build['lifetime_banks_sha256'][name]
        after_name=f'particle-frame{a.lifetime_frame}/after.bin';assert sha(native_root/after_name)==build['lifetime_banks_sha256'][after_name]
        api_frame=dict(native_root=str(native_root),frame=a.lifetime_frame,native_after=str(native_root/after_name),native_after_sha256=build['lifetime_banks_sha256'][after_name],native_build_sha256=sha(case/'build.json'))
    elif a.previous:
        previous=a.previous.resolve();prior=json.loads((previous/'report.json').read_text());h=json.loads((previous/'hardware/report.json').read_text())
        assert prior['completed'] and prior['source_context']==context and prior['origin_case']==str(case)
        assert h['completed'] and h['emulator_retained_state_matches_hardware'] and not h['source_instruction_patches'] and not h['supplied_game_services']
        assert h['original_report_sha256']==sha(previous/'report.json') and h['original_xbe_sha256']==NEW_PIN
        origin=previous/'hardware/hardware-after.bin';assert sha(origin)==h['hardware_after_sha256']
        previous_evidence=dict(report=str(previous/'report.json'),report_sha256=sha(previous/'report.json'),hardware_report_sha256=sha(previous/'hardware/report.json'))
    else:assert sha(origin)==build['files_sha256'][origin.name]
    assert sha(NEW_DEFAULT)==NEW_PIN;image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    # D3D is owned original executable code too. Admit its unchanged bytes;
    # genuine kernel/device imports still stop without supplied policies.
    owners=[code,_Code(image,'D3D')]
    data=list(regions(origin));g=Uc(UC_ARCH_X86,UC_MODE_32);mapped=set()
    def map_range(p,n):
        pages=[q for q in range(p&~4095,(p+n+4095)&~4095,4096) if q not in mapped]
        i=0
        while i<len(pages):
            j=i+1
            while j<len(pages) and pages[j]==pages[j-1]+4096:j+=1
            g.mem_map(pages[i],(j-i)*4096);i=j
        mapped.update(pages)
    for p,b in data:map_range(p,len(b));g.mem_write(p,b)
    for owner in owners:assert bytes(g.mem_read(owner.lo,len(owner.bytes)))==owner.bytes
    for p in (0x70000000,0x60300000):map_range(p,4096)
    g.mem_write(0x70000008,segment(0,0x9b)+segment(0,0x93)+segment(0x60200000,0x93))
    g.reg_write(R.UC_X86_REG_GDTR,(0,0x70000000,0xfff,0))
    for reg,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(reg,value)
    def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
    clock=u(0x51df54);delta_bits=u(clock+4);assert struct.unpack('<f',struct.pack('<I',delta_bits))[0]>0
    gp=[0,0,0,0,0x600f0000,0,0,0]
    raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    for reg,value in zip(REGISTERS,gp):g.reg_write(reg,value)
    for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
    for reg,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff),(R.UC_X86_REG_MXCSR,0x1f80)):g.reg_write(reg,value)
    arguments=[delta_bits] if a.entry in (0x179df0,0x13a9b0,0x17ead0,0xd72f0,0x18ee50,0x1378e0) else []
    if a.entry==0x1373f0:gp[2]=0x556e18;g.reg_write(REGISTERS[2],gp[2]);raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x108db0:
        assert a.phase=='before' and not a.previous and metadata['entry']==0x108db0
        gp[1]=metadata['weapon'];g.reg_write(REGISTERS[1],gp[1]);arguments=[0]
        raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x1030f0:
        assert a.phase=='before' and not a.previous and metadata['entry']==0x1030f0
        arguments=[metadata['weapon']]
    if actor_constructor:
        if a.entry==0x1e7e40:arguments=[metadata['character']]
        elif a.entry==0x1e8410:
            gp[0]=metadata['declared_squad'];gp[2]=metadata['character'];arguments=[metadata['unit'],0]
            g.reg_write(REGISTERS[0],gp[0]);g.reg_write(REGISTERS[2],gp[2])
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
        elif a.entry==0x1e9000:
            gp[7]=metadata['actor'];g.reg_write(REGISTERS[7],gp[7]);arguments=[metadata['unit']]
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
        elif a.entry==0x1d03a0:
            gp[6]=metadata['player'];g.reg_write(REGISTERS[6],gp[6])
            raw_input=struct.pack('<8I',*gp)+bytes(128)+struct.pack('<II',2,0x60200000)
    if a.entry==0x136ea0:arguments=[0x556e18]
    portal_caller=None
    if a.entry==0x175f90:
        assert a.previous and u(0x505e7c)!=0
        visibility=json.loads((case/'visibility.json').read_text())
        assert sha(case/'visibility.json')==build['files_sha256']['visibility.json']
        collector=u(0x556e18);count=u(collector,2)
        assert 0<count<=6 and visibility['containing_cluster']>=0
        arguments=[count,collector+4,visibility['containing_cluster'],collector]
        portal_caller=dict(collector=collector,group_count=count,containing_cluster=visibility['containing_cluster'],visibility_sha256=sha(case/'visibility.json'),mask_pointer=u(0x505e7c))
    if a.entry==0x1770c0:arguments=[0] # Explicit ordinary world particle layer.
    if a.entry==0x1772c0:
        effect_pool=u(0x4f720c);instance=metadata['effect_instance'];effect_record=u(effect_pool+0x44)+(instance&65535)*400
        assert u(effect_record,2)==instance>>16
        particle=u(effect_record+0xfc);particle_pool=u(0x51df74);particle_record=u(particle_pool+0x44)+(particle&65535)*84
        assert particle!=0xffffffff and u(particle_record,2)==particle>>16
        arguments=[particle,0,0,0] # The actual graph's ordinary world draw input.
    queued_callback=None
    if a.queued_callback is not None:
        assert a.previous and a.entry==0x176b30
        assert 0<=a.queued_callback<u(0x4c2aa0)<=1024
        command=u(0x4c2aa8)+a.queued_callback*112
        assert u(command)==3 and u(command+32)==a.entry
        system,location=u(command+36),u(command+40)
        for slot,record,stride in ((0x51df74,system,84),(0x52d71c,location,52)):
            pool=u(slot);first=u(pool+0x44)
            assert first<=record<first+u(pool+0x38)*stride and (record-first)%stride==0
            assert u(record,2)!=0
        arguments=[command+36]
        queued_callback=dict(index=a.queued_callback,command_address=command,callback=a.entry,payload=command+36,system_record=system,location_record=location)
    else:assert a.entry!=0x176b30
    caller=struct.pack('<'+'I'*(1+len(arguments)),0x60300000,*arguments)
    old_caller=bytes(g.mem_read(gp[4],len(caller)));g.mem_write(gp[4],caller)
    (out/'native-input.bin').write_bytes(raw_input)
    def dump(name):
        with (out/name).open('wb') as f:
            for p,b in data:f.write(struct.pack('<II',p,len(b)));f.write(bytes(g.mem_read(p,len(b))))
    dump('before.bin')
    inventory=ROOT/'analysis/multiplayer/title-update-1.5/functions.json'
    starts={int(row['ea'],16) for row in json.loads(inventory.read_text())};visited=Counter();reached={};count=0;pc=0;writes=0;callers=[]
    stores=(out/'original-stores.bin').open('wb');trace=(out/'original-trace.bin').open('wb')
    def hook(uc,p,n,user):
        nonlocal count,pc
        if not any(owner.inside(p) for owner in owners):raise RuntimeError('Original reached an unsupplied external service at '+hex(p))
        raw=bytes(g.mem_read(p,n));assert raw==image.read(p,n),hex(p)
        pc=p;count+=1;trace.write(struct.pack('<I',p));reached.setdefault(p,raw.hex())
        if p in starts:
            visited[p]+=1
            if len(callers)<4096:
                sp=g.reg_read(R.UC_X86_REG_ESP)
                callers.append(dict(entry=p,order=count,gp=[g.reg_read(r) for r in REGISTERS],stack_words=[u(sp+i*4) for i in range(5)]))
    def write(uc,access,p,n,value,user):
        nonlocal writes
        if 0x60000000<=p<0x60200000:return
        old=bytes(g.mem_read(p,n));new=(value&((1<<(n*8))-1)).to_bytes(n,'little')
        stores.write(struct.pack('<III',pc,p,n));stores.write(old);stores.write(new);writes+=1
    source_faults=[]
    def invalid(uc,access,address,size,value,user):
        source_faults.append(dict(access=access,address=address,size=size,instruction=pc,gp=[g.reg_read(r) for r in REGISTERS]))
        return False
    g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
    error=None
    try:g.emu_start(a.entry,0x60300000,count=10000000)
    except (UcError,RuntimeError,AssertionError) as e:error=str(e)
    stores.close();trace.close();completed=g.reg_read(R.UC_X86_REG_EIP)==0x60300000
    if not completed and error is None:error='Instruction limit reached without returning'
    boundary=None
    if not completed and g.reg_read(R.UC_X86_REG_EIP)>=0x80000000:
        target=g.reg_read(R.UC_X86_REG_EIP);sp=g.reg_read(R.UC_X86_REG_ESP)
        boundary=dict(kind='unsupplied Xbox kernel import',target=target,ordinal=target&0x7fffffff,caller_instruction=pc,gp=[g.reg_read(r) for r in REGISTERS],stack_words=[u(sp+i*4) for i in range(8)])
        error=f'Original reached an unsupplied Xbox kernel import at {target:#x}'
    dump('original-after.bin')
    tags=g.reg_read(R.UC_X86_REG_FPTAG);depth=sum(((tags>>(2*i))&3)!=3 for i in range(8))
    exit_=struct.pack('<8I',*(g.reg_read(r) for r in REGISTERS))
    exit_+=b''.join(g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little') for i in range(8))
    exit_+=struct.pack('<IIIHHI',g.reg_read(R.UC_X86_REG_EFLAGS),g.reg_read(R.UC_X86_REG_FS_BASE),depth,g.reg_read(R.UC_X86_REG_FPSW),g.reg_read(R.UC_X86_REG_FPCW),g.reg_read(R.UC_X86_REG_MXCSR))
    (out/'original-exit.bin').write_bytes(exit_)
    report=dict(scope=__doc__,tool_sha256=sha(__file__),original_xbe_sha256=NEW_PIN,source_context=context,
        origin_case=str(case),origin_phase=a.phase,origin_bank_sha256=sha(origin),origin_hardware_report_sha256=origin_hardware_sha256,previous_evidence=previous_evidence,native_api_frame=api_frame,
        entry=a.entry,queued_callback=queued_callback,portal_caller=portal_caller,source_memory_faults=source_faults,frame_delta_bits=delta_bits,caller_argument_words=arguments,caller_frame_address=gp[4],caller_before_hex=old_caller.hex(),caller_after_hex=caller.hex(),
        supplied_game_services=[],instruction_patches=[],completed=completed,error=error,unowned_boundary=boundary,original_instructions=count,retained_stores=writes,exit_pc=g.reg_read(R.UC_X86_REG_EIP),
        original_functions={f'{p:08x}':n for p,n in sorted(visited.items())},original_function_entries=callers,
        executed_instruction_bytes={f'{p:08x}':v for p,v in sorted(reached.items())},
        files_sha256={p.name:sha(p) for p in out.iterdir() if p.is_file()},native_implementation_verified=False,physical_xbox_verified=False,full_weapon_parity=False)
    (out/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    shutil.copyfile(__file__,out/'probe-source.py')
    print(json.dumps({k:report[k] for k in ('completed','error','original_instructions','retained_stores','exit_pc','original_functions')}));return 0 if completed else 1
if __name__=='__main__':raise SystemExit(main())
