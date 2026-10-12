#!/usr/bin/env python3
"""Execute pinned, unchanged PC lifecycle request/pump/handler instructions.

External query/playback/network callbacks have explicit inputs and reentrant
mutations. No game, loader, audio device, native compiler or GPU is executed.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
BASE=0x400000; STOP=0x1800000; MANAGER=0x1810000; SESSION=0x1A00000
OPTIONS=0x1A10000; HANDLERS=0x1B00000; VTABLES=0x1B01000; STACK=0x1900000
CB_UPDATE=STOP+0x100; CB_ENTER=STOP+0x200; CB_EXIT=STOP+0x300
SOURCE_SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
PAYLOAD_WORDS=8
METHODS={'R':0x5D75DA,'P':0x5D7738,'I':0x5D95A7,'G':0x5D903F,'E':0x5D9070,'J':0x5D7738}
# External service ordinals are production API ordinals, independently captured
# from original call addresses (not from native-computed behavior).
SERVICES={0x5D7644:0,0x5AD423:1,0x5ADD8E:2,0x5AD85D:3,0x4499A8:4,
          0x5C596E:5,0x5CC0A8:6,0x5C2293:7,0x449FD7:8,0x4497EA:9,
          0x5C23A7:11,0x5D94A1:13,0x5D75DA:14,0x5B07A0:15,0x5D763E:16}
def w(v):return v&0xffffffff
def pack(values):return struct.pack('<'+'I'*len(values),*map(w,values))
def manager(kind=3,pending=0,requested=4,length=8):
    return [kind,0x81,0x92,pending,requested,w(length),*[0xAABBCC00+i for i in range(PAYLOAD_WORDS)]]
def action(typ,kind=0,ret=0,request=None,overwrite=None,pending=None):
    # type,handler,AL,effect bits,request kind/length,source8,overwrite8,pendingbyte
    mask=(1 if request else 0)|(2 if overwrite is not None else 0)|(4 if pending is not None else 0)
    reqkind,length,source=request if request else (0,0,[0]*8)
    return [typ,kind,ret,mask,reqkind,w(length),*source,*(overwrite or [0]*8),pending or 0]
def query(**changes):
    # Same explicit input projection as VistaLifecycleInGameQueries.
    q=[0,0,1,0,1,4,0,0,0,0x11223344,0x55667788,0x11223344,0x55667788,0,7,8,7,8]
    names=['preflight','pending','admitted','closing','authority','phase','complete','force','ready',
           'retained0','retained1','identity0','identity1','map_valid','selected0','selected1','world0','world1']
    for key,value in changes.items():q[names.index(key)]=w(value)
    return q
class Oracle:
    def __init__(self,image):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32);self.uc.mem_map(BASE,(len(image)+4095)&~4095);self.uc.mem_write(BASE,image)
        for address,size in ((STOP,0x1000),(MANAGER,0x1000),(SESSION,0x8000),(OPTIONS,0x1000),(HANDLERS,0x2000),(STACK,0x10000)):
            self.uc.mem_map(address,size)
        self.uc.hook_add(UC_HOOK_CODE,self.hook);self.visits=0
    def get(self,a):return struct.unpack('<I',self.uc.mem_read(a,4))[0]
    def put(self,a,v,size=4):self.uc.mem_write(a,pack([v])[:size])
    def bank(self):
        return [self.get(MANAGER),*self.uc.mem_read(MANAGER+0x34,3),self.get(MANAGER+0x38),self.get(MANAGER+0x3C),
                *struct.unpack('<8I',self.uc.mem_read(MANAGER+0x40,32))]
    def set_bank(self,b):
        self.put(MANAGER,b[0]);self.uc.mem_write(MANAGER+0x34,bytes(b[1:4]));self.put(MANAGER+0x38,b[4]);self.put(MANAGER+0x3C,b[5]);self.uc.mem_write(MANAGER+0x40,pack(b[6:]))
    def kind(self,p):
        d=p-HANDLERS
        if d<0 or d%0x100 or d//0x100>=8:raise RuntimeError('unowned handler pointer')
        return d//0x100
    def ret(self,value=0,pop=0):
        s=self.uc.reg_read(UC_X86_REG_ESP);self.uc.reg_write(UC_X86_REG_EAX,w(value));self.uc.reg_write(UC_X86_REG_EIP,self.get(s));self.uc.reg_write(UC_X86_REG_ESP,s+4+pop)
    def callback(self,typ,kind,other=0,length=0,payload=()):
        self.events.append([typ,kind,other,w(length),len(payload),*payload,*self.bank()])
        if self.kind_of_run=='J':return 0
        if self.step>=len(self.actions):raise RuntimeError('missing explicit callback action')
        a=self.actions[self.step];self.step+=1
        if (typ,kind)!=(a[0],a[1]):raise RuntimeError(f'callback mismatch {(typ,kind)} vs {a[:2]}')
        mask=a[3]
        if mask&1:
            # Reentrant external callback request uses the SAME unchanged request
            # writer through a nested original emulation entry, outside this hook.
            # Its exact field effects are supplied here and separately proven R.
            self.put(MANAGER+0x38,a[4]);self.put(MANAGER+0x36,1,1);self.put(MANAGER+0x3C,a[5]);self.put(MANAGER+0x40,0)
            n=struct.unpack('<i',pack([a[5]]))[0]
            if n>0:self.uc.mem_write(MANAGER+0x40,pack(a[6:14])[:n])
        if mask&2:self.uc.mem_write(MANAGER+0x40,pack(a[14:22]))
        if mask&4:self.put(MANAGER+0x36,a[22],1)
        return a[2]
    def hook(self,uc,pc,size,_):
        self.visits+=1;s=uc.reg_read(UC_X86_REG_ESP)
        if pc==STOP:uc.emu_stop();return
        if pc==0x687E18:
            dst,src,n=(self.get(s+i) for i in (4,8,12));uc.mem_write(dst,bytes(uc.mem_read(src,n)));self.ret(dst);return
        if pc==0x5EA803:
            self.callback(0,0);self.ret();return
        if pc in (CB_UPDATE,CB_EXIT,CB_ENTER):
            kind=self.kind(uc.reg_read(UC_X86_REG_ECX))
            if pc==CB_UPDATE:ret=self.callback(1,kind);pop=0
            elif pc==CB_EXIT:ret=self.callback(2,kind,self.kind(self.get(s+4)));pop=4
            else:
                length=self.get(s+8);ptr=self.get(s+12);signed=struct.unpack('<i',pack([length]))[0]
                payload=list(struct.unpack('<'+'I'*(signed//4),uc.mem_read(ptr,signed))) if signed>0 else []
                ret=self.callback(3,kind,self.kind(self.get(s+4)),length,payload);pop=12
            self.ret(ret,pop);return
        if pc==0x449801:
            ordinal=10 if self.options_count==0 else 12;self.options_count+=1
            self.calls.append([ordinal]);self.ret(OPTIONS);return
        if pc not in SERVICES:return
        service=SERVICES[pc];args=[]
        if service in (5,6,15):args=[self.get(s+4)]
        if service==14:args=[self.get(s+4),self.get(s+8),self.get(s+12)]
        if service==16:args=[self.kind(self.get(s+4)),self.get(s+8),self.get(s+12)]
        self.calls.append([service,*args])
        if service in (14,16):return # Execute unchanged request / original no-op base enter.
        q=self.q
        if service==0:
            self.put(MANAGER+0x36,q[1],1);self.ret(q[0]);return
        values={1:q[2],2:q[3],3:q[4],4:q[6],7:q[7],9:q[8]}
        if service in values:self.ret(values[service]);return
        if service==5:
            if self.kind_of_run=='J':self.put(SESSION+0x4C68,args[0])
            self.ret(0,4);return
        if service==6:self.ret(0,4);return
        if service==11:
            self.put(self.get(s+4),q[14]);self.put(self.get(s+8),q[15]);self.put(self.get(s+12),0);self.ret(q[13],12);return
        if service==13:self.ret(0,4);return
        self.ret()
    def execute(self,kind,before,inputs=(),actions=()):
        self.kind_of_run=kind;self.actions=actions;self.step=0;self.events=[];self.calls=[];self.options_count=0
        self.q=list(inputs) if kind in ('I','J') else query()
        if kind=='G':self.q[0],self.q[1],self.q[5]=inputs
        self.set_bank(before);self.put(MANAGER+0x24,SESSION);self.put(SESSION+0x4C68,self.q[5])
        self.uc.mem_write(OPTIONS+0x10,pack(self.q[11:13]));self.uc.mem_write(OPTIONS+0x80,pack(self.q[16:18]))
        for i in range(8):
            handler=HANDLERS+i*0x100;vt=VTABLES+i*16
            self.put(MANAGER+4+i*4,handler)
            self.put(handler,vt);self.put(handler+4,i);self.put(handler+8,MANAGER);self.uc.mem_write(handler+0x10,pack(self.q[9:11]))
            update=METHODS['I'] if kind=='J' and i==3 else METHODS['G'] if kind=='J' and i==4 else CB_UPDATE
            enter=METHODS['E'] if kind=='J' and i==4 else CB_ENTER
            self.uc.mem_write(vt,pack([update,enter,CB_EXIT,0]))
        esp=STACK+0x8000;args=[]
        if kind=='R':
            reqkind,length,source=inputs[0],inputs[1],inputs[2:];self.uc.mem_write(OPTIONS+0x200,pack(source));args=[reqkind,length,OPTIONS+0x200]
        if kind=='E':args=[HANDLERS+inputs[0]*0x100,0,0]
        self.uc.mem_write(esp,pack([STOP,*args]));self.uc.reg_write(UC_X86_REG_ESP,esp)
        self.uc.reg_write(UC_X86_REG_ECX,MANAGER if kind in ('R','P','J') else HANDLERS+(4 if kind in ('G','E') else 3)*0x100)
        try:self.uc.emu_start(METHODS[kind],STOP+1,count=30000)
        except Exception as error:raise RuntimeError(f'kind={kind} pc={self.uc.reg_read(UC_X86_REG_EIP):08x} bank={self.bank()} events={self.events}') from error
        if self.uc.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original did not return (unbounded loop or missing owner)')
        if kind=='P' and self.step!=len(actions):raise RuntimeError('unused callback actions')
        return dict(after=self.bank(),repeat=self.uc.reg_read(UC_X86_REG_EAX)&255,calls=self.calls,events=self.events,phase=self.get(SESSION+0x4C68))
def call_words(calls):
    out=[len(calls)]
    for c in calls:out += [c[0],len(c)-1,*c[1:]]
    return out
def event_words(events):
    out=[len(events)]
    for e in events:out += [len(e),*e]
    return out

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
    p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_lifecycle_original.tsv'))
    p.add_argument('--report',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/lifecycle-original.json'));a=p.parse_args()
    image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SOURCE_SHA
    o=Oracle(image);records=[];lines=['# H2V_LIFECYCLE_ORIGINAL_V1','# kind label word_count semantic_u32_words_hex; payload extent is explicit32B, not original capacity']
    def add(kind,label,before,inputs=(),actions=()):
        r=o.execute(kind,before,inputs,actions);words=[]
        if kind=='R':words=[*before,*inputs,*r['after']]
        elif kind=='P':words=[*before,len(actions),*itertools.chain.from_iterable(actions),*r['after'],*event_words(r['events'])]
        elif kind in ('I','G','E'):words=[*inputs,r['repeat'] if kind!='E' else 0,*call_words(r['calls'])]
        elif kind=='J':words=[*before,*inputs,*r['after'],r['phase'],*call_words(r['calls']),*event_words(r['events'])]
        records.append(dict(kind=kind,label=label,before=before,inputs=inputs,actions=actions,result=r))
        lines.append('\t'.join([kind,label,str(len(words)),*(f'{w(x):08x}' for x in words)]))
    for k,n in itertools.product((0,1,3,4,0xffffffff),(-0x80000000,-1,0,1,3,4,8,31,32)):
        add('R',f'request_{len(records):04d}',manager(),[k,w(n),*[0x12340000+i for i in range(8)]])
    for pre,pending,admit,phase in itertools.product((0,1,0x80),(0,1,0xff),(0,1,0xff),(0,1,4,5,6,0xffffffff)):
        add('I',f'entry_guard_{len(records):04d}',manager(),query(preflight=pre,pending=pending,admitted=admit,phase=phase))
    for closing,authority,complete,force,ready,identity,mapvalid,mapmatch in itertools.product((0,1),repeat=8):
        add('I',f'ingame_{len(records):04d}',manager(),query(closing=closing,authority=authority,complete=complete,force=force,ready=ready,identity1=0x55667788+identity,map_valid=mapvalid,world1=8+mapmatch))
    for name,changes in [('raw_guard',dict(preflight=0xff)),('raw_complete',dict(complete=0x80,ready=0xff,map_valid=0xff,world1=9)),('raw_closing',dict(closing=0xff,authority=0x80)),('raw_force',dict(force=0xff,ready=0x80)),('identity_first_word',dict(ready=1,identity0=0x11223345,map_valid=1,world0=9)),('map_first_word',dict(ready=1,map_valid=1,world0=9)),('map_second_word',dict(ready=1,map_valid=1,world1=9))]:
        add('I',name,manager(),query(**changes))
    for pre,pending,phase in itertools.product((0,1,0x80),(0,1,0xff),(0,4,5,6,0xffffffff)):
        add('G',f'postgame_{len(records):04d}',manager(4),[pre,pending,phase])
    for old in (0,1,3,4,7):add('E',f'post_enter_{old}',manager(4),[old])
    src=[0x10203040+i for i in range(8)];overwrite=[0xDEADB000+i for i in range(8)]
    plans=[('quiet',manager(),[action(0),action(1,3)]),
      ('repeat_raw_AL',manager(),[action(0),action(1,3,0x80),action(1,3)]),
      ('same_kind',manager(pending=0xff,requested=3,length=-1),[action(0),action(1,3)]),
      ('ordinary_transition',manager(pending=0x80,requested=4,length=8),[action(0),action(2,3),action(3,4),action(1,4)]),
      ('exit_payload_alias',manager(pending=1,requested=4,length=16),[action(0),action(2,3,overwrite=overwrite),action(3,4),action(1,4)]),
      ('exit_reentrant_request',manager(pending=1,requested=4,length=16),[action(0),action(2,3,request=(1,8,src)),action(3,4),action(2,4),action(3,1),action(1,1)]),
      ('enter_reentrant_request',manager(pending=1,requested=4,length=8),[action(0),action(2,3),action(3,4,request=(1,0,src)),action(2,4),action(3,1),action(1,1)]),
      ('update_reentrant_request',manager(),[action(0),action(1,3,request=(4,8,src)),action(2,3),action(3,4),action(1,4)]),
      ('network_reentrant_request',manager(),[action(0,request=(4,8,src)),action(2,3),action(3,4),action(1,4)]),
      ('same_request_from_update',manager(),[action(0),action(1,3,request=(3,0,src)),action(1,3)]),
      ('negative_length',manager(pending=1,requested=4,length=-0x80000000),[action(0),action(2,3),action(3,4),action(1,4)]),
      ('zero_length',manager(pending=1,requested=4,length=0),[action(0),action(2,3),action(3,4),action(1,4)])]
    for label,before,actions in plans:add('P',label,before,actions=actions)
    # Original methods3/4 run within original pump. Network/old-exit and phase
    # write side effects are explicit boundaries; no renderer or frames exist.
    add('J','same_pump_phase4_to_postgame',manager(),query(complete=1))
    a.fixture.parent.mkdir(parents=True,exist_ok=True);a.fixture.write_text('\n'.join(lines)+'\n')
    windows={'request':(0x5D75DA,0x5D760E),'pump':(0x5D7738,0x5D77CF),'in_game':(0x5D95A7,0x5D9704),'post_game':(0x5D903F,0x5D9070),'post_enter':(0x5D9070,0x5D909A),'constant_false':(0x404541,0x404544)}
    report={'version':'H2V_LIFECYCLE_ORIGINAL_V1','source':str(a.exe.resolve()),'source_sha256':SOURCE_SHA,'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),'case_count':len(records),'counts':{k:sum(r['kind']==k for r in records) for k in METHODS},'instruction_visits':o.visits,'ranges':{n:{'begin':hex(x),'end_exclusive':hex(y),'sha256':hashlib.sha256(image[x-BASE:y-BASE]).hexdigest()} for n,(x,y) in windows.items()},'boundary':{'unchanged_instructions':True,'payload':'32-byte caller-owned test extent; no source capacity claim. Positive pointer remains alias through exit and enter.','callbacks':'Explicit external query results, network and old-exit callbacks; reentrant manager mutation is fixture input. Reentrant request side effects are independently executed request cases; callback mutation itself is not original service execution.','phase_write':'Explicit joined external effect writes phase5 only; full PC writer already has separate original fixture, not reconstructed here.','preflight':'5D7644 is explicit query/side-effect boundary, not asserted to be pure.','world':'Actual identity/map query services return supplied options; loading/world/service side effects remain external.','repeat':'RawAL retained. No frame-gap or original maximum-iteration claim.','memcpy':'687E18 copies exact positive request bytes; overlapping request-source pointer behavior is outside fixture.'},'records':records}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('case_count','counts','instruction_visits','fixture_sha256')}))
if __name__=='__main__':main()
