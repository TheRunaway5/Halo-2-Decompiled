#!/usr/bin/env python3
"""Actual supplied PC PCR identity, placement and CRT decimal consumers.

All session globals and records are explicit caller-owned inputs. Only the
selected-variant pointer, CRT per-thread owner, and locale lookup boundary are
provided; no identity predicate or formatted result is substituted.
"""
from pathlib import Path
import hashlib,itertools,json,struct
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX,UC_X86_REG_EIP,UC_X86_REG_ESP
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from oracle_vista_pcr_screen import ROOT,SHA,STOP,STACK,OBJ,machine,u32,w32,string,wide

OUT=ROOT/'analysis/frontend/vista-session-flow-audit/native-pcr-original'
N=0x7b88
IDENTITY=OBJ+0x9000
FORMAT=OBJ+0xa000
DEST=OBJ+0xb000
TLS=OBJ+0x19000

def initial(q):
    bank=bytearray((i*37+q['seed']*13+(i>>8)*17)&255 for i in range(N))
    for at,value in ((0x73a0,q['state']),(0x1258,q['mask']),(0x71c0,q['peer']),
                     (0x64,q['peer']^0x1234)):
        struct.pack_into('<I',bank,at,value&0xffffffff)
    for index in range(16):
        at=0x125c+index*0x128
        bank[at:at+8]=struct.pack('<II',0x12345678+index,0xaabbccdd)
        struct.pack_into('<I',bank,at+8,(index%3 if q['matching_peer'] else 99))
    # Deliberate duplicate confirms first occupied matching XUID semantics.
    if q['duplicate']:
        bank[0x125c+15*0x128:0x1264+15*0x128]=bank[0x125c:0x1264]
    return bytes(bank)

def execute(image,q):
    u=machine(image);u.mem_write(OBJ,initial(q))
    w32(u,0x820fc0,q['active']);w32(u,0x820fc4,q['kind']);w32(u,0x820fe8,OBJ)
    identity=struct.pack('<II',0x12345678+q['identity_index'],0xaabbccdd)
    u.mem_write(IDENTITY,identity);visits=0;calls=[];cookie=0
    def ret(value):
        stack=u.reg_read(UC_X86_REG_ESP)
        u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        u.reg_write(UC_X86_REG_EIP,u32(u,stack));u.reg_write(UC_X86_REG_ESP,stack+4)
    def hook(_u,pc,_size,_):
        nonlocal visits,cookie
        visits+=1
        if pc==STOP:u.emu_stop();return
        if pc==0x45b171:ret(OBJ+0x10000);return
        if pc==0x6908fe:ret(TLS);return
        if pc==0x46b601:
            calls.append(['localized',u32(u,u.reg_read(UC_X86_REG_ESP)+4)])
            ret(DEST);return
        if pc==0x6875d3:cookie+=1
    u.hook_add(UC_HOOK_CODE,hook)
    op=q['op']
    if op=='identity':entry,args=0x6156cd,[IDENTITY]
    elif op=='primary':entry,args=0x616d34,[IDENTITY]
    elif op=='player':entry,args=0x615820,[q['datum']]
    elif op=='placement':entry,args=0x46c675,[q['value']&0xffffffff,DEST]
    else:
        w32(u,OBJ+0x10044,2)
        # Actual source static C-locale records retained by an explicit CRT
        # thread owner. The formatter and all integer conversion run unchanged.
        w32(u,TLS+0x68,u32(u,0x8694a0));w32(u,TLS+0x6c,u32(u,0x869680))
        if op=='score':entry,args=0x470182,[q['values'][0]&0xffffffff,DEST]
        else:
            wide(u,FORMAT,q['format'])
            entry,args=0x40c85b,[DEST,FORMAT]+[v&0xffffffff for v in q['values']]
    esp=STACK+0x18000
    u.mem_write(esp,struct.pack('<'+'I'*(1+len(args)),STOP,*args));u.reg_write(UC_X86_REG_ESP,esp)
    u.emu_start(entry,STOP+1,count=100000)
    assert u.reg_read(UC_X86_REG_EIP)==STOP
    value=u.reg_read(UC_X86_REG_EAX)
    if op in ('identity','player'):value&=255
    if op in ('score','format'):value=string(u,DEST)
    if op=='placement':value=calls[0][1]
    assert bytes(u.mem_read(OBJ,N))==initial(q),'query mutated actual session storage'
    return dict(input=q,returned=value,calls=calls,visits=visits,cookie=cookie)

def main():
    image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes()
    assert hashlib.sha256(image).hexdigest()==SHA
    rows=[]
    def add(**kw):
        q=dict(op='identity',seed=17,state=5,mask=0xffff,peer=0,matching_peer=1,
               active=1,kind=1,identity_index=0,duplicate=0,datum=0,value=0,values=[],format='')
        q.update(kw);rows.append(execute(image,q))
    for op,state,active,kind,index in itertools.product(('identity','primary'),
            (*range(12),0xffffffff),(0,1,0x80),(0,1,5,6,0xffffffff),(0,7,15,16)):
        add(op=op,state=state,active=active,kind=kind,identity_index=index)
    for op,mask,index,duplicate in itertools.product(('identity','primary'),
            (0,1,0x8000,0x8001,0xffff),(0,7,15,16),(0,1)):
        add(op=op,mask=mask,identity_index=index,duplicate=duplicate)
    for state,active,mask,peer,matching,datum in itertools.product((0,3,8,9),
            (0,1),(0,1,2,0x8000,0x80000000,0xffffffff),(0,1),(0,1),
            (0,1,15,16,31,0x12340001,0x10000)):
        add(op='player',state=state,active=active,mask=mask,peer=peer,matching_peer=matching,datum=datum)
    for value in (-0x80000000,-32768,-1,*range(18),0x7fffffff):add(op='placement',value=value)
    for value in (-0x80000000,-2147483647,-32768,-100,-1,0,1,9,10,32767,2147483647):
        add(op='score',values=[value])
        for fmt in ('%d','%02d','%012d','%12d','value=%d%%'):
            add(op='format',format=fmt,values=[value])
    for first,second in itertools.product((-61,-1,0,1,61),(-59,-1,0,1,59)):
        add(op='format',format='%d:%02d',values=[first,second])
    OUT.mkdir(parents=True,exist_ok=True)
    fixture=ROOT/'tests/fixtures/native_vista_pcr_original.json'
    fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
    windows={'identity':(0x5b176b,0x5b181f),'primary':(0x616c85,0x616d6c),
             'eligible':(0x615820,0x615884),'session_records':(0x5c20c8,0x5c20eb),
             'current_session':(0x5ad736,0x5ad7b7),'member_guard':(0x5ad423,0x5ad457),
             'player_bank_query':(0x5b1a37,0x5b1b0f),'placement':(0x46c675,0x46c711),
             'score':(0x470182,0x4701b6),'format_wrapper':(0x40c85b,0x40c87c),
             'format_owner':(0x44bd7b,0x44bd9d),'crt_format':(0x688e32,0x688e51)}
    c=Cs(CS_ARCH_X86,CS_MODE_32);asm=[]
    for name,(lo,hi) in windows.items():
        asm.append('; '+name);asm.extend(f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in c.disasm(image[lo-0x400000:hi-0x400000],lo))
    (OUT/'source.asm').write_text('\n'.join(asm)+'\n')
    report=dict(source_sha256=SHA,cases=len(rows),visits=sum(r['visits'] for r in rows),
                fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),
                source_windows={name:dict(begin=hex(lo),end_exclusive=hex(hi),sha256=hashlib.sha256(image[lo-0x400000:hi-0x400000]).hexdigest()) for name,(lo,hi) in windows.items()},
                boundaries=['All actual current-session/lifecycle/XUID scans and peer predicates execute unchanged; no query result is controlled.',
                            'Original470182/40C85B/44BD7B and CRT688E32 integer formatting execute unchanged with actual static C locale and an explicit CRT per-thread owner.',
                            'Only selected-variant pointer, CRT thread owner and placement locale lookup are controlled service boundaries.',
                            'This does not establish world lifecycle completion, achievement persistence, full retained metadata or GPU rendering.'])
    (OUT/'execution.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('cases','visits','fixture_sha256')}))

if __name__=='__main__':main()
