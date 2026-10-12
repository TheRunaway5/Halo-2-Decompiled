#!/usr/bin/env python3
"""Original-PC owned-bank player preparation and parameter refresh oracle.

All getters, row/address builders, source setters, refresh, source clock getter
and security-cookie check execute unchanged. Only memcpy/memset are services;
the refresh clock's original override bank supplies its explicit sample.
"""
from pathlib import Path
import argparse,hashlib,itertools,json,struct
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EIP,UC_X86_REG_ESP
SHA='33119398d3f68ad569c5010b652039105d3ce1b6512c418508e49ce69309f109'
BASE=0x400000;STOP=0x1800000;SESSION=0x1A00000;STACK=0x1900000;SIZE=0x7b88
METHODS={'P':0x5D991C,'R':0x5C5BC5}
WINDOWS={'preparation':(0x5D991C,0x5D9A9F),'row_builder':(0x5D9852,0x5D98ED),
 'address_copy':(0x5B5933,0x5B5949),'prepared_getter':(0x5C2513,0x5C256F),
 'membership_getter':(0x5C20C8,0x5C20EB),'variant_getter':(0x5C24DB,0x5C2513),
 'word_getter':(0x5C25A7,0x5C25DF),'prepared_setter':(0x5C5EB7,0x5C5F7B),
 'parameter_setters':(0x5C5CE3,0x5C5D9B),'refresh_wrapper':(0x5C5BC5,0x5C5BE6),
 'refresh_action':(0x5C4F33,0x5C500A),'network_clock':(0x5B3CA0,0x5B3CB4),
 'cookie':(0x6875D3,0x6875DD)}
def u(v):return v&0xffffffff
def pack(v):return struct.pack('<'+'I'*len(v),*map(u,v))
def pattern(seed):return bytearray((i*37+(i>>8)*17+seed)&255 for i in range(SIZE))
def put(b,o,v,n=4):b[o:o+n]=pack([v])[:n]
def patches(before,after):
    result=[];i=0
    while i<len(before):
        if before[i]==after[i]:i+=1;continue
        lo=i;i+=1
        while i<len(before) and before[i]!=after[i]:i+=1
        result.append(f'{lo:x}:{after[lo:i].hex()}')
    return ','.join(result) or '-'
def preparation_bank(seed=19,state=5,present=0,count=1,active=1,teamflags=1,
                     option=0xffff,team=1,revision=0x12345678,skipmask=0,peerindex=None):
    b=pattern(seed)
    for o,v,n in [(0x73a0,state,4),(0x5101,present,1),(0x84,count,4),(0x1258,active,4),
                  (0x4e84,teamflags,1),(0x5eb8,option,2),(0x4c60,revision,4)]:put(b,o,v,n)
    for i in range(16):
        o=0x125c+i*0x128
        put(b,o+8,(i%18) if peerindex is None else peerindex)
        put(b,o+0x14,0xffffffff if skipmask&(1<<i) else i)
        put(b,o+0x118,team,1)
    return b
class Oracle:
    def __init__(self,image):
        self.uc=Uc(UC_ARCH_X86,UC_MODE_32);self.uc.mem_map(BASE,(len(image)+4095)&~4095);self.uc.mem_write(BASE,image)
        for a,n in ((STOP,0x1000),(SESSION,0x8000),(STACK,0x10000)):self.uc.mem_map(a,n)
        self.uc.hook_add(UC_HOOK_CODE,self.hook);self.visits=0
    def get(self,a):return struct.unpack('<I',self.uc.mem_read(a,4))[0]
    def ret(self,v,pop=0):
        s=self.uc.reg_read(UC_X86_REG_ESP);self.uc.reg_write(UC_X86_REG_EAX,u(v))
        self.uc.reg_write(UC_X86_REG_EIP,self.get(s));self.uc.reg_write(UC_X86_REG_ESP,s+4+pop)
    def hook(self,uc,pc,size,_):
        self.visits+=1;s=uc.reg_read(UC_X86_REG_ESP)
        if pc==STOP:uc.emu_stop();return
        if pc==0x687BA9:
            dst,value,n=(self.get(s+i) for i in (4,8,12));uc.mem_write(dst,bytes([value&255])*n);self.ret(dst);return
        if pc==0x687E18:
            dst,src,n=(self.get(s+i) for i in (4,8,12));uc.mem_write(dst,bytes(uc.mem_read(src,n)));self.ret(dst);return
        if pc==0x5B3CA0:
            self.clock_calls+=1
            uc.mem_write(0x91ABCC,pack([self.clock])) # Actual override getter executes.
        if pc in (0x5D9852,0x5B5933,0x5C5EB7,0x5C5CE3,0x5C5D3F,0x5C4F33):
            self.calls.append(hex(pc))
    def execute(self,kind,before,target=-1,flag=0,clock=0):
        self.uc.mem_write(SESSION,bytes(before));self.uc.mem_write(SESSION+SIZE,b'\xa5'*32)
        self.uc.mem_write(0x91ABC8,b'\1');self.clock=clock;self.clock_calls=0;self.calls=[]
        args=[SESSION] if kind=='P' else [u(target),flag]
        esp=STACK+0x8000;self.uc.mem_write(esp,pack([STOP,*args]))
        self.uc.reg_write(UC_X86_REG_ESP,esp);self.uc.reg_write(UC_X86_REG_ECX,SESSION)
        self.uc.emu_start(METHODS[kind],STOP+1,count=50000)
        assert self.uc.reg_read(UC_X86_REG_EIP)==STOP
        assert self.uc.mem_read(SESSION+SIZE,32)==b'\xa5'*32
        return bytes(self.uc.mem_read(SESSION,SIZE)),self.uc.reg_read(UC_X86_REG_EAX)&255,self.clock_calls
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe',type=Path,default=Path('Halo 2 Vista installer/halo2.exe'))
    p.add_argument('--fixture',type=Path,default=Path('tests/fixtures/vista_session_player_preparation_original.tsv'))
    p.add_argument('--directory',type=Path,default=Path('analysis/frontend/vista-session-flow-audit/player-preparation-original'))
    a=p.parse_args();image=a.exe.read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
    a.directory.mkdir(parents=True,exist_ok=True);o=Oracle(image);records=[]
    lines=['# H2V_PLAYER_PREPARATION_ORIGINAL_V1',
      '# kind label pattern_seed target flag clock AL clock_calls before_patches after_patches; offsets/bytes hex']
    def add(kind,label,seed,before,target=-1,flag=0,clock=0):
        visits=o.visits;after,al,clocks=o.execute(kind,before,target,flag,clock)
        lines.append('\t'.join([kind,label,f'{seed:x}',f'{u(target):x}',f'{flag:x}',f'{u(clock):x}',f'{al:x}',f'{clocks:x}',
          patches(pattern(seed),before),patches(before,after)]))
        records.append({'kind':kind,'label':label,'before_sha256':hashlib.sha256(before).hexdigest(),
          'after_sha256':hashlib.sha256(after).hexdigest(),'al':al,'clock_calls':clocks,'calls':o.calls.copy(),
          'instruction_visits':o.visits-visits})
    for state,present in itertools.product((0,3,4,5,6,7,8,10,0xffffffff),(0,1,0x80,0xff)):
        add('P',f'admission_{len(records):04d}',19,preparation_bank(state=state,present=present))
    for count,active,option,revision in itertools.product((-0x80000000,-1,0,1,2,16,17,18),(0,1,0x8000,0xffff),(0xffff,0),(0,0xffffffff)):
        add('P',f'bank_{len(records):04d}',148,preparation_bank(seed=148,count=count,active=active,option=option,revision=revision))
    for teamflags,team,skipmask in itertools.product((0,1,0x80,0xff),(0xff,0x80,0xfe,0,1,7,8,127),(0,0xaaaa,0xffff)):
        add('P',f'rows_{len(records):04d}',19,preparation_bank(count=18,active=0xffff,teamflags=teamflags,team=team,skipmask=skipmask))
    for peerindex in (0,1,16,17):
        add('P',f'peer_reference_{peerindex}',19,preparation_bank(count=18,active=0xffff,peerindex=peerindex))
    for state in (0,3,4,6,7,8,9,0xffffffff):
        b=preparation_bank(state=state,count=18)
        add('R',f'refresh_state_{state:x}',19,b,target=-1,flag=0x80,clock=0xabcdef01)
    for count,target,flag,host in itertools.product((-0x80000000,-1,0,1,2,17,18,31,32,33,0x7fffffff),
            (-1,0,1,17,31,32,33,-2),(0,1,0x80,0xff),(0,1,17,31,32,0xffffffff)):
        b=preparation_bank(count=count);put(b,0x71c0,host)
        add('R',f'refresh_{len(records):05d}',19,b,target,flag,0xfedcba98)
    a.fixture.write_text('\n'.join(lines)+'\n');md=Cs(CS_ARCH_X86,CS_MODE_32);windows={};asm=[]
    for name,(lo,hi) in WINDOWS.items():
        b=image[lo-BASE:hi-BASE];windows[name]={'begin':hex(lo),'end_exclusive':hex(hi),
          'sha256':hashlib.sha256(b).hexdigest(),'bytes':b.hex()}
        asm.append(f'; {name} {lo:08X}..{hi:08X}')
        asm += [f'{x.address:08X} {x.mnemonic} {x.op_str}' for x in md.disasm(b,lo)]
    (a.directory/'source.asm').write_text('\n'.join(asm)+'\n')
    report={'source_sha256':SHA,'fixture_sha256':hashlib.sha256(a.fixture.read_bytes()).hexdigest(),
      'case_count':len(records),'instruction_visits':o.visits,'windows':windows,'records':records,
      'boundaries':['All source preparation/getter/row/address/setter/refresh/clock/cookie instructions execute unchanged.',
       'Only687BA9 memset and687E18 memcpy are external byte-memory services. Actual clock override bank supplies explicit sample.',
       'Every complete7B88-byte before/after bank is compared through independently captured patch encodings;32-byte suffix preserved.',
       'No player,peer,identity,readiness,transport acknowledgment,native session or game is constructed. Input banks are explicit controls.']}
    (a.directory/'execution.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('case_count','instruction_visits','fixture_sha256')}))
if __name__=='__main__':main()
