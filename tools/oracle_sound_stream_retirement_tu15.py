#!/usr/bin/env python3
"""Observe unchanged TU1.5 chunk retirement, including its real CRT copy.

Records are raw ABI inputs, including arbitrary callback status arguments.
No voice/cache admission, completed device packet or callee result is supplied.
"""
import argparse,collections,hashlib,json,os,random
from pathlib import Path
from oracle_sound_stream_packets_tu15 import Xbe,NEW_DEFAULT,NEW_PIN,STREAM,CHUNK,CACHE,ENTRIES,STACK,STOP,REGS
from unicorn import Uc,UcError,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE,UC_HOOK_MEM_INVALID
from unicorn.x86_const import *
ENTRY_PCS=[0x2b82a0,0x2b99c0]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);args.output=args.output.resolve()
    if not args.output.is_relative_to(owner):raise ValueError('Fixture must stay in its disposable owner')
    args.output.mkdir(parents=True,exist_ok=False)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN;rng=random.Random(0x2b82a0)
    specs=[]
    for entry in ENTRY_PCS:
        for alignment in range(4):
            for queued in [1,2,3]:
                for submitted in range(8):
                    for high in [0,0x40,0x80,0xc0]:
                        specs.append((entry,alignment,high|submitted*8|queued,None))
        for flags in range(256):specs.append((entry,0,flags,'handle' if flags&7 not in [1,2,3] else None))
        for fault in ['stream','chunk','cache','entries','handle','alias']:
            specs.append((entry,0,0xc2,fault))
        for queued in [0,4,5,6,7]:specs.append((entry,0,0x10|queued,None))
    header={'original_xbe_sha256':NEW_PIN,'raw_abi_only':True,'direction_flag_clear':True,'platform_results_supplied':[],
        'required_entries':ENTRY_PCS,'minimum_cases':1200,'source_stack_stores_observed':True,
        'regions':[[0x50e000,4096],[0x2000000,0x10000]]+[[a,4096]for a in [STREAM,CHUNK,CACHE,ENTRIES]]}
    counts=collections.Counter();sites=collections.Counter();statuses=collections.Counter();returns=faults=stores_total=0
    fixture=args.output/'cases.jsonl';digest=hashlib.sha256()
    with fixture.open('wb')as output:
        def emit(row):
            line=(json.dumps(row,separators=(',',':'))+'\n').encode();output.write(line);digest.update(line)
        emit(header)
        for ordinal,(entry,alignment,flags,fault)in enumerate(specs):
            stream=STREAM+alignment;chunk=CHUNK;status=[0,0x80004004,0x80004005,0xffffffff][ordinal%4]
            g=Uc(UC_ARCH_X86,UC_MODE_32);g.mem_map(0x10000,0x800000)
            for section in image.sections:g.mem_write(section.va,image.section_bytes(section))
            text=image.section('.text');g.mem_protect(text.va&~4095,((text.va%4096+text.vsize+4095)//4096)*4096,5)
            for a,n in header['regions'][1:]:g.mem_map(a,n)
            g.mem_map(STOP,4096);fields=[]
            def w(a,n,v):
                v&=(1<<(n*8))-1;g.mem_write(a,v.to_bytes(n,'little'));fields.append([a,n,v])
            def u(a,n):return int.from_bytes(g.mem_read(a,n),'little')
            for a,v in [(0x50ebdc,CACHE),(CACHE+0x44,ENTRIES),(CHUNK+8,(rng.getrandbits(16)<<16)),
                        (CHUNK+0x108,(rng.getrandbits(16)<<16)|1),
                        (stream+0x1c,CHUNK),(stream+0x20,CHUNK+0x100),(stream+0x24,0x9753abcd),
                        (stream+0x2c,rng.getrandbits(32))]:w(a,4,v)
            for index in [0,1]:
                w(ENTRIES+index*16+4,1,[0,1,2,254,255][(ordinal+index)%5])
                w(ENTRIES+index*16+5,1,[0,1,2,254,255][(ordinal*3+index)%5])
            w(stream+1,1,ordinal%256);w(stream+2,1,flags)
            if fault=='stream':stream=0
            elif fault=='chunk':chunk=0
            elif fault=='cache':w(0x50ebdc,4,0)
            elif fault=='entries':w(CACHE+0x44,4,0)
            elif fault=='handle':w(CHUNK+8,4,0xffffffff)
            elif fault=='alias':
                chunk=stream+0x14;w(stream+0x1c,4,0x12340000);w(stream+0x20,4,0xabcd0001)
            w(STACK,4,STOP);w(STACK+4,4,stream);w(STACK+8,4,chunk);w(STACK+12,4,status)
            gp=[rng.getrandbits(32)for _ in REGS];gp[4]=STACK
            if entry==0x2b82a0:gp[6]=stream;gp[2]=chunk
            for reg,v in zip(REGS,gp):g.reg_write(reg,v)
            g.reg_write(UC_X86_REG_EFLAGS,0x202)
            xmm=[rng.getrandbits(128)for _ in range(8)]
            for n,v in enumerate(xmm):g.reg_write(UC_X86_REG_XMM0+n,v)
            cw=[0x027f,0x037f,0x077f,0x0b7f,0x0f7f][ordinal%5];mx=[0x1f80,0x3f80,0x5f80,0x7f80,0x9fc0][ordinal%5]
            g.reg_write(UC_X86_REG_FPCW,cw);g.reg_write(UC_X86_REG_MXCSR,mx);g.reg_write(UC_X86_REG_FPSW,(ordinal%8)<<11)
            fp=[g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]
            stores=[];stack_stores=[];bad=[];executed=set()
            def instruction(_g,a,n,_):
                assert image.read(a,n)==bytes(g.mem_read(a,n));executed.add(a)
            def write(_g,kind,a,n,v,_):
                try:before=u(a,n)
                except UcError:return
                if 0x2000000<=a<0x2010000:
                    stack_stores.append([a,n,before,v&((1<<(8*n))-1)]);return
                stores.append([g.reg_read(UC_X86_REG_EIP),a,n,before,v&((1<<(8*n))-1)])
            def invalid(_g,kind,a,n,v,_):bad.append([a,n]);return False
            g.hook_add(UC_HOOK_CODE,instruction);g.hook_add(UC_HOOK_MEM_WRITE,write);g.hook_add(UC_HOOK_MEM_INVALID,invalid)
            try:g.emu_start(entry,STOP,count=20000)
            except UcError:
                if not bad:raise
            returned=not bad
            if returned:assert g.reg_read(UC_X86_REG_EIP)==STOP;returns+=1
            else:faults+=1
            assert [g.reg_read(UC_X86_REG_XMM0+n)for n in range(8)]==xmm
            assert [g.reg_read(reg)for reg in [UC_X86_REG_FPCW,UC_X86_REG_MXCSR,UC_X86_REG_FPSW]]==fp
            emit({'entry':entry,'fields':fields,'gp':gp,'out_gp':[g.reg_read(reg)for reg in REGS]if returned else None,
                'stores':stores,'source_stack_stores':stack_stores,'fault':bad[0]if bad else None,'xmm':[[v>>(32*k)&0xffffffff for k in range(4)]for v in xmm],
                'fp':fp,'case':ordinal,'input_status':status,'input_alignment':alignment,'input_flags':flags,'input_fault':fault})
            counts[f'{entry:08x}']+=1;sites.update(executed);stores_total+=len(stores)
            if entry==0x2b99c0:statuses[f'{status:08x}']+=1
    report={'passed':True,'scope':__doc__,'original_xbe_sha256':NEW_PIN,'cases':len(specs),'returns':returns,'faults':faults,
        'entries':dict(counts),'callback_status_inputs':dict(statuses),'ordered_stores':stores_total,
        'instruction_sites':len(sites),'executed_instructions_lower_bound':sum(sites.values()),
        'fixture':str(fixture),'fixture_bytes':fixture.stat().st_size,'fixture_sha256':digest.hexdigest(),
        'source_stack_stores_observed':True,'instruction_patches':[],'callee_or_platform_results_supplied':[],'native_world_voice_or_device_admission':False,
        'multi_chunk_loop_playback_verified':False}
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
if __name__=='__main__':main()
