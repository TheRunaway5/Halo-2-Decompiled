#!/usr/bin/env python3
"""Private genuine MOPP invalid-byte diagnostics through returned stream owners.

The buffered writer/formatter are exact retained reference algorithms. The
original null store is a declared beforeInstruction boundary, never resumed.
"""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import random
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from xbe import Xbe
from validate_native_diagnostic_stream_tu15 import (INPUTS,PARENTS,fixture_rows,
    physical_with_code_reads,provenance,write_bodies,write_source_bodies,PIN)

FILES=tuple(dict.fromkeys((*INPUTS,
    'include/halo2/native_tu15_diagnostic_logger_kernel.hpp',
    'src/runtime/native_tu15_diagnostic_logger.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_diagnostic_frame.hpp',
    'src/physics/runtime/native_tu15_mopp_ray_integer_frame.hpp',
    'tests/native_tu15_mopp_diagnostic_return_tests.cpp',
    'tools/validate_native_mopp_diagnostic_return_tu15.py')))
def hashes():
    # Unrelated sibling prefix installations may change the whole Primary TU.
    # Retain exact formatter/helper definitions rather than a stale TU hash.
    from native_diagnostic_stream_replacements import function
    from audit_native_diagnostic_stream_tu15 import PRIMARY
    result={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in FILES if name!=PRIMARY}
    text=(ROOT/PRIMARY).read_text()
    from audit_native_diagnostic_stream_tu15 import DEFINITION
    bodies={m[1]:m[0]for m in DEFINITION.finditer(text)};pending=['run_32ba80'];seen=set()
    import re
    while pending:
        name=pending.pop()
        if name in seen:continue
        seen.add(name);body=bodies[name];result[PRIMARY+'::'+name]=hashlib.sha256(body.encode()).hexdigest()
        pending.extend(set(re.findall(r'\b(run_[0-9a-f]+)\(s\)',body))-seen)
    result[PRIMARY+'::sign_extend']=hashlib.sha256(function(text,'sign_extend').encode()).hexdigest()
    parent='src/campaign/ai/source_ai_frame_tu15_generated.cpp'
    result[parent+'::run_30aa70']=hashlib.sha256(function((ROOT/parent).read_text(),'run_30aa70').encode()).hexdigest()
    if result[parent+'::run_30aa70']!='0965ba2dfbc6d947bea64e48f8224b3f85d36efbe9c39739f46d7ec012d0064d':raise ValueError('Complete owning MOPP sourcebody drift')
    return result

def inputs(image):
    originals,regions=fixture_rows(image,True)
    regions=(*regions,(0x48c000,4096))
    template=next(row for row in originals if row['name']=='buffered-format-0')
    rng=random.Random(0x30ae16);rows=[]
    for n in range(80):
        row=copy.deepcopy(template);row.update(id=n,name=('getter-'if n<16 else'buffered-parent-'if n<40 else'default-parent-'if n<48 else'direct-epilogue-')+str(n))
        row['banks'].append(dict(address=0x48c000,before=image.read(0x48c000,4096).hex()))
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        def put(address,value,width=4):
            data=int(value&((1<<(8*width))-1)).to_bytes(width,'little')
            for base,bank in banks.items():
                if base<=address<=base+len(bank)-width:bank[address-base:address-base+width]=data;return
            raise ValueError('Unbacked genuine diagnostic input')
        gp=[rng.getrandbits(32)for _ in range(8)];sp=0x62006000+n%16;gp[4]=sp
        row.update(gp=gp,entry=0x2e16d0 if n<16 else 0x30aa70,
            before_fp=originals[n%len(originals)]['before_fp'],
            live_x87=originals[n%len(originals)]['live_x87'],
            xmm=originals[n%len(originals)]['xmm'],eflags=0x202,max_steps=30000,
            matrix=0x61000800,source=0x61000100)
        for offset,value in enumerate((0x63000000,row['matrix'],row['source'],0x61000900)):put(sp+offset*4,value)
        if n>=48:
            row['entry']=0x30ae4c;gp[5]=sp+(-4,0,4,8,12,16,64,128)[n%8]
            for offset in range(3):put(sp+offset*4,rng.getrandbits(32))
            put(gp[5],rng.getrandbits(32));put(gp[5]+4,0x63000000)
            def word(address):
                base=next(base for base,bank in banks.items()if base<=address<=base+len(bank)-4)
                return int.from_bytes(banks[base][address-base:address-base+4],'little')
            # The common physical runner seeds these three words itself.
            # Match the actual declared overlapping stack after all inputs.
            row.update(return_pc=word(sp),matrix=word(sp+4),source=word(sp+8))
            row['eflags']=0x202|((n%2)*0x400)|(0xd5 if n&2 else 0)
        elif n<16:
            gp[1]=(0,1,7,0xfffffff8,0xfffffffc,0xffffffff,0x61000400,0x80000000)[n%8]
            row['eflags']=0x202|((n%2)*0x400)|(0xd5 if n&2 else 0)
        else:
            gp[1]=0x61000a00
            logger=0x61000400+(n%4)*0x40;sink=0x61001000+(n%16)*32;buffer=0x61002000+(n%16)*64
            put(0x48c99c,logger);put(logger,0x41be00);put(logger+6,1,2)
            put(logger+8,0x41bf0c);put(logger+16,sink)
            opcode=0x6c+(n*17)%0x94;put(row['source'],opcode,1)
            if n<40:
                for offset,value in enumerate((0x41bf8c,0x1001c,0,buffer,0,(1,2,8,17,18,4096)[n%6],1)):put(sink+offset*4,value)
                row.update(null_boundary=True,fault=dict(phase='before_instruction',instruction=0x30ae42,ordinal=0))
            else:
                put(sink,0x41be1c);put(sink+6,1,2)
                row.update(sink_boundary=True,stop_before_entries=[0x2e1c80])
        row['banks']=[dict(address=base,before=bytes(data).hex())for base,data in banks.items()]
        rows.append(row)
    for original in rows[16:40]:
        row=copy.deepcopy(original);row.update(id=len(rows),name=original['name']+'-actual-null-sigsegv',actual_write_fault=True)
        row.pop('null_boundary');row.pop('fault');rows.append(row)
    return rows,regions

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--xbe',type=Path,required=True);parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins=hashes()
    report=dict(passed=False,production_binding=False,source_sha256=pins,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
        if image.read(0x2e16d0,4)!=bytes.fromhex('8d4108c3'):raise ValueError('Complete getter opcode drift')
        if image.read(0x30ae42,10)!=bytes.fromhex('c7050000000000000000'):raise ValueError('Intentional MOPP null store drift')
        _,formatter=provenance(image);rows,regions=inputs(image)
        owners=(0x30aa70,0x2e16d0,*PARENTS,*formatter,0x31d400,0x2e2e30,0x2e3d60,0x2e3db0,0x2e3e70)
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        original_metadata=common.metadata;code=common._Code(image,'.text')
        def metadata(image):
            result=original_metadata(image)
            for site in result['sites']:
                mnemonic=code.at(site['instruction']).mnemonic
                if mnemonic in('ret','pop','leave'):site['operands'].append(dict(base=5 if mnemonic=='leave'else 4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if mnemonic=='test':
                    for operand in site['operands']:operand.update(read=True,write=False)
            return result
        common.metadata=metadata;common.ENTRIES=owners;meta=metadata(image)
        owned=set(common.complete_body(code,0x30aa70).code)|set(common.complete_body(code,0x2e16d0).code)
        for owner in PARENTS:owned.update(common.complete_body(code,owner).code)
        io=[site['instruction']for site in meta['sites']if site['instruction']in owned and
            (site['operands']or code.at(site['instruction']).mnemonic in('push','pop','ret','call'))]
        traces=sorted(set(io)|{0x2e16d0,0x32ba80,0x31d400,0x2e3db0,0x2e3e70})
        observed,authority=physical_with_code_reads(image,rows,scratch,entries=owners,regions=regions,
            trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in traces],trace_extended=True,preserve_x87_prefix=True)
        executed=set()
        for row,actual in zip(rows,observed['cases'],strict=True):
            if row.get('null_boundary'):assert not actual['completed']and actual['boundary']['instruction']==0x30ae42
            elif row.get('actual_write_fault'):assert not actual['completed']and actual['boundary']['phase']=='actual_write_fault'and actual['boundary']['instruction']==0x30ae42
            else:assert actual['completed']
            assert actual['live_x87_unchanged'];executed.update(actual['executed_sites'])
            row.update({key:actual[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')})
            row['expected_pc']=0x30ae42 if row.get('null_boundary')or row.get('actual_write_fault')else 0x2e1c80 if row.get('sink_boundary')else 0x30ae52 if row['entry']==0x30ae4c else 0x2e16d3
            if row.get('actual_write_fault'):
                row['pre_null_store_packet']=next(packet for packet in row['calls']if packet['entry']==0x30ae42)
            for packet in row['calls']:packet.pop('arguments')
            for bank,after in zip(row['banks'],actual['banks'],strict=True):bank['after']=after['after']
        write_bodies(scratch,formatter,image,True);write_source_bodies(scratch,True)
        section=image.section('.text');fixture=scratch/'fixture.json'
        fixture.write_text(json.dumps(dict(cases=rows,packet_sites=io,immutable_text=dict(address=section.va,bytes=image.section_bytes(section).hex())),separators=(',',':')))
        for compiler in args.compiler or['g++']:
            binary=scratch/('native-'+Path(compiler).name)
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-DHALO2_TEST_GENUINE_BUFFERED_WRITER=1','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_mopp_diagnostic_return_tests.cpp'),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            compiled=subprocess.run(command,capture_output=True,text=True)
            if compiled.returncode:raise ValueError('Diagnostic-return compilation failed '+compiler+': '+compiled.stderr[-9000:])
            tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            row=dict(compiler=compiler,passed=tested.returncode==0,diagnostic=(tested.stdout+tested.stderr)[-12000:]);report['compilers'].append(row)
            if tested.returncode:raise ValueError('Diagnostic-return comparison failed '+row['diagnostic'])
            row['result']=json.loads(tested.stdout)
        report.update(passed=True,physical_original=authority,cases=len(rows),getter_original_bytes='8d4108c3',getter_sites=2,
            parent_executed_sites=sorted(executed&set(common.complete_body(code,0x30aa70).code)),
            separately_declared_epilogue_sites=[0x30ae4c,0x30ae4d,0x30ae4e,0x30ae4f,0x30ae51,0x30ae52],
            actual_original_sigsegv_cases=sum(row.get('actual_write_fault',False)for row in rows),
            scope='Private actual30AA70 invalid-opcode entry through genuinely returned authored logger getter/native stream controllers/retained formatter and buffered writer. Separate actual CPU NULL-write SIGSEGV/native provider refusals and directly entered raw epilogue returns; no successful NULL store or whole-parent normal return is supplied.',
            input_ownership='Constructor-consistent authored41BE00 logger+embedded41BF0C stream and41BF8C buffered writer; real48C99C pointer supplied as backed input. Initializer execution is not claimed.',
            retained_dependencies=['32ba80 formatter and executed children','2e3db0/2e3e70/2e3d60/2e2e30 buffered writer/copy'],
            native_default_getter=False,no_supplied_child_outcomes=True)
        if hashes()!=pins:raise ValueError('Diagnostic-return source drift')
    except Exception as error:report.update(passed=False,error=str(error)[-14000:])
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=report['passed'],report=str(args.report),error=report.get('error'))));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
