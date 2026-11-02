#!/usr/bin/env python3
"""Physical unchanged-i386 reference for native renderer script flag effects.

The ScriptVm owns evaluation/completion. Compare the completed2AE380 side
effect and wholeBE180 memory behavior, including raw BOOLEAN bytes and NONE.
All ELF files, observer programs and raw banks belong to run_test.py scratch.
"""
import argparse
import gzip
import hashlib
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
PIN='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
DATA,ARG,BANK,POOL,RECORDS,BODY=0x1000000,0x1003000,0x1001000,0x1001200,0x1001800,0x1002000
SP,STACK,STOP=0x62000800,0x62000000,0x63000000


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def digest(value):return hashlib.sha256(json.dumps(value,sort_keys=True,separators=(',',':')).encode()).hexdigest()


def inputs():
    parameters=[dict(kind='warthog',value=n,object=0,flags=0xdeadbeef)for n in range(256)]
    parameters +=[dict(kind='visibility',value=value,object=obj,flags=flags)for value,obj,flags in itertools.product(
        (0,1,2,0x80,0xff),(0,0xbeef0000,0x12340001,0x99990001,0xffffffff),
        (0,0xffffffff,0x200000,0xffdfffff,0xdeadbeef,0xa5a5a5a5))]
    for value in (0,1,0x80,0xff):parameters.append(dict(kind='warthog',value=value,object=0,flags=0,alias=True))
    rows=[]
    for n,p in enumerate(parameters):
        banks=[dict(address=0x4ea000,before=bytearray((i*13+0x57)&255 for i in range(4096))),
            dict(address=0x4ec000,before=bytearray((i*7+0x31)&255 for i in range(4096))),
            dict(address=DATA,before=bytearray((i*17+0x63)&255 for i in range(0x4000))),dict(address=STACK,before=bytearray(4096))]
        def put(address,value):
            for b in banks:
                if b['address']<=address and address+4<=b['address']+len(b['before']):struct.pack_into('<I',b['before'],address-b['address'],value);return
            raise ValueError('Unmapped declared input')
        put(0x4eabc4,0x4eabc4-0x82 if p.get('alias')else BANK);put(0x4ecbd0,POOL);put(POOL+0x44,RECORDS)
        put(RECORDS+8,BODY);put(RECORDS+20,BODY+0x100);put(BODY+4,p['flags']);put(BODY+0x104,p['flags']^0x1357ace1)
        put(ARG,p['value']);stop=0x2ae3bc if p['kind']=='warthog'else STOP
        put(SP,stop);put(SP+4,p['value']);put(SP+8,0)
        rows.append(dict(id=n,name=p['kind']+'-'+str(n),parameters=p,kind=p['kind'],value=p['value'],object=p['object'],
            entry=0x2ae3a9 if p['kind']=='warthog'else 0xbe180,stop_pc=stop,
            gp=[ARG if p['kind']=='warthog'else p['object'],0x22222222,0x33333333,0x44444444,SP,0x66666666,0x77777777,0x88888888],
            matrix=p['value'],source=0,before_fp=[0x27f,0x4000,0x9fc0],live_x87=[],
            banks=[dict(address=b['address'],before=b['before'].hex())for b in banks]))
    return rows


def prepare(image,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0xbe180,0x2ae380);metadata=common.metadata(image);metadata['stop']=STOP
    metadata['sites']=list({s['instruction']:s for s in metadata['sites']}.values())
    directory=scratch/'physical-original';directory.mkdir();section=image.section('.text')
    (directory/'text.bin').write_bytes(image.section_bytes(section));(directory/'metadata.json').write_text(json.dumps(metadata))
    sections={'globals':(0x4ea000,4096),'poolglobal':(0x4ec000,4096),'data':(DATA,0x4000),'stack':(STACK,4096)}
    assembly=f'.code32\n.section .original,"ax"\n.incbin "{directory/"text.bin"}"\n'
    for name,(_,size)in sections.items():assembly+=f'.section .{name},"aw"\n.space {size}\n'
    assembly+='.section .stop,"ax"\nint3\n.section .bridge,"ax"\n.globl _start\n_start:\nfninit\nint3\njmp _start\n'
    (directory/'reference.s').write_text(assembly)
    linker=f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} '
    linker+=' '.join(f'.{name} 0x{address:x}:{{*(.{name})}}'for name,(address,_)in sections.items())
    linker+=f' .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n';(directory/'link.ld').write_text(linker)
    for command in (['as','--32',str(directory/'reference.s'),'-o',str(directory/'reference.o')],
        ['ld','-melf_i386','-T',str(directory/'link.ld'),str(directory/'reference.o'),'-o',str(directory/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(directory/'observer')]):
        r=subprocess.run(command,text=True,capture_output=True)
        if r.returncode:raise ValueError('Original reference build failed: '+r.stderr[-3000:])
    return directory,metadata


def physical(directory,rows):
    (directory/'inputs.json').write_text(json.dumps(rows,separators=(',',':')))
    r=subprocess.run([str(directory/'observer'),str(directory/'reference'),str(directory/'metadata.json'),str(directory/'inputs.json'),str(directory/'observed.json')],text=True,capture_output=True,timeout=150)
    if r.returncode:raise ValueError('Original execution failed: '+(r.stdout+r.stderr)[-4000:])
    proof=json.loads((directory/'observed.json').read_text())
    if proof['instruction_patches']or proof['supplied_original_outcomes']:raise ValueError('Reference supplied instructions or outcomes')
    return proof


def compact_expected(row,original):
    if not original['completed']:raise ValueError('Original did not complete '+row['name'])
    if original['after_fp']!=row['before_fp']:raise ValueError('Original flag primitive unexpectedly changed FP')
    # The typed API receives the already evaluated BOOLEAN value. The source
    # evaluator's argument read and BE180's stack operand are outside it.
    reads=[[r['address'],r['bytes']]for r in original['reads']if r['address']!=ARG and not STACK<=r['address']<STACK+4096]
    writes=[[w['address'],w['written']]for w in original['writes']]
    return dict(completed=True,reads=reads,writes=writes,banks_sha256=digest(original['banks']))


def compare(rows,expected,native):
    digest_all=hashlib.sha256();cases=0
    with (Path(os.environ['HALO2_TEST_SCRATCH'])/'native.stderr').open('w+')as errors:
        process=subprocess.Popen([str(native)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True)
        try:
            for row,gold in zip(rows,expected,strict=True):
                for publisher in (False,True):
                    process.stdin.write(json.dumps(dict(row,publisher=publisher),separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
                    if not line:errors.seek(0);raise ValueError(row['name']+': native stopped: '+errors.read()[-3000:])
                    actual=json.loads(line);actual['banks_sha256']=digest(actual.pop('banks'))
                    if actual!=gold:raise ValueError(row['name']+': native effects differ; native='+str(actual)+' original='+str(gold))
                    digest_all.update(json.dumps(actual,sort_keys=True).encode());cases+=1
            process.stdin.close()
            if process.wait(timeout=10):errors.seek(0);raise ValueError(errors.read()[-3000:])
        finally:
            if process.poll()is None:process.terminate();process.wait(timeout=5)
    return dict(cases=cases,comparison_sha256=digest_all.hexdigest())


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--xbe',type=Path,required=True);p.add_argument('--native',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True);p.add_argument('--fixture-output',type=Path);a=p.parse_args()
    if not a.xbe.is_file():print('SKIP missing owned TU1.5 executable');return 77
    if sha(a.xbe)!=PIN:raise ValueError('Wrong original executable')
    sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'));from xbe import Xbe
    image=Xbe(a.xbe);rows=inputs();directory,metadata=prepare(image,Path(os.environ['HALO2_TEST_SCRATCH']))
    registration={}
    for index,address,evaluator,types in ((252,0x457ec8,0x2ae380,(5,)),(94,0x45721c,0x2abef0,(50,5))):
        if int.from_bytes(image.read(0x480c00+4*index,4),'little')!=address:raise ValueError('Original builtin descriptor pointer changed')
        desc=image.read(address,14+2*len(types));return_type=struct.unpack_from('<H',desc)[0];argc=struct.unpack_from('<H',desc,12)[0]
        if return_type!=4 or argc!=len(types)or struct.unpack_from('<I',desc,4)[0]!=evaluator or struct.unpack_from('<'+str(len(types))+'H',desc,14)!=types:
            raise ValueError('Original builtin signature/evaluator changed')
        registration[str(index)]=dict(descriptor=address,bytes=desc.hex(),evaluator=evaluator)
    proof=physical(directory,rows);expected=[compact_expected(row,original)for row,original in zip(rows,proof['cases'],strict=True)]
    result=compare(rows,expected,a.native)
    names=['include/halo2/native_tu15_render_script_flags.hpp','src/rendering/native_tu15_render_script_flags.cpp',
        'tests/native_tu15_render_script_flags_tests.cpp','tools/validate_native_tu15_render_script_flags.py','tools/observe_native_campaign_ai_vector_math_tu15.cpp']
    report=dict(passed=True,**result,physical_cases=len(rows),warthog_cases=sum(r['kind']=='warthog'for r in rows),
        visibility_cases=sum(r['kind']=='visibility'for r in rows),none_cases=sum(r['kind']=='visibility'and r['object']==0xffffffff for r in rows),
        source_sha256={n:sha(ROOT/n)for n in names},original_xbe_sha256=PIN,registration=registration,owners=metadata['owners'],
        original_instructions=proof['original_instructions'],instruction_patches=[],supplied_original_outcomes=False,retained_large_artifacts=[],
        scope='Completed2AE380 flag effect and wholeBE180 retained memory reads/writes; evaluated VM arguments and normal completion belong to ScriptVm, not a claimed full evaluator ABI retirement.',
        whole_renderer_visual_parity_verified=False)
    if a.fixture_output:
        fixture=dict(schema='TU15_NATIVE_RENDER_SCRIPT_FLAGS_V1',original_xbe_sha256=PIN,
            inputs_sha256=[digest(r)for r in rows],expected=expected,physical_owners=metadata['owners'])
        packed=gzip.compress(json.dumps(fixture,separators=(',',':')).encode(),mtime=0)
        if len(packed)>1024*1024:raise ValueError('Compact fixture exceeds1MiB')
        a.fixture_output.parent.mkdir(parents=True,exist_ok=True);a.fixture_output.write_bytes(packed)
        report['required_reference_fixture']=dict(path=str(a.fixture_output),bytes=len(packed),sha256=sha(a.fixture_output),scope='Typed declared-input hashes and original effect/bank digests; no raw banks or executable bytes.')
    a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS',len(rows),'physical original script flag cases,',result['cases'],'native publisher comparisons')
    return 0


if __name__=='__main__':raise SystemExit(main())
