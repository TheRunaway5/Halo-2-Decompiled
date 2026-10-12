#!/usr/bin/env python3
"""Execute original Vista PCR headings, active-pane chrome and row voice rules."""
import hashlib
import itertools
import json
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_vista_pcr_screen import ROOT, SHA, BASE, STOP, STACK, OBJ, CHILD, BUFFER, machine, u32, w32, wide, string

HEADINGS = (0x40c1a7,0x40c214,0x40c36e,0x40c423,0x40c490,0x40c4fd)
OFFSETS = (0xad0,0x1e40,0x31b0,0x4520,0x5894,0x6c04)
TITLE=OBJ+0x53c
TABLE=OBJ+0x18000
TEXT=OBJ+0x18100
TITLE_BUFFER=OBJ+0x19000
LOCALE={0x06000763:'\ue101 DONE',0x0d000764:'\ue101 LEAVE PARTY',0x13000906:'\ue100 SHOW GAMERCARD '}

def execute(image,q):
    u=machine(image);events=[];visits=0;output={}
    def ret(value=None,pop=0):
        sp=u.reg_read(UC_X86_REG_ESP)
        if value is not None:u.reg_write(UC_X86_REG_EAX,value&0xffffffff)
        u.reg_write(UC_X86_REG_EIP,u32(u,sp));u.reg_write(UC_X86_REG_ESP,sp+4+pop)
    def hook(_u,pc,size,_):
        nonlocal visits
        visits+=1;sp=u.reg_read(UC_X86_REG_ESP);this=u.reg_read(UC_X86_REG_ECX)
        if pc==STOP:u.emu_stop();return
        if pc==0x6875d3:ret();return
        if pc==0x611e23:events.append(['initialize']);ret();return
        if pc==0x611973:
            index=u32(u,sp+4);events.append(['text_child',index]);ret(CHILD+index*0x100,4);return
        if pc==0x6119f7:events.append(['bitmap_child',u32(u,sp+4)]);ret(CHILD,4);return
        if pc==0x61bf85:
            key=u32(u,sp+4)
            if this==TITLE:events.append(['footer_string',key]);wide(u,TITLE_BUFFER,LOCALE[key]);output['footer']=LOCALE[key]
            else:events.append(['string',(this-CHILD)//0x100,key])
            ret(0,4);return
        if pc in (0x6152b0,0x5ac4a3,0x615ff6,0x616012,0x6156b9):
            name={0x6152b0:'host',0x5ac4a3:'platform',0x615ff6:'class_a',0x616012:'class_b',0x6156b9:'details'}[pc]
            value=q[name];events.append([name,value]);ret(value);return
        if pc==0x40b892:ret(int(u32(u,sp+4)==OBJ+OFFSETS[q['pane']]),4);return
        if pc==0x6101a4:
            key,destination=u32(u,sp+4),u32(u,sp+8);events.append(['localized',key]);wide(u,destination,LOCALE[key]);ret(destination,8);return
        if pc==STOP+0x100:ret(TEXT);return
        if pc==STOP+0x110:ret(TITLE_BUFFER);return
        if pc in (STOP+0x120,0x612cd8):ret();return
        if pc==0x61baa9:
            output['footer']=string(u,u32(u,sp+4));events.append(['footer_text',output['footer']]);ret(0,4);return
        if pc==0x616cfc:
            events.append(['identity',bytes(u.mem_read(u32(u,sp+4),8)).hex(),q['slot']]);ret(q['slot']);return
        if pc in (0x5b4afc,0x5b4aa1):
            name='enabled' if pc==0x5b4afc else 'talking';events.append([name,u32(u,sp+4),q[name]]);ret(q[name]);return
        if pc==0x61c300:events.append(['frame',u32(u,sp+4)]);output['frame']=u32(u,sp+4);ret(0,4);return
    def write(_u,access,address,size,value,_):
        index,offset=divmod(address-CHILD,0x100)
        if offset==0x6e and size==1:events.append(['visible',index,value]);output['visible']=value
    u.hook_add(UC_HOOK_CODE,hook)
    u.hook_add(UC_HOOK_MEM_WRITE,write,begin=CHILD,end=CHILD+0xfff)
    if q['kind']=='headings':
        w32(u,0x86eee4,q['variant']);entry=HEADINGS[q['pane']];this=OBJ;args=[0]
    elif q['kind']=='chrome':
        this=OBJ;args=[];entry=0x40ce6a;w32(u,OBJ,TABLE);w32(u,TABLE+0x60,STOP+0x120)
        w32(u,OBJ+0xacc,OBJ+OFFSETS[q['pane']]);u.mem_write(OBJ+0x7f74,bytes([q['lobby']]))
        w32(u,0x86eee8,q['players']);w32(u,TITLE,TABLE+0x100);w32(u,TABLE+0x14c,STOP+0x100)
        w32(u,TEXT,TABLE+0x200);w32(u,TABLE+0x20c,STOP+0x110)
    else:
        this=OBJ;args=[OBJ+0x1000,q['row']];entry=0x40c5c4 if q['lobby'] else 0x40c5da
        u.mem_write(0x86eef8+q['row']*0x110+0x100,bytes.fromhex('0123456789abcdef'))
        output={'visible':0,'frame':None}
    sp=STACK+0x18000;u.mem_write(sp,struct.pack('<'+'I'*(len(args)+1),STOP,*args));u.reg_write(UC_X86_REG_ESP,sp);u.reg_write(UC_X86_REG_ECX,this)
    u.emu_start(entry,STOP+1,count=100000);assert u.reg_read(UC_X86_REG_EIP)==STOP
    return dict(input=q,events=events,output=output,visits=visits)

def main():
    image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes();assert hashlib.sha256(image).hexdigest()==SHA
    rows=[]
    for pane,variant in itertools.product(range(6),range(11)):
        rows.append(execute(image,dict(kind='headings',pane=pane,variant=variant)))
    for pane,lobby,host,details,players,platform,a,b in itertools.product(range(6),(0,0x80),(0,0x80),(0,0x80),(0,7,0xffffffff),(0,1),(0,1),(0,1)):
        rows.append(execute(image,dict(kind='chrome',pane=pane,lobby=lobby,host=host,details=details,players=players,platform=platform,class_a=a,class_b=b)))
    for lobby,slot,enabled,talking in itertools.product((0,0x80),(0xffffffff,0,13),(0,0x80),(0,0x80)):
        rows.append(execute(image,dict(kind='voice',row=7,lobby=lobby,slot=slot,enabled=enabled,talking=talking)))
    output=ROOT/'analysis/frontend/vista-session-flow-audit/pcr-chrome-original';output.mkdir(parents=True,exist_ok=True)
    fixture=ROOT/'tests/fixtures/vista_pcr_chrome_original.json';fixture.write_text(json.dumps(rows,separators=(',',':'))+'\n')
    m=Cs(CS_ARCH_X86,CS_MODE_32);asm=[];windows={}
    for name,lo,hi in [('headings',0x40c1a7,0x40c5b0),('voice',0x40c5c4,0x40c66a),('chrome',0x40ce6a,0x40d04c),('append',0x44bcd5,0x44bd22)]:
        raw=image[lo-BASE:hi-BASE];windows[name]=dict(begin=hex(lo),end_exclusive=hex(hi),sha256=hashlib.sha256(raw).hexdigest());asm.append('; '+name);asm.extend(f'{i.address:08X} {i.mnemonic} {i.op_str}' for i in m.disasm(raw,lo))
    (output/'source.asm').write_text('\n'.join(asm)+'\n')
    report=dict(source_sha256=SHA,cases=len(rows),visits=sum(x['visits'] for x in rows),fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(),windows=windows,
        boundaries=['Original six heading consumers,active-pane chrome decisions,voice decisions and44BCD5 append execute unchanged.',
        'Widget lookup/string assignment,host/platform/session/details and voice identity/state are explicit services with exact operands. Voice61C300 page assignment remains external; actual assets contain required frames0,1,2.',
        'No live owner result,GPU shader,original framebuffer or complete session publication is manufactured.'])
    (output/'execution.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k] for k in ['cases','visits','fixture_sha256']}))

if __name__=='__main__':main()
