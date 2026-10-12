#!/usr/bin/env python3
"""Observe unchanged retail cluster MOPP callbacks using authored map bytes.

Reads original caches/XBE only. Read-only hooks observe callback arguments,
returns, cluster entry and opcode dispatch; no execution service is replaced.
Cold breakable bits and static-only source query flags are explicit inputs.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX, UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
sys.path.insert(0,str(Path(__file__).resolve().parent))
from inspect_multiplayer import Cache

ROOT=Path(__file__).resolve().parents[1]
XBE=ROOT/'assets/disc/default.xbe'
image=XBE.read_bytes()
assert hashlib.sha256(image).hexdigest()=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
u=Uc(UC_ARCH_X86,UC_MODE_32)
u.mem_map(0,0x601000)
u.reg_write(UC_X86_REG_FPCW,0x027f)
u.reg_write(UC_X86_REG_MXCSR,0x1f80)
base=struct.unpack_from('<I',image,0x104)[0]
count,table=struct.unpack_from('<II',image,0x11c)
for i in range(count):
    _,va,_,off,size=struct.unpack_from('<5I',image,table-base+i*56)
    u.mem_write(va,image[off:off+size])
u.mem_map(0x2000000,0x20000)
u.mem_map(0x3000000,0x1000)
def put(at,*words):u.mem_write(at,struct.pack('<'+'I'*len(words),*words))
def word(at):return struct.unpack('<I',bytes(u.mem_read(at,4)))[0]
put(0x4ed280,0x2010000)
u.mem_write(0x4686c4,b'\0\0')
u.mem_write(0x2010000,bytes([255])*0x1000)
origin,ray,result=0x2000100,0x2000200,0x2000300
seen=[]
opcodes=[]
current_cluster=None
pending_context=None
def observe(uc,address,size,unused):
    global pending_context
    context=uc.reg_read(UC_X86_REG_ECX)
    pending_context=context
    shape=word(uc.reg_read(UC_X86_REG_ESP)+4)
    key=word(shape+4)
    seen.append({'key':key,'instance':struct.unpack('<h',struct.pack('<H',key&65535))[0],
        'collector_fraction_bits':word(context+4),'previous_owner':word(result+0x3c),
        'previous_type':word(result),'cluster':current_cluster})
u.hook_add(UC_HOOK_CODE,observe,begin=0x244c10,end=0x244c10)
def observe_return(uc,address,size,unused):
    seen[-1]['returned_fraction_bits']=word(pending_context+4)
u.hook_add(UC_HOOK_CODE,observe_return,begin=0x244c06,end=0x244c06)
def observe_cluster(uc,address,size,unused):
    global current_cluster
    current_cluster=uc.reg_read(UC_X86_REG_EAX)
u.hook_add(UC_HOOK_CODE,observe_cluster,begin=0x244980,end=0x244980)
def observe_opcode(uc,address,size,unused):
    code=uc.reg_read(UC_X86_REG_ESI)
    mopp=word(word(0x4e0348)+0xa0)+current_cluster*176
    mopp=word(mopp+0xac)
    opcodes.append({'cluster':current_cluster,'byte_offset':code-mopp-0x30,
                    'opcode':bytes(uc.mem_read(code,1))[0]})
u.hook_add(UC_HOOK_CODE,observe_opcode,begin=0x300a10,end=0x300a10)
rows=[]
cache=None
name=None
for line in (ROOT/'analysis/multiplayer-shading/instance-tie-census.tsv').read_text().splitlines():
    f=line.split()
    if name!=f[1]:
        if cache:
            u.mem_unmap(memory,allocated);cache.data.close();cache.stream.close()
        name=f[1]
        path=ROOT/'assets/disc/maps'/(name+'.map')
        if not path.exists():path=ROOT/'assets/map-pack/maps'/(name+'.map')
        cache=Cache(path)
        entry=cache.block(cache.scenario+0x210,0x44)[0]
        start,size,memory=cache.u(entry),cache.u(entry+4),cache.u(entry+8)
        allocated=(size+4095)//4096*4096
        u.mem_map(memory,allocated)
        u.mem_write(memory,bytes(cache.data[start:start+size]))
        put(0x4e0348,memory+16)
        # Source11C1B0 supplies the authored main collision block pointer.
        put(0x4e0340,cache.u(start+16+0x18))
        magic=memory-start
        clusters=cache.u(start+16+0xa0)-magic
    p=list(map(float,f[5:8]));d=list(map(float,f[8:11]))
    u.mem_write(origin,struct.pack('<3f',*p));u.mem_write(ray,struct.pack('<3f',*d))
    cluster=54 if name=='headlong' else 7
    cluster_at=clusters+cluster*176
    mopp_file=cache.u(cluster_at+0xac)-magic
    def execute(address,cluster=None):
        seen.clear();opcodes.clear()
        u.mem_write(result,bytes(96));put(result+4,0x3f800000)
        put(0x4e7c1c,1);u.mem_write(0x4e7c20,bytes(512*4))
        put(0x4e7414,1);u.mem_write(0x4e7418,bytes(512*4))
        stack=0x200f000
        if cluster is None:
            put(stack,0x3000000,0x04800005,origin,ray,0xffffffff,0xffffffff,result)
        else:
            u.reg_write(UC_X86_REG_EAX,cluster)
            u.reg_write(UC_X86_REG_ESI,origin);u.reg_write(UC_X86_REG_EDI,ray)
            put(stack,0x3000000,0x04800005,9,result)
        u.reg_write(UC_X86_REG_ESP,stack)
        u.emu_start(address,0x3000000,count=5000000)
        assert u.reg_read(UC_X86_REG_EIP)==0x3000000,hex(u.reg_read(UC_X86_REG_EIP))
        raw=bytes(u.mem_read(result,96))
        return {'hit':u.reg_read(UC_X86_REG_EAX)&255,'result_words':list(struct.unpack('<24I',raw)),
                'callbacks':list(seen),'opcodes':list(opcodes)}
    rows.append({'map':name,'input_source':int(f[3]),'face':int(f[4]),'origin':p,'ray':d,
        'expected_fraction_bits':int(f[11]),'tied_owners':[int(s.split(':')[0]) for s in f[14:]],
        'cluster':cluster,'mopp_file_offset':mopp_file,'mopp_info_words':list(struct.unpack_from('<12I',cache.data,mopp_file)),
        'raw_cluster':execute(0x244980,cluster),'raw_static_query':execute(0x1697c0)})
    print(name,f[3],f[4],rows[-1]['raw_cluster']['result_words'][15],rows[-1]['raw_static_query']['result_words'][15],flush=True)
report={'xbe_sha256':hashlib.sha256(image).hexdigest(),
    'source':'unchanged244980/2DF760/2DF720/301800/3009E0/244BB0/244C10/1691A0 and complete1697C0 static query',
    'execution_hooks':'read-only244C10 callback argument,244C06 return,244980 cluster entry and300A10 opcode observers; no replacement services',
    'floating_point_control':{'fpcw':0x027f,'mxcsr':0x1f80},
    'inputs':'explicit source static-only flags04800005,testflags9,cold all-one breakable bank, authored cache bytes unchanged',
    'rows':rows}
(ROOT/'analysis/multiplayer-shading/instance-accelerator-original.json').write_text(json.dumps(report,indent=2)+'\n')
fixture=ROOT/'tests/fixtures/retail_instance_accelerator.tsv'
bank_lines=[]
for bank_name,cluster in [('headlong',54),('triplicate',7)]:
    path=ROOT/('assets/disc/maps' if bank_name=='headlong' else 'assets/map-pack/maps')/(bank_name+'.map')
    c=Cache(path);entry=c.block(c.scenario+0x210,0x44)[0]
    start,memory=c.u(entry),c.u(entry+8);magic=memory-start
    clusters=c.u(start+16+0xa0)-magic;at=clusters+cluster*176
    capacity=c.u(at+0xa8);off=c.u(at+0xac)-magic
    bank_lines.append(f'B {bank_name} {capacity} '+bytes(c.data[off:off+0x30+capacity]).hex())
    c.data.close();c.stream.close()
query_lines=[]
bits=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
for index,row in enumerate(rows):
    trace=row['raw_cluster'];callbacks=trace['callbacks'];instructions=trace['opcodes']
    flat=[*map(bits,row['origin']),*map(bits,row['ray']),0x3f800000,len(callbacks),
        *[v for c in callbacks for v in (c['key'],c['collector_fraction_bits'],c['returned_fraction_bits'])],
        len(instructions),*[v for i in instructions for v in (i['byte_offset'],i['opcode'])],trace['result_words'][1]]
    query_lines.append(f'Q {row["map"]}_{index} {row["map"]} '+' '.join(f'{v:x}' for v in flat))
fixture.write_text('# Literal original MoppInfo/callback/opcode fixture. B name capacity raw; Q bank input6bits limit callbackN(key,before,after) instructionsN(pc,op) final.\n'+'\n'.join(bank_lines+query_lines)+'\n')
fixture.with_suffix('.provenance.json').write_text(json.dumps({
    'xbe_sha256':report['xbe_sha256'],'source':report['source'],
    'hooks':report['execution_hooks'],'fpcw':'027F','mxcsr':'1F80',
    'inputs':report['inputs'],'queries':len(rows),
    'fixture_sha256':hashlib.sha256(fixture.read_bytes()).hexdigest(),
    'boundary':'Authored static instance ties; actual MOPP visitation recovered from cache and unchanged source. No dynamic registration/session or pixel parity claim.'},indent=2)+'\n')
print('completed',len(rows),flush=True)
