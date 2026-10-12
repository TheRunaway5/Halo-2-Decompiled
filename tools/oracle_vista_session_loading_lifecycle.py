#!/usr/bin/env python3
"""Execute unchanged supplied-PC start-game handler, init, and gate instructions.

Player preparation/parameter refresh and source session actions are explicit
external services. No world, native session, loader, audio, or GPU is started.
"""
from pathlib import Path
import argparse, hashlib, itertools, json, struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP

SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BASE=0x400000;STOP=0x1800000;MANAGER=0x1810000;SESSION=0x1A00000
HANDLER=0x1B00000;STACK=0x1900000;OLD=0x1B01000
SERVICES={0x5D763E:0,0x45906F:1,0x5B5A84:2,0x5C5D9B:3,
          0x5C5E5B:4,0x5C5E03:5,0x5C5EB7:6,0x5D7644:7,
          0x5AD423:8,0x5AD85D:9,0x5D991C:10,0x5E0D4F:11,
          0x5C5BC5:12,0x5C969F:13,0x5C596E:14,0x5D75DA:15}
WINDOWS={'registration':(0x5D972B,0x5D973C),'exit':(0x5D9741,0x5D9746),
         'initialize_parameters':(0x5D9746,0x5D97BB),'gate':(0x5D97BB,0x5D9852),
         'enter':(0x5D98ED,0x5D991C),'update':(0x5D9A9F,0x5D9B4C),
         'player_preparation_external':(0x5D991C,0x5D9A9F),
         'parameter_refresh_external':(0x5C5BC5,0x5C5BE6),
         'identity_seed_mutators_external':(0x5C5D9B,0x5C5EB7),
         'prepared_bank_mutator_external':(0x5C5EB7,0x5C5F7B),
         'countdown_clear_external':(0x5C969F,0x5C96FB),
         'network_query':(0x5E0D4F,0x5E0D55),'base_enter_exit':(0x5D763E,0x5D7644),
         'manager_pump':(0x5D7738,0x5D77CF),'request':(0x5D75DA,0x5D760E),
         'ingame_enter':(0x5D9704,0x5D972B),
         'vtable':(0x7C6330,0x7C6344)}
def word(v):return v&0xffffffff
def packed(v):return struct.pack('<'+'I'*len(v),*map(word,v))
def call_words(calls):
    result=[len(calls)]
    for c in calls:result += [c[0],len(c)-1,*c[1:]]
    return result
class Oracle:
    def __init__(self,image):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32)
        self.uc.mem_map(BASE,(len(image)+4095)&~4095);self.uc.mem_write(BASE,image)
        for a,n in ((STOP,0x1000),(MANAGER,0x1000),(SESSION,0x8000),(HANDLER,0x2000),(STACK,0x10000)):
            self.uc.mem_map(a,n)
        self.uc.hook_add(UC_HOOK_CODE,self.hook);self.visits=0
    def put(self,a,v,n=4):self.uc.mem_write(a,packed([v])[:n])
    def get(self,a):return struct.unpack('<I',self.uc.mem_read(a,4))[0]
    def ret(self,v=0,pop=0):
        s=self.uc.reg_read(UC_X86_REG_ESP);self.uc.reg_write(UC_X86_REG_EAX,word(v))
        self.uc.reg_write(UC_X86_REG_EIP,self.get(s));self.uc.reg_write(UC_X86_REG_ESP,s+4+pop)
    def hook(self,uc,pc,size,_):
        self.visits+=1;s=uc.reg_read(UC_X86_REG_ESP)
        if pc==STOP:uc.emu_stop();return
        if getattr(self,'joined',False):
            if pc==0x5EA803:self.events.append(['network']);self.ret();return
            if pc==0x5D9A9F:self.events.append(['update',2,self.get(SESSION+0x4C68)])
            if pc==0x5D9741:self.events.append(['exit',2,3])
            if pc==0x5D9704:self.events.append(['enter',3,2,self.get(s+8),self.get(s+12)])
            if pc==0x5D94A1:self.events.append(['ingame_initialization']);self.ret(0,4);return
            if pc==STOP+0x100:self.events.append(['update',3]);self.ret();return
        if pc not in SERVICES:return
        ordinal=SERVICES[pc];args=[]
        if ordinal==0:
            if getattr(self,'joined',False):
                args=[2,self.get(s+8),self.get(s+12)];assert self.get(s+4)==HANDLER
            else:
                args=[self.old,self.get(s+8),self.get(s+12)];assert self.get(s+4)==OLD
        elif ordinal==2:
            args=[self.get(s+4)];assert args==[8]
        elif ordinal in (3,6):args=[self.get(s+4),self.get(s+8)]
        elif ordinal in (4,14):args=[self.get(s+4)]
        elif ordinal==12:args=[self.get(s+4),self.get(s+8)&255]
        elif ordinal==15:args=[self.get(s+4),self.get(s+8),self.get(s+12)]
        self.calls.append([ordinal,*args])
        if ordinal==0:return # Execute literal no-op base-enter instructions.
        if self.mode=='E':
            values=self.inputs
            if ordinal==1:self.ret(values[3]);return
            if ordinal==2:
                uc.mem_write(self.get(s+8),packed(values[4:6]));self.ret();return
            if ordinal in (3,6):self.ret(1,8);return
            if ordinal==4:self.ret(1,4);return
            if ordinal==5:self.ret(1);return
            raise RuntimeError(f'unexpected enter service {pc:x}')
        q=self.inputs
        if ordinal==7:self.put(MANAGER+0x36,q[3],1);self.ret(q[2]);return
        if ordinal==8:self.ret(q[4]);return
        if ordinal==9:self.ret(q[6]);return
        if ordinal==10:
            # Explicit actual views after the external preparation callback.
            self.put(SESSION+0x73A0,q[8]);self.put(SESSION+0x4C74,q[10],1)
            self.put(SESSION+0x73A4,q[12],1);self.ret(q[7]);return
        if ordinal==11:
            # Execute actual5E0D4F getter. Subsequent source reads are retained
            # separately to cover a declared reentrant external state change.
            self.put(SESSION+0x73A0,q[11]);self.put(SESSION+0x4C74,q[10],1)
            self.put(SESSION+0x73A4,q[12],1);self.put(0x926CFC,q[9],1);return
        if ordinal==12:self.ret(q[13],8);return
        if ordinal==13:self.ret(1);return
        if ordinal==14:
            if getattr(self,'joined',False):self.put(SESSION+0x4C68,args[0])
            self.ret(0,4);return
        if ordinal==15:
            if getattr(self,'joined',False):return # Actual request writer.
            self.ret(0,12);return
        raise RuntimeError(f'unexpected update service {pc:x}')
    def execute(self,mode,inputs):
        self.joined=False;self.mode=mode;self.inputs=list(inputs);self.calls=[]
        self.put(HANDLER+8,MANAGER);self.put(MANAGER+0x24,SESSION)
        if mode=='E':
            old,state,phase,seed,lo,hi,b10,b11=inputs;self.old=old
            self.put(SESSION+0x73A0,state);self.put(SESSION+0x4C68,phase)
            self.put(HANDLER+0x10,b10,1);self.put(HANDLER+0x11,b11,1)
            args=[OLD,0xfeed1234,0x1234feed];entry=0x5D98ED
        else:
            self.put(HANDLER+0x10,inputs[0],1);self.put(HANDLER+0x11,inputs[1],1)
            self.put(MANAGER+0x36,0,1);self.put(SESSION+0x4C68,inputs[5]);args=[];entry=0x5D9A9F
        esp=STACK+0x8000;self.uc.mem_write(esp,packed([STOP,*args]))
        self.uc.reg_write(UC_X86_REG_ESP,esp);self.uc.reg_write(UC_X86_REG_ECX,HANDLER)
        self.uc.emu_start(entry,STOP+1,count=10000)
        if self.uc.reg_read(UC_X86_REG_EIP)!=STOP:raise RuntimeError('original did not return')
        return {'flags':list(self.uc.mem_read(HANDLER+0x10,2)),
                'repeat':self.uc.reg_read(UC_X86_REG_EAX)&255 if mode=='U' else 0,
                'calls':self.calls}
    def execute_joined(self):
        self.joined=True;self.mode='U';self.inputs=queries(network=0,flag10=1);self.calls=[];self.events=[]
        self.put(MANAGER,2);self.put(MANAGER+0x24,SESSION);self.put(MANAGER+0x36,0,1)
        self.put(SESSION+0x4C68,2);self.put(SESSION+0x73A0,5)
        self.put(HANDLER+8,MANAGER);self.put(OLD+8,MANAGER)
        self.put(HANDLER+0x10,1,1);self.put(HANDLER+0x11,0x80,1)
        self.put(HANDLER,HANDLER+0x100);self.put(OLD,OLD+0x100)
        self.uc.mem_write(HANDLER+0x100,packed([0x5D9A9F,0x5D98ED,0x5D9741]))
        self.uc.mem_write(OLD+0x100,packed([STOP+0x100,0x5D9704,0x5D90B5]))
        self.put(MANAGER+4+2*4,HANDLER);self.put(MANAGER+4+3*4,OLD)
        esp=STACK+0x8000;self.uc.mem_write(esp,packed([STOP]))
        self.uc.reg_write(UC_X86_REG_ESP,esp);self.uc.reg_write(UC_X86_REG_ECX,MANAGER)
        self.uc.emu_start(0x5D7738,STOP+1,count=30000)
        assert self.uc.reg_read(UC_X86_REG_EIP)==STOP
        assert self.events==[['network'],['update',2,2],['update',2,4],['exit',2,3],
                            ['enter',3,2,0,0],['ingame_initialization'],['update',3]]
        assert self.get(MANAGER)==3 and self.get(SESSION+0x4C68)==4
        assert self.uc.mem_read(MANAGER+0x36,1)==b'\0'
        return {'events':self.events,'calls':self.calls,'current_kind':self.get(MANAGER),
                'phase':self.get(SESSION+0x4C68),'pending':0}
def queries(**changes):
    names=['flag10','flag11','preflight','pending','admitted','phase','authority',
           'prepared','state','network','local_flag','later_state','transition','refresh']
    q=[0,0x80,0,0,1,2,1,1,5,1,0,5,0,1]
    for k,v in changes.items():q[names.index(k)]=v
    return q
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
    p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_session_loading_lifecycle_original.tsv'))
    p.add_argument('--directory',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/start-game-lifecycle-original'))
    a=p.parse_args();image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
    a.directory.mkdir(parents=True,exist_ok=True);o=Oracle(image);records=[]
    lines=['# H2V_SESSION_LOADING_LIFECYCLE_ORIGINAL_V1',
           '# E/U label word_count semantic_u32_hex; external service boundaries are declared in report']
    def add(mode,label,inputs):
        before=o.visits;r=o.execute(mode,inputs)
        words=[*inputs,*r['flags'],r['repeat'],*call_words(r['calls'])]
        lines.append('\t'.join([mode,label,str(len(words)),*(f'{word(x):08x}' for x in words)]))
        records.append(dict(mode=mode,label=label,inputs=inputs,result=r,instruction_visits=o.visits-before))
    for state,phase,old in itertools.product((0,3,4,5,6,7,8,9,0xffffffff),(0,1,2,3,4,0xffffffff),(0,1,3,7)):
        add('E',f'enter_{len(records):04d}',[old,state,phase,0x12345678,0xabcdef01,0x98765432,0x80,0xff])
    for pre,pending,admitted,phase in itertools.product((0,1,0x80),(0,1,0xff),(0,1,0xff),(0,1,2,3,4,5,0xffffffff)):
        add('U',f'admission_{len(records):04d}',queries(preflight=pre,pending=pending,admitted=admitted,phase=phase))
    for authority,prepared in itertools.product((0,1,0xff),repeat=2):
        add('U',f'preparation_{len(records):04d}',queries(authority=authority,prepared=prepared))
    for state,network,localflag,laterstate,transition,b10,b11,refresh in itertools.product(
            (0,3,4,5,6,7,8,9,0xffffffff),(0,1,0x80),(0,0xff),(5,6,7,8),(0,0x80),(0,0xff),(0,0x80),(0,1,0xff)):
        add('U',f'gate_{len(records):05d}',queries(state=state,network=network,local_flag=localflag,
            later_state=laterstate,transition=transition,flag10=b10,flag11=b11,refresh=refresh))
    a.fixture.write_text('\n'.join(lines)+'\n')
    joined_start=o.visits;joined=o.execute_joined();joined['instruction_visits']=o.visits-joined_start
    md=Cs(CS_ARCH_X86,CS_MODE_32);windows={};asm=[]
    for name,(lo,hi) in WINDOWS.items():
        b=image[lo-BASE:hi-BASE]
        windows[name]={'begin':hex(lo),'end_exclusive':hex(hi),'sha256':hashlib.sha256(b).hexdigest(),'bytes':b.hex()}
        asm.append(f'; {name} {lo:08X}..{hi:08X}')
        if name!='vtable':asm += [f'{x.address:08X} {x.mnemonic} {x.op_str}' for x in md.disasm(b,lo)]
    (a.directory/'source.asm').write_text('\n'.join(asm)+'\n')
    report={'source_sha256':SHA,'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
            'case_count':len(records),'instruction_visits':o.visits,'windows':windows,
            'boundaries':['Actual unchanged enter5D98ED,init5D9746,update5D9A9F,gate5D97BB and network getter5E0D4F execute.',
                'Seed/random,preflight,admission,authority,player preparation5D991C,parameter refresh5C5BC5 and session mutators/requests are explicit service inputs or intents.',
                'Later gate state fields are independent actual views after external preparation/network services; no source byte is patched.',
                'No full player-preparation bank, session, loader, readiness, frame gap, game, native executable or GPU is constructed.'],
            'joined_original_pump':joined,'records':records}
    (a.directory/'execution.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('source_sha256','fixture_sha256','case_count','instruction_visits')}))
if __name__=='__main__':main()
