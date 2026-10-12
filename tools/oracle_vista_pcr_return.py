#!/usr/bin/env python3
"""Unchanged supplied-PC PCR Done and UI lifecycle observer, with explicit services.

No original game/loader, native compiler, audio device or GPU is run. Protected
query bodies and UI allocation/registration are recorded external boundaries.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP, UC_X86_REG_ESI
from oracle_vista_lobby_countdown import SOURCE_SHA, STOP, OBJECT, STACK, make_machine, read32, word
SESSION=0x1A00000; SCREEN=0x1B00000; WORLD=0x1C00000; UI=OBJECT
PHASE=(0x4C68,0x4C64,0x4C6C,0x796C,0x7970,0x7974,0x4C60)

def put(uc,a,v,n=4):uc.mem_write(a,struct.pack('<I',word(v))[:n])
def words(uc,a,n):return list(struct.unpack('<'+'I'*n,uc.mem_read(a,n*4)))
def machine(image):
    uc=make_machine(image)
    for a,n in ((SESSION,0x8000),(SCREEN,0x9000),(WORLD,0x2000)):uc.mem_map(a,n)
    put(uc,0x820FE8,SESSION);put(uc,0x882D3C,WORLD)
    return uc

def run(uc,address,args=(),ecx=0,services=None):
    esp=STACK+0xE000;uc.mem_write(esp,struct.pack('<'+'I'*(len(args)+1),STOP,*map(word,args)))
    uc.reg_write(UC_X86_REG_ESP,esp);uc.reg_write(UC_X86_REG_ECX,ecx)
    visits=[];events=[];clocks=0
    def ret(value=None,pop=0):
        s=uc.reg_read(UC_X86_REG_ESP)
        if value is not None:uc.reg_write(UC_X86_REG_EAX,word(value))
        uc.reg_write(UC_X86_REG_EIP,read32(uc,s));uc.reg_write(UC_X86_REG_ESP,s+4+pop)
    def hook(u,pc,size,_):
        nonlocal clocks
        visits.append(pc);s=u.reg_read(UC_X86_REG_ESP)
        if pc==STOP:u.emu_stop();return
        if pc==0x6875D3:ret();return # standard cookie check, no AL override
        if pc==0x687BA9:
            dst,val,n=words(u,s+4,3);u.mem_write(dst,bytes([val&255])*n);ret(dst);return
        if pc==0x5B3CA0:
            clocks+=1;value=(0x11111111,0x22222222)[clocks-1]
            put(u,0x91ABCC,value);events.append({'service':'network_clock','value':value});return # execute original getter
        if pc==0x5BED40:
            args=words(u,s+4,6);events.append({'service':'send','this':u.reg_read(UC_X86_REG_ECX),
                'args':args[:5],'payload':words(u,args[5],0x4B4//4)})
            ret(0,24);return
        if pc==0x60E243:events.append({'service':'dialog','args':words(u,s+4,8)});ret();return
        if pc==0x612604:events.append({'service':'close_screen','args':words(u,s+4,1)});ret(0,4);return
        if services is None:return
        queries={0x6096B4:('screen',services['screen']),0x615EA9:('protocol',services['protocol']),
            0x60A89F:('ui_active',services['ui_active']),0x609674:('ui_owner',UI),
            0x5AD718:('task_gate_a',services['task_a']),0x5AD722:('task_gate_b',services['task_b']),
            0x5AD704:('task_state',services['task_state']),0x5AD70E:('task_blocks',services['task_blocks']),
            0x6073AE:('local_controller_count',services['controllers']),
            0x5ADD68:('closing',services['closing']),0x449887:('world_gate',services['world_gate']),
            0x449876:('world_gate_b',services['world_gate_b']),0x60758D:('controller',services['controller']),
            0x439760:('loading',0),0x439EFD:('loading_secondary',0)}
        if pc in queries:
            name,v=queries[pc];events.append({'service':name,'value':word(v)});ret(v);return
        if pc in (0x60A87E,0x609790,0x6096DA,0x5AD6AF,0x617400):
            name,count={0x60A87E:('set_ui_active',1),0x609790:('ui_channel',3),0x6096DA:('ui_channel_close',2),0x5AD6AF:('session_close',1),0x617400:('observer_suffix',1)}[pc]
            events.append({'service':name,'args':words(u,s+4,count),'eax':u.reg_read(UC_X86_REG_EAX),'ecx':u.reg_read(UC_X86_REG_ECX)})
            ret();return
        if pc in (0x646752,0x6467E9,0x642DA8,0x646CEF,0x646F94):
            ptr=read32(u,s+4);events.append({'service':'screen_factory','factory':pc,'descriptor':words(u,ptr,8)})
            ret(0);return
        if pc==0x60C724:events.append({'service':'pcr16_request'});ret();return
        if pc==0x408EAF:events.append({'service':'mainmenu_request','reason':read32(u,s+4)});ret();return
        if pc==0x4096BD:events.append({'service':'other_mainmenu_request'});ret();return
    h=uc.hook_add(UC_HOOK_CODE,hook)
    try:uc.emu_start(address,STOP+1,count=100000)
    finally:uc.hook_del(h)
    assert uc.reg_read(UC_X86_REG_EIP)==STOP, 'source did not return'
    return {'return_al':uc.reg_read(UC_X86_REG_EAX)&255,'events':events,'instruction_visits':len(visits),
            'executed_entries':[hex(a) for a in (0x40C01A,0x6152B0,0x5AD940,0x6156D2,0x5AD9E3,0x5AD87C,0x5C884A,0x5C596E,
                0x6176B0,0x616D6C,0x6154D8,0x5AD660,0x616EF0,0x614F5D,0x60C851,0x409109) if a in visits]}

def done(image,**overrides):
    q=dict(enabled=1,kind=4,state=5,host=7,local=7,variant=0,code=1,controller=0,phase=5,peer_present=1)
    q.update(overrides);uc=machine(image)
    for a,v,n in ((0x820FC0,q['enabled'],1),(0x820FC4,q['kind'],4),(SESSION+0x73A0,q['state'],4),
        (SESSION+0x71C0,q['host'],4),(SESSION+0x74,q['local'],4),(SCREEN+0x7F74,q['variant'],1),
        (OBJECT+4,q['controller'],4),(OBJECT+8,q['code'],4),(0x91ABC8,1,1),
        (SESSION+0x84,1,4),(SESSION+0x71C5,q['peer_present'],1),(SESSION+0x64,0,4)):
        put(uc,a,v,n)
    before=[q['phase'],0xFFFFFFFE,0x33333333,0x80,3,0x44444444,0xFFFFFFFE]
    for off,v in zip(PHASE,before):put(uc,SESSION+off,v,1 if off==0x796C else 4)
    put(uc,SESSION+0x71C6,0x80,1)
    out=run(uc,0x40C01A,(OBJECT,),SCREEN)
    out['phase_before']=before
    out['phase_after']=[int.from_bytes(uc.mem_read(SESSION+off,1 if off==0x796C else 4),'little') for off in PHASE]
    out['peer_mark_after']=uc.mem_read(SESSION+0x71C6,1)[0]
    return {'input':q,'output':out}

def observer(image,**overrides):
    q=dict(screen=16,world_ready=1,world_kind=3,network_enabled=1,lifecycle_enabled=1,kind=1,
        protocol=3,ui_disabled=0,ui_active=0,controllers=1,task_a=0,task_b=0,task_state=0,task_retained=0,
        task_blocks=0,closing=0,world_gate=0,world_gate_b=0,controller=2)
    q.update(overrides);uc=machine(image)
    for a,v,n in ((WORLD+0x1190,q['world_ready'],1),(WORLD+8,q['world_kind'],4),(0x91C45C,q['network_enabled'],1),
        (0x820FC0,q['lifecycle_enabled'],1),(0x820FC4,q['kind'],4),(UI+0x608,q['ui_disabled'],1),
        (0x863B3C,q['task_retained'],4)):
        put(uc,a,v,n)
    out=run(uc,0x6176B0,services=q);out['retained_task_after']=read32(uc,0x863B3C)
    return {'input':q,'output':out}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
    ap.add_argument('--output',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/pcr-return-original'))
    a=ap.parse_args();image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
    a.output.mkdir(parents=True,exist_ok=True);records=[]
    def add(kind,label,result):records.append({'kind':kind,'label':label,**result})
    for kind,enabled,state,hostmatch in itertools.product((0,1,4),(0,1),(*range(12),-1),(0,1)):
        add('D',f'done_{len(records):04d}',done(image,kind=kind,enabled=enabled,state=state,local=7 if hostmatch else 8))
    for q in (dict(code=13),dict(code=0),dict(variant=1),dict(variant=0x80),dict(peer_present=0),dict(phase=1),
              dict(local=8,controller=2),dict(local=8,controller=32),dict(state=3),dict(state=4)):
        add('D',f'done_control_{len(records):04d}',done(image,**q))
    for screen,kind,protocol in itertools.product((16,185,14,13,9),(0,1,2,3,4,5,6,7),(0,3,4,5,6)):
        add('U',f'observer_{len(records):04d}',observer(image,screen=screen,kind=kind,protocol=protocol))
    for q in (dict(controllers=0),dict(controllers=-1),dict(controllers=0x8000),dict(controllers=0x10001),dict(controllers=2),
        dict(ui_disabled=1),dict(ui_disabled=0x80),dict(network_enabled=0),dict(lifecycle_enabled=0),
        dict(task_a=1,task_state=1),dict(task_b=1,task_state=1),dict(task_a=1,task_state=1,task_retained=1),
        dict(task_blocks=1),dict(task_blocks=0x80),dict(ui_active=1,protocol=4),
        dict(world_ready=1,world_kind=2,kind=4),dict(world_ready=0,world_kind=2,kind=4),
        dict(world_ready=1,world_kind=2,kind=3),dict(world_ready=1,world_kind=2,kind=4,closing=1,world_gate=1)):
        add('U',f'observer_control_{len(records):04d}',observer(image,**q))
    canonical=done(image);assert canonical['output']['phase_after'][0]==1
    normal=observer(image);factories=[e for e in normal['output']['events'] if e['service']=='screen_factory']
    assert len(factories)==1 and factories[0]['factory']==0x646752
    assert factories[0]['descriptor']==[0xFFFF0000,5,4,0,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x646752]
    changed=observer(image,world_ready=1,world_kind=2,kind=4)
    assert any(e=={'service':'mainmenu_request','reason':7} for e in changed['output']['events'])
    # Pin all original instruction corridors/table bytes used, not native outputs.
    spans={'done':(0x40C01A,0x40C0DD),'done_wrapper':(0x6156D2,0x6156E1),'host_predicate':(0x5AD940,0x5AD98F),
        'guarded_action':(0x5AD9E3,0x5ADA24),'host_action_predicate':(0x5AD87C,0x5AD8BB),
        'phase_request':(0x5C884A,0x5C8977),'phase_write':(0x5C596E,0x5C5A08),
        'screen_classification':(0x616D6C,0x616EF0),'manager_classification':(0x6154D8,0x61553C),
        'ui_observer':(0x616EF0,0x617138),'observer_caller':(0x6176B0,0x61771B),
        'factory_descriptor':(0x60C851,0x60C899),'lobby_constructor_prefix':(0x6463AB,0x646409),
        'pregame_entry':(0x5D9B62,0x5D9BA2),'reason7':(0x409109,0x409111)}
    pins={}
    import subprocess
    for name,(begin,end) in spans.items():
        raw=image[begin-0x400000:end-0x400000];(a.output/f'{name}.bin').write_bytes(raw)
        asm=subprocess.run(['objdump','-D','-Mintel',f'--start-address={begin}',f'--stop-address={end}',str(a.exe)],capture_output=True,text=True,check=True).stdout
        (a.output/f'{name}.asm').write_text(asm)
        pins[name]={'begin':hex(begin),'end_exclusive':hex(end),'sha256':hashlib.sha256(raw).hexdigest()}
    report={'schema':'H2V_PCR_RETURN_ORIGINAL_1','source':{'path':str(a.exe),'sha256':SOURCE_SHA},
        'oracle_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'case_count':len(records),'instruction_visits':sum(r['output']['instruction_visits'] for r in records),
        'records':records,'source_spans':pins,
        'limits':['Protected task queries and UI/current-screen/controller/protocol queries are explicit callback inputs.',
          'Factory requests execute60C851 descriptor construction, then stop at actual factory; allocation/registration/child construction are not emulated.',
          'Observer suffix617400 is separately recorded; this oracle does not claim the complete UI update or a frame gap.',
          'Done executes actual phase writer; network send is an explicit recorded service and clocks are supplied actual getter override inputs.',
          'Reason7 request reaches408EAF boundary; protected4089BA map-load completion is not reconstructed.',
          'Cases are discriminating input controls, not observed session constructors or proof every state is reachable.']}
    path=a.output/'report.json';path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'report':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'cases':len(records),'instruction_visits':report['instruction_visits']}))
if __name__=='__main__':main()
