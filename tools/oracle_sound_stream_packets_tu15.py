#!/usr/bin/env python3
"""Observe unchanged TU1.5 stock stream packet/cache helpers on raw ABI inputs.

No source voice, cache admission, I/O completion or device result is supplied.
The private packet/cache/allocator records are declared component inputs only.
"""
import argparse,collections,hashlib,json,os,random,struct,sys
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
STREAM,CHUNK,PACKET,CACHE,ENTRIES,ALLOCATOR,BLOCK_OWNER,BLOCKS=[0x4000000+i*0x100000 for i in range(8)]
STACK,STOP=0x2008000,0x3000000
REGS=[UC_X86_REG_EAX,UC_X86_REG_ECX,UC_X86_REG_EDX,UC_X86_REG_EBX,UC_X86_REG_ESP,UC_X86_REG_EBP,UC_X86_REG_ESI,UC_X86_REG_EDI]
ENTRIES_PC=[0x21f360,0x2b98d0,0x2b9950]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.absolute()
    if not args.output.is_relative_to(owner):raise ValueError('Fixture must stay in its disposable owner')
    args.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;code=_Code(image,'.text');rng=random.Random(0x2b98d0)
    specs=[]
    for reference in [0,1,254,255]:
        for shift in [0,1,4,31,32,33,63,0xffffffff]:
            for handle in [0,1,7]:specs.append((0x21f360,0,36,0,reference,shift,handle,None))
    for n in range(256):specs.append((0x21f360,0,36,0,rng.randrange(256),rng.getrandbits(32),rng.randrange(8),None))
    sizes=[0,1,35,36,71,72,73,32760,63000,0x3fffffff,0x40000024,0x80000048,0xffffffff]
    cursors=[0,1,35,36,71,72,73,32760,0x7fffffff,0x80000000,0xffffffff]
    for flags in range(256):
        for n in range(3):specs.append((0x2b98d0,flags,sizes[(flags+n)%len(sizes)],cursors[(flags*3+n)%len(cursors)],flags,flags%35,flags%8,None))
    for size in sizes:
        for cursor in cursors:specs.append((0x2b98d0,0x80,size,cursor,255,4,1,None))
    for flags in range(256):specs.append((0x2b9950,flags,36,rng.getrandbits(32),0,0,0,None))
    for entry in ENTRIES_PC:
        for fault in ['stream','chunk','cache','blocks','packet']:
            if entry==0x21f360 and fault in ['stream','packet']:continue
            if entry==0x2b9950 and fault!='stream':continue
            specs.append((entry,0,36,0,3,0,0,fault))
    specs.extend([(0x21f360,0,36,0,0,0,0xffff,None),(0x2b98d0,0,36,0,0,0,0xabcdffff,None)])
    all_sites=collections.Counter();counts=collections.Counter();returns=faults=0
    fixture=args.output/'cases.jsonl';full_hash=hashlib.sha256()
    header={'original_xbe_sha256':NEW_PIN,'raw_abi_only':True,'platform_results_supplied':[],
      'regions':[[0x50e000,0x1000],[0x2000000,0x10000]]+[[p,0x1000]for p in [STREAM,CHUNK,PACKET,CACHE,ENTRIES,ALLOCATOR,BLOCK_OWNER,BLOCKS]]}
    with fixture.open('wb')as output:
        line=(json.dumps(header,separators=(',',':'))+'\n').encode();output.write(line);full_hash.update(line)
        for ordinal,(entry,flags,size,cursor,reference,shift,handle,fault)in enumerate(specs):
            if ordinal%2:handle|=rng.getrandbits(16)<<16
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
            for base,extent in header['regions'][1:]:g.mem_map(base,extent)
            g.mem_map(STOP,0x1000);fields=[]
            def w(a,n,value):
                value&=(1<<(n*8))-1;g.mem_write(a,value.to_bytes(n,'little'));fields.append([a,n,value])
            def u(a,n):return int.from_bytes(g.mem_read(a,n),'little')
            for a,v in [(0x50ebdc,CACHE),(0x50ebe0,0x81234567),(0x50ebe4,ALLOCATOR),(CACHE+0x44,ENTRIES),
                (ALLOCATOR+0x64,BLOCK_OWNER),(ALLOCATOR+0x34,shift),(BLOCK_OWNER+0x44,BLOCKS),(CHUNK+8,handle),(CHUNK+4,size)]:
                w(a,4,v)
            if handle&65535<8:
                w(ENTRIES+(handle&65535)*16+5,1,reference);w(BLOCKS+(handle&65535)*24+8,4,rng.getrandbits(32))
            for n in range(-4,4):w(STREAM+0x1c+n*4,4,CHUNK)
            w(STREAM+2,1,flags);w(STREAM+0x2c,4,cursor)
            for n in range(6):w(PACKET+n*4,4,rng.getrandbits(32))
            w(STACK,4,STOP);w(STACK+4,4,STREAM);w(STACK+8,4,PACKET)
            gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK
            if entry==0x21f360:gp[2]=CHUNK
            if fault=='stream':w(STACK+4,4,0)
            elif fault=='chunk':
                if entry==0x21f360:gp[2]=0
                else:w(STREAM+0x1c,4,0)
            elif fault=='cache':w(0x50ebdc,4,0)
            elif fault=='blocks':w(BLOCK_OWNER+0x44,4,0)
            elif fault=='packet':w(STACK+8,4,0)
            for reg,v in zip(REGS,gp):g.reg_write(reg,v)
            xmm=[rng.getrandbits(128)for _ in range(8)]
            for n,v in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,v)
            cw=[0x027f,0x037f,0x077f,0x0b7f,0x0f7f][ordinal%5];mx=[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5]
            g.reg_write(UC_X86_REG_FPCW,cw);g.reg_write(UC_X86_REG_MXCSR,mx);g.reg_write(UC_X86_REG_FPSW,(ordinal%8)<<11)
            fp=[g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]
            stores=[];bad=[];sites=set()
            def instruction(_g,a,n,_):
                assert image.read(a,n)==bytes(g.mem_read(a,n));sites.add(a)
            def write(_g,kind,a,n,value,_):
                if 0x2000000<=a<0x2010000:return
                stores.append([g.reg_read(UC_X86_REG_EIP),a,n,u(a,n),value&((1<<(8*n))-1)])
            def invalid(_g,kind,a,n,value,_):bad.append([a,n]);return False
            g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
            try:g.emu_start(entry,STOP,count=2000)
            except UcError:
                if not bad:raise
            returned=not bad
            if returned:assert g.reg_read(UC_X86_REG_EIP)==STOP;returns+=1
            else:faults+=1
            assert [g.reg_read(UC_X86_REG_XMM0+n)for n in range(8)]==xmm
            assert [g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]==fp
            record={'entry':entry,'fields':fields,'gp':gp,'out_gp':[g.reg_read(reg)for reg in REGS]if returned else None,
                'stores':stores,'fault':bad[0]if bad else None,'xmm':[[v>>(32*k)&0xffffffff for k in range(4)]for v in xmm],
                'fp':fp,'case':ordinal}
            line=(json.dumps(record,separators=(',',':'))+'\n').encode();output.write(line);full_hash.update(line)
            counts[f'{entry:08x}']+=1;all_sites.update(sites)
    report={'passed':True,'scope':__doc__,'original_xbe_sha256':NEW_PIN,'cases':len(specs),'returns':returns,'faults':faults,
      'entries':dict(counts),'instruction_sites':len(all_sites),'executed_instructions_lower_bound':sum(all_sites.values()),
      'fixture':str(fixture),'fixture_bytes':fixture.stat().st_size,'fixture_sha256':full_hash.hexdigest(),'instruction_patches':[],
      'source_world_voice_or_device_admission':False,'callee_or_platform_results_supplied':[]}
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()

