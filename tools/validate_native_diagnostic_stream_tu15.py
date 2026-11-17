#!/usr/bin/env python3
"""Private diagnostic controllers with real retained formatting and sink stops."""
import argparse
import copy
import copy
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import struct
import subprocess
import sys
from types import SimpleNamespace

ROOT=Path(__file__).resolve().parents[1]
sys.path[:0]=[str(ROOT/'tools'),str(ROOT/'tools/reference/halo2-decompiled/tools')]
from audit_native_diagnostic_stream_tu15 import provenance,DEFINITION,PRIMARY,PIN
from validate_native_tu15_prt_helpers import physical_original
from xbe import Xbe
from native_geometry_visibility_original import OBSERVER,OBSERVER_PIN
from native_bsp_admission_original import DECLARATION,BOUNDARY,STOP

INPUTS=('include/halo2/native_tu15_diagnostic_stream_kernel.hpp',
    'src/runtime/native_tu15_diagnostic_stream.hpp','tests/native_tu15_diagnostic_stream_tests.cpp',
    'src/runtime/native_tu15_diagnostic_stream_admission.hpp','tools/native_diagnostic_stream_replacements.py',
    'tools/validate_native_diagnostic_stream_tu15.py','tools/audit_native_diagnostic_stream_tu15.py',
    PRIMARY,'src/runtime/source_map_world_support_tu15_generated.cpp',
    'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
    'src/physics/runtime/native_tu15_raw_x87_scope.hpp','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp')
PARENTS=(0x2e3080,0x2e30e0,0x2e2d10,0x2e2e00)


def write_source_bodies(scratch,require_binding=False):
    import native_diagnostic_stream_replacements as binding
    source=(ROOT/'src/runtime/source_map_world_support_tu15_generated.cpp').read_text()
    bound=binding.replace_diagnostic_stream(source)
    if require_binding and bound!=source:raise ValueError('Production diagnostic source defaults missing')
    if binding.replace_diagnostic_stream(bound)!=bound:raise ValueError('Diagnostic source binding nonidempotent')
    def instrument(body):
        return re.sub(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\))',r'\1::retained_statement();',body)
    result='\n'.join(f'static void run_{entry:x}(WorldState&);'for entry in binding.ENTRIES)
    result+='\n'+binding.function(source,'selected')+'\n'
    result+='\n'.join(instrument(binding.function(bound,f'run_{entry:x}'))for entry in binding.ENTRIES)
    (scratch/'diagnostic_source_bodies.inc').write_text(result)
    return dict(original_complete_owner_sha256={hex(entry):binding.digest(binding.function(source,f'run_{entry:x}'))for entry in binding.ENTRIES},
        bound_complete_owner_sha256={hex(entry):binding.digest(binding.function(bound,f'run_{entry:x}'))for entry in binding.ENTRIES})


def physical_with_code_reads(image,rows,scratch,**options):
    original_reader=Path.read_text;section=image.section('.text')
    marker='auto retained=[&](std::uint32_t p,unsigned n){for(const auto&b:c.at("banks"))'
    extension=f'auto retained=[&](std::uint32_t p,unsigned n){{if(p>={section.va}u&&std::uint64_t(p-{section.va}u)+n<={section.rsize}u)return true;for(const auto&b:c.at("banks"))'
    def read(path,*args,**kwargs):
        text=original_reader(path,*args,**kwargs)
        if path.resolve()==OBSERVER:
            if hashlib.sha256(text.encode()).hexdigest()!=OBSERVER_PIN or any(text.count(needle)!=1 for needle in(marker,DECLARATION,BOUNDARY)):
                raise ValueError('Complete diagnostic observer code-data seam drift')
            text=text.replace(marker,extension,1).replace(DECLARATION,'  std::map<std::uint32_t,unsigned>instruction_occurrences;\n'+DECLARATION,1).replace(BOUNDARY,STOP+BOUNDARY,1)
        return text
    Path.read_text=read
    try:proof,authority=physical_original(image,rows,scratch,**options)
    finally:Path.read_text=original_reader
    authority.update(readonly_code_data_reads_included=True,diagnostic_base_sha256=OBSERVER_PIN,
        retained_code_data_extension_sha256=hashlib.sha256(extension.encode()).hexdigest(),
        before_instruction_extension_sha256=hashlib.sha256((DECLARATION+BOUNDARY+STOP).encode()).hexdigest())
    return proof,authority


def fixture_rows(image,buffered=False):
    # Exact original interface/literal/format-table and CRT-control pages.
    # Unneeded image/BSS megabytes are not duplicated for every prefix case.
    regions=((0x41b600,0x1a00),(0x41f000,4096),(0x46e000,4096),(0x61000000,16384),(0x62000000,32768))
    seed=[image.read(base,size)if base<0x60000000 else bytes(size)for base,size in regions]
    rng=random.Random(0x2e3080);rows=[]
    live=('0000000000000080ff3f','45230100000000c0ff7f','01000000000000000000','0000000000000000ffbf',
          '00000000000000800040','00000000000000000000','0000000000000080ffbf','00000000000000000080')
    for family in(('format','flush','length','buffered-format','buffered-flush','formatter-tail')if buffered else('format','flush','length','formatter-tail')):
        for index in range(8):
            banks=[bytearray(b)for b in seed];sp=0x62007000+index
            def put(address,data):
                for(base,size),bank in zip(regions,banks):
                    if base<=address<=base+size-len(data):bank[address-base:address-base+len(data)]=data;return
                raise ValueError('Unbacked diagnostic source input')
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
            entry=0x2e2d10 if family=='formatter-tail'else 0x2e3080 if family.endswith('format')else 0x2e30e0 if family.endswith('flush')else 0x2e2e00
            if family=='formatter-tail':
                destination=0x61001000+index;argument=0x61000200+index
                value=(0,1,127,255,0x40000000,0x7fffffff,0x80000000,0xffffffff)[index]
                put(argument,struct.pack('<I',value));args=[destination,1024,0x41c4d4,argument]
            elif family.endswith('format'):
                stream=0x61000400+index;sink=0x61000500+index
                # Constructor-consistent caller inputs, not an initialized-singleton claim.
                put(stream,struct.pack('<3I',0x41bf0c,0,sink));put(sink,struct.pack('<I',0x41be1c))
                value=(0,1,127,255,0x40000000,0x7fffffff,0x80000000,0xffffffff)[index]
                args=[stream,0x41c4d4,value]
            elif family.endswith('flush'):
                stream=0x61000400+index;sink=0x61000500+index;gp[1]=stream
                put(stream,struct.pack('<3I',0x41bf0c,0,sink));put(sink,struct.pack('<I',0x41be1c));args=[]
            else:
                text=0x61000400+index;size=(0,1,2,3,7,17,255,1023)[index]
                put(text,b'x'*size+b'\0');args=[text]
            if family.startswith('buffered'):
                # Actual2E4000 permits a null downstream writer and publishes
                # this buffered-writer interface/backing/count/capacity layout.
                payload=b'previously buffered bytes\n'if family.endswith('flush')else b''
                capacity=4096 if payload else(1,2,4096,4096,8,17,18,4096)[index]
                buffer=sp-1032 if not payload and index==2 else sp+4 if not payload and index==3 else sink+0x10 if not payload and index==7 else 0x61002000+index
                put(sink,struct.pack('<7I',0x41bf8c,0x1001c,0,buffer,len(payload),capacity,1))
                if payload:put(buffer,payload)
            put(sp,struct.pack('<'+'I'*(len(args)+1),0x63000000,*args))
            row=dict(id=len(rows),name=f'{family}-{index}',entry=entry,gp=gp,
                matrix=args[0]if args else 0,source=args[1]if len(args)>1 else 0,
                eflags=0x202|(0x400 if index&1 and not family.startswith('buffered')else 0),max_steps=30000,
                xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[(0x27f,0x37f,0x77f,0xf7f)[index%4],(index%8)<<11,0x1f80|((index%4)<<13)],
                live_x87=list(live[:(0,1,4,8)[index%4]]),
                banks=[dict(address=base,before=bytes(bank).hex())for(base,_),bank in zip(regions,banks)])
            if family=='format':row['stop_before_entries']=[0x2e1c80]
            rows.append(row)
    return rows,regions


def write_bodies(scratch,entries,image,buffered=False):
    text=(ROOT/PRIMARY).read_text();bodies={int(m[1][4:],16):m[0]for m in DEFINITION.finditer(text)}
    helper=re.search(r'^std::uint32_t sign_extend\([^\n]*\) \{.*?^\}',text,re.M|re.S)
    if not helper:raise ValueError('Missing actual formatter sign-extension helper')
    result=helper[0]+'\n'+'\n'.join(f'static void run_{entry:x}(State&);'for entry in entries)
    result+='\nstatic void dispatch(State&,std::uint32_t,std::uint32_t,bool){throw std::runtime_error("Unqualified retained formatter dynamic owner");}\n'
    def sampled(body):
        return re.sub(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\))',r'\1::retained_child_statement();',body)
    result+='\n'.join(sampled(bodies[entry].replace('(WorldState &s)','(State &s)'))for entry in entries)
    if buffered:
        from generate_physics_live_support_tu15 import generate,complete_body,_Code
        import contextlib,io
        code=_Code(image,'.text');child_entries=(0x2e2e30,0x2e3d60,0x2e3db0,0x2e3e70)
        body_sites={};instructions={};addresses={};routines=[]
        for entry in child_entries:
            body=complete_body(code,entry);body_sites[f'{entry:08x}']=sorted(body.code)
            routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,width,n in body.tables if width==4]))
            for pc in body.code:
                instruction=code.at(pc);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
                for operand in instruction.operands:
                    if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                    if operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
        paired=dict(image='English TU1.5',original_sha256=PIN,entries={f'{entry:08x}':entry for entry in child_entries},
            body_sites=body_sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
        correspondence=scratch/'buffered-correspondence.json';correspondence.write_text(json.dumps(paired))
        registration=scratch/'buffered-registrations.json';registration.write_text('{}')
        args=SimpleNamespace(output=scratch/'buffered-reference.cpp',manifest=scratch/'buffered-generation.json',
            correspondence=correspondence,additional_entries=registration)
        with contextlib.redirect_stdout(io.StringIO()):generate(args,paired)
        generated=args.output.read_text();extra={int(m[1][4:],16):m[0]for m in DEFINITION.finditer(generated)}
        if not set(child_entries)<=extra.keys():raise ValueError('Incomplete actual buffered-writer reference generation')
        result+='\n'+'\n'.join(f'static void run_{entry:x}(State&);'for entry in child_entries)
        result+='\n'+'\n'.join(sampled(extra[entry].replace('(WorldState &s)','(State &s)'))for entry in child_entries)
    (scratch/'diagnostic_formatter_bodies.inc').write_text(result)


def semantic_mutants(scratch,command,fixture):
    mutations=(
        ('double-length','include/halo2/native_tu15_diagnostic_stream_kernel.hpp','while(view.next_byte()!=0)++bytes;','while(view.next_byte()!=0)bytes+=2u;'),
        ('wrong-varargs','src/runtime/native_tu15_diagnostic_stream.hpp','g[0]=g[4]+0x414u;','g[0]=g[4]+0x410u;'),
        ('swapped-saved-registers','src/runtime/native_tu15_diagnostic_stream.hpp','g[7]=pop(0x2e30c8u);g[6]=pop(0x2e30c9u);','g[6]=pop(0x2e30c8u);g[7]=pop(0x2e30c9u);'),
        ('wrong-flush-owner','src/runtime/native_tu15_diagnostic_stream.hpp','const auto target=word(g[0]+12u);','const auto target=word(g[0]+8u);'),
        ('early-output-capture','include/halo2/native_tu15_diagnostic_stream_kernel.hpp','view.format_message();\n    view.capture_output();','view.capture_output();\n    view.format_message();'),
        ('stale-cleared-callable','src/runtime/native_tu15_diagnostic_stream_admission.hpp','else if(s.instruction_cursor||s.literal_service_sites||c.position||c.literal_service_sites||s.observe)return false;','else if(false)return false;'),
        ('reference-control-bypass','src/runtime/native_tu15_diagnostic_stream_admission.hpp','if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;','if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;'))
    results=[]
    for name,path,old,new in mutations:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Missing unique diagnostic mutant '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated.insert(1,'-I'+str(overlay));mutated[mutated.index('-o')+1]=str(binary)
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('Diagnostic semantic mutant did not compile '+name+': '+compiled.stderr[-3000:])
        arguments=['--guards']if name=='stale-cleared-callable'else['--reference-guards']if name=='reference-control-bypass'else[str(fixture)]
        env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
        if name=='reference-control-bypass':env['HALO2_REFERENCE_DIAGNOSTIC_STREAM']='1'
        tested=subprocess.run([str(binary),*arguments],capture_output=True,text=True,env=env)
        if tested.returncode==0:raise ValueError('Compiled diagnostic semantic mutant survived '+name)
        results.append(dict(name=name,compiled=True,rejected=True,diagnostic=(tested.stdout+tested.stderr)[-2500:]))
    return results


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--xbe',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='g++');parser.add_argument('--sanitizers',action='store_true');parser.add_argument('--buffered-writer',action='store_true');parser.add_argument('--prefixes',action='store_true');parser.add_argument('--semantic-mutants',action='store_true');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    if sys.platform!='linux'or platform.machine().lower()not in('x86_64','amd64','i386','i686')or not args.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);pins={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest()for path in INPUTS}
    image=Xbe(args.xbe);assert hashlib.sha256(image.data).hexdigest()==PIN
    owners,formatter_entries=provenance(image);entries=(*PARENTS,*formatter_entries,0x31d400,*((0x2e2e30,0x2e3d60,0x2e3db0,0x2e3e70)if args.buffered_writer else()))
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    original_metadata=common.metadata
    def metadata(image):
        result=original_metadata(image);code=common._Code(image,'.text')
        for site in result['sites']:
            instruction=code.at(site['instruction']);mnemonic=instruction.mnemonic
            if mnemonic in('ret','pop','leave'):site['operands'].append(dict(base=5 if mnemonic=='leave'else 4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            # Authenticated TEST r/m,reg (e.g.85 75 FC at32D812) is a read;
            # this Capstone build reports zero operand-access flags for it.
            if mnemonic=='test':
                for operand in site['operands']:operand.update(read=True,write=False)
        return result
    common.metadata=metadata;common.ENTRIES=entries;meta=metadata(image);code=common._Code(image,'.text')
    parent_sites={site['instruction']for site in meta['sites']if (0x2e3080<=site['instruction']<=0x2e30e5)or(0x2e2e00<=site['instruction']<=0x2e2e10)}
    io={site['instruction']for site in meta['sites']if site['instruction']in parent_sites and(site['operands']or code.at(site['instruction']).mnemonic in('push','call'))}
    write_sites={site['instruction']for site in meta['sites']if any(o['write']for o in site['operands'])or code.at(site['instruction']).mnemonic in('push','call')}
    rows,regions=fixture_rows(image,args.buffered_writer)
    trace_calls=[dict(entry=pc,gp=True,xmm=True)for pc in sorted(io|{0x32ba80,0x2e1c80,0x31d400,0x2e3db0,0x2e3e70})]
    proof,authority=physical_with_code_reads(image,rows,scratch,entries=entries,regions=regions,
        trace_calls=trace_calls,trace_extended=True,preserve_x87_prefix=True)
    prefix_authority=None
    if args.prefixes:
        prefixes=[]
        for index in((0,7,8,16,23,24,31,32)if args.buffered_writer else(0,7,8,16,23)):
            grouped={}
            for event in proof['cases'][index]['calls']:
                pc=event['entry']
                if pc in io:grouped.setdefault(pc,[]).append(event)
            for pc,events in grouped.items():
                for occurrence in sorted({0,len(events)-1}):
                    row=copy.deepcopy(rows[index]);row.pop('stop_before_entries',None)
                    row.update(id=len(rows)+len(prefixes),name=row['name']+f'-before-{pc:x}-{occurrence}',
                        fault=dict(phase='before_instruction',instruction=pc,ordinal=occurrence))
                    prefixes.append(row)
        directory=scratch/'prefix';directory.mkdir()
        stopped,prefix_authority=physical_with_code_reads(image,prefixes,directory,entries=entries,regions=regions,
            trace_calls=trace_calls,trace_extended=True,preserve_x87_prefix=True)
        rows+=prefixes;proof['cases']+=stopped['cases']
    for row,observed in zip(rows,proof['cases'],strict=True):
        row.update({key:observed[key]for key in('boundary_gp','boundary_eflags','boundary_xmm','after_fp','reads','writes','calls')})
        row['expected_pc']=row['fault']['instruction']if row.get('fault')else 0x2e1c80 if row.get('stop_before_entries')else 0x32bad5 if row['entry']==0x2e2d10 else 0x2e30d0 if row['entry']==0x2e3080 else 0x2e3da2 if row['name'].startswith('buffered-flush')else 0x31d400 if row['entry']==0x2e30e0 else 0x2e2e10
        for call in row['calls']:assert call.pop('arguments')==[]
        for bank,after in zip(row['banks'],observed['banks'],strict=True):bank['after']=after['after']
    section=image.section('.text')
    path=scratch/'fixture.json';path.write_text(json.dumps(dict(cases=rows,packet_sites=sorted(io),write_sites=sorted(write_sites),
        immutable_text=dict(address=section.va,bytes=image.section_bytes(section).hex())),separators=(',',':')))
    write_bodies(scratch,formatter_entries,image,args.buffered_writer);source_route=write_source_bodies(scratch,args.require_binding);binary=scratch/'tests'
    command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(scratch),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/'tests/native_tu15_diagnostic_stream_tests.cpp'),'-o',str(binary)]
    if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    if args.buffered_writer:command+=['-DHALO2_TEST_GENUINE_BUFFERED_WRITER=1']
    compiled=subprocess.run(command,capture_output=True,text=True)
    if compiled.returncode:raise ValueError('Diagnostic private compilation failed: '+compiled.stderr[-9000:])
    tested=subprocess.run([str(binary),str(path)],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
    checks=[]
    if tested.returncode==0:
        for name,arguments,extra in(
            ('live_untouched_fallbacks',['--guards'],{}),
            ('reference_untouched_fallbacks',['--reference-guards'],{'HALO2_REFERENCE_DIAGNOSTIC_STREAM':'1'}),
            ('compiled_source_default',['--source-default',str(path)],{}),
            ('compiled_source_positive_reference',['--source-reference',str(path)],{'HALO2_REFERENCE_DIAGNOSTIC_STREAM':'1'}),
            ('compiled_empty_service_default',['--source-empty-default',str(path)],{}),
            ('compiled_empty_service_positive_reference',['--source-empty-reference',str(path)],{'HALO2_REFERENCE_DIAGNOSTIC_STREAM':'1'})):
            checked=subprocess.run([str(binary),*arguments],capture_output=True,text=True,
                env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0',**extra))
            result=dict(name=name,passed=checked.returncode==0,diagnostic=(checked.stdout+checked.stderr)[-9000:])
            if checked.returncode==0:result['result']=json.loads(checked.stdout)
            checks.append(result)
    report=dict(passed=tested.returncode==0 and all(row['passed']for row in checks),private_unbound=not args.require_binding,owned_scratch=str(scratch),source_sha256=pins,
        physical_original=authority,original_cases=len(rows),formatter_segments=sum(row['entry']==0x2e3080 for row in rows),source_route=source_route,
        physical_prefix_original=prefix_authority,prefix_cases=sum('fault'in row for row in rows),
        default_debugger_sink_entries_unresumed=True,full_diagnostic_return_qualified=False,
        buffered_writer_normal_returns=args.buffered_writer,
        buffered_inputs='Constructor-consistent original41BF8C subtype with backed4096-byte buffer and permitted null downstream; whole singleton/constructor execution remains separate.'if args.buffered_writer else None,
        scope='Typed print/flush/strlen controllers; genuine integer formatter and buffered-writer CPU normal returns plus unresumed default debugger-sink entries. This is not a completed default singleton/output-service path.'if args.buffered_writer else'Typed print/flush/strlen controllers; genuine integer formatter returns compose only to actual sink entry. Genuine RET0 flush and strlen complete normally.',
        implementation_status=dict(
            native_controllers=['2e3080 stream controller/frame','2e30e0 dynamic flush tail','2e2d10 formatter handoff','2e2e00 typed byte-length loop'],
            formatter32ba80='Exact compiled retained Primary reference; six actual CPU owners execute for%i. No independent native formatter exists in this proof or production.',
            buffered_writer_and_copy='Exact disposable generated retained2e3db0/2e3e70/2e3d60/2e2e30 reference children; not independently native children.',
            production_defaults='Four exact diagnostic source defaults installed; no native-only dependency closure claim.'if args.require_binding else'Private controller proof; production binding not required by this run.',
            external_output='Default2e1c80 debugger/stdout path remains an unresumed genuine boundary.'),
        diagnostic=(tested.stdout+tested.stderr)[-16000:],retained_large_artifacts=[])
    report['compiled_source_checks']=checks
    report['semantic_mutants']=semantic_mutants(scratch,command,path)if args.semantic_mutants and report['passed']else[]
    executed={pc for case in proof['cases']for pc in case['executed_sites']}
    report['owned_controller_site_coverage']={hex(entry):dict(
        total=authority['owners'][hex(entry)]['sites'],
        executed=len(set(common.complete_body(code,entry).code)&executed))for entry in PARENTS}
    if args.buffered_writer and any(row['total']!=row['executed']for row in report['owned_controller_site_coverage'].values()):
        raise ValueError('Genuine buffered-writer corpus did not cover every diagnostic controller site')
    if tested.returncode==0:report['native']=json.loads(tested.stdout)
    if pins!={path:hashlib.sha256((ROOT/path).read_bytes()).hexdigest()for path in INPUTS}:raise ValueError('Diagnostic source drift')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(report['diagnostic']);
    if not report['passed']:print(json.dumps([row for row in checks if not row['passed']]))
    return 0 if report['passed']else 1


if __name__=='__main__':
    try:raise SystemExit(main())
    except Exception as error:
        if '--report'in sys.argv:
            path=Path(sys.argv[sys.argv.index('--report')+1]);path.parent.mkdir(parents=True,exist_ok=True)
            path.write_text(json.dumps(dict(passed=False,private_unbound=True,owned_scratch=os.environ.get('HALO2_TEST_SCRATCH'),error=str(error)[-12000:],retained_large_artifacts=[]),indent=2)+'\n')
        raise
