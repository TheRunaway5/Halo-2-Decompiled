#!/usr/bin/env python3
"""Original physical cached speech-cluster mask versus native retained owners."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import subprocess
import sys
sys.dont_write_bytecode=True
from validate_native_campaign_audio_leaves_tu15 import ROOT,ORIGINAL,PIN,text_section,sha,save,checked
BASE,SIZE=0x1000000,0x40000
STRUCTURE,SCALE,TABLE,VISIBILITY,LOCATION=BASE,BASE+0x1000,BASE+0x10000,BASE+0x32000,BASE+0x3f000
CACHE,BIAS,KEY=0x502000,0x502200,0x502240
STACK,SP,STOP=0x2000000,0x200f000,0x3000000
REGIONS=((BASE,SIZE),(0x4ecc18,4),(CACHE,0x1000),(0x46a000,0x1000))
def original_bytes(image,address,size):
    base,count,table=(struct.unpack_from('<I',image,p)[0]for p in (0x104,0x11c,0x120))
    for index in range(count):
        row=table-base+index*56;va,virtual,raw,length=struct.unpack_from('<4I',image,row+4)
        if va<=address and address+size<=va+min(virtual,length):return image[raw+address-va:raw+address-va+size]
    raise ValueError(f'Original immutable input unowned: {address:x}')
def signed(v):return (v&0xffffffff)-0x100000000 if v&0x80000000 else v&0xffffffff
def pair(first,second,count):
    low,high=sorted((first,second));product=signed(((low+1)*low)&0xffffffff);half=abs(product)//2*(-1 if product<0 else 1)
    return ((count-1)*low-half+high-1)&0xffffffff
def corpus(image):
    rng=random.Random(0x203c00);rows=[];configs=[]
    for modern in (False,True):
        for first,second in ((0,0),(0,1),(1,0),(1,2),(2,1),(0,65536),(-1,0),(-3,-1),(65536,65537)):
            for blocked in (False,True):configs.append(dict(kind='blocked',label=f'blocked-{modern}-{first}-{second}-{blocked}',modern=modern,first=first,second=second,blocked=blocked,count=65538 if first==65536 else 7))
    for count in (-31,-1,0,1,2,3,31,32,33,64,513):
        for modern in (False,True):configs.append(dict(kind='mask',label=f'count-{count}-{modern}',count=count,modern=modern,cluster=0))
    for modern in (False,True):
        for cell in (0,1,19,20,127,128,129,254,255):configs.append(dict(kind='mask',label=f'cells-{modern}-{cell}',modern=modern,cell=cell))
    for lower in (0,0x421fffff,0x42200000,0x42200001,0xbf800000,0x7fc01234,0x7f800001):
        for rounding in range(4):configs.append(dict(kind='mask',label=f'boundary-{lower:x}-{rounding}',modern=True,cell=1,lower=lower,rounding=rounding))
    for denorm in (0,0x40,0x8000,0x8040):configs.append(dict(kind='mask',label=f'scale-denorm-{denorm}',modern=True,cell=127,lower=1,upper=0x007fffff,denorm=denorm))
    configs += [dict(kind='mask',label=label,**options)for label,options in [
        ('cache-hit',dict(cached=True)),('cache-hit-unavailable-structure',dict(cached=True,structure_pointer=0)),
        ('self-high-index-cache-overlap',dict(count=513,cluster=512)),('negative-cluster',dict(cluster=-1)),
        ('negative-scale-count',dict(scale_count=-1)),('visibility-all-blocked',dict(modern=True,blocked=True)),
        ('live-count-alias-bitmap',dict(count_alias=True,count=3)),('scale-lower-alias-bitmap',dict(modern=True,lower_alias=True)),
        ('visibility-alias-bitmap',dict(modern=True,visibility_alias=True)),('legacy-table-alias-bitmap',dict(legacy_alias=True)),
        ('literal-threshold-upper-nan',dict(modern=True,cell=127,upper=0x7fc01234))]]
    configs += [dict(kind='mask',label='actual203D80-mask-block-'+str(modern),modern=modern,caller=True,cluster=1)for modern in (False,True)]
    for options in configs:
        main=bytearray(rng.randbytes(SIZE));cache=bytearray(rng.randbytes(0x1000));banks={BASE:main,CACHE:cache}
        def put(p,v,n=4):
            for at,data in banks.items():
                if at<=p and p+n<=at+len(data):data[p-at:p-at+n]=int(v&((1<<(n*8))-1)).to_bytes(n,'little');return
            raise ValueError(hex(p))
        def fill(p,size,value):
            for at,data in banks.items():
                if at<=p and p+size<=at+len(data):data[p-at:p-at+size]=bytes([value])*size;return
            raise ValueError(hex(p))
        count=options.get('count',7);cluster=options.get('cluster',0);first=options.get('first',cluster);second=options.get('second',1);modern=options.get('modern',False)
        structure=BIAS-0x9c if options.get('count_alias')else STRUCTURE
        scale=BIAS-4 if options.get('lower_alias')else SCALE
        table=BIAS if options.get('legacy_alias')else TABLE;visibility=BIAS if options.get('visibility_alias')else VISIBILITY
        if not options.get('legacy_alias'):fill(TABLE,0x21000,options.get('cell',10))
        fill(VISIBILITY,0x8000,255 if options.get('blocked')else 0)
        if options['kind']=='blocked':
            index=pair(first,second,count)
            if modern:
                p=visibility+(signed(index)>>5)*4;put(p,1<<(index&31)if options['blocked']else 0)
            elif first!=second:
                # The fallback kernel consumes signed WORD cluster operands.
                narrow=lambda x:(x&65535)-65536 if x&32768 else x&65535
                index=pair(narrow(first),narrow(second),count);put(table+signed(index),0x80 if options['blocked']else 0,1)
        for p,v,n in [(structure+0x9c,count,4),(structure+0xe8,table,4),(structure+0x224,options.get('scale_count',1 if modern else 0),4),(structure+0x228,scale,4),(scale+4,options.get('lower',0),4),(scale+8,options.get('upper',0x42a00000),4),(scale+0x20,visibility,4),(scale+0x28,TABLE,4),(KEY,cluster if options.get('cached')else 0x7ffe,2),(LOCATION+4,cluster,2)]:put(p,v,n)
        number=len(rows);depth=number%9;rounding=options.get('rounding',number%4);precision=number%3
        before_x87=[struct.pack('<QH',v<<(63-(v.bit_length()-1)),16383+v.bit_length()-1).hex()for v in range(1,depth+1)]
        gp=[first&0xffffffff,second&0xffffffff,structure,0x456789ab,SP,0x56789abc,0x6789abcd,0x789abcde]
        entry=0x252f30 if options['kind']=='blocked'else 0x203c00;arguments=[]if options['kind']=='blocked'else[cluster&0xffffffff]
        pairs=count*(count-1)//2 if 0<count<=513 else 0x100
        table_extent=max(0x200,pairs+0x100)
        mask_extent=max(0x200,(pairs+31)//32*4+0x100)
        if options['kind']=='blocked'and modern:
            mask_extent=max(mask_extent,max(0,signed(pair(first,second,count))>>5)*4+0x104)
        declared=[dict(address=BASE,before=bytes(main[:0x2000]).hex()),dict(address=TABLE-0x100,before=bytes(main[TABLE-BASE-0x100:TABLE-BASE+table_extent]).hex()),dict(address=VISIBILITY-0x100,before=bytes(main[VISIBILITY-BASE-0x100:VISIBILITY-BASE+mask_extent]).hex()),dict(address=LOCATION,before=bytes(main[LOCATION-BASE:LOCATION-BASE+16]).hex()),dict(address=0x4ecc18,before=struct.pack('<I',options.get('structure_pointer',structure)).hex()),dict(address=CACHE,before=bytes(cache).hex()),dict(address=0x46a000,before=original_bytes(image,0x46a000,0x1000).hex())]
        row=dict(name=options['label'],kind=options['kind'],entry=entry,structure=structure,first=first,second=second,cluster=cluster,gp=gp,xmm=[[0x3f800000+i,0x24680000+i,0x35790000+i,0x468a0000+i]for i in range(8)],stack=SP,stop=STOP,arguments=arguments,before_x87=before_x87,before_fp=[(0x7f,0x27f,0x37f)[precision]|rounding<<10,((number%8)<<11)|(number%64),0x1f80|rounding<<13|options.get('denorm',(0,0x40,0x8000,0x8040)[(number//16)%4])],banks=declared)
        if options.get('caller'):
            row.update(entry=0x203e8b,capture_entry=0x203c00,stop=0x203ea3,arguments=[],stack_locals=[(SP+0x10,LOCATION)])
        rows.append(row)
    return rows

def physical(scratch):
    if sha(ORIGINAL)!=PIN:raise ValueError('Original TU1.5 image differs')
    image=ORIGINAL.read_bytes();
    for p,v in {0x46a148:0x437f0000,0x46a26c:0x43800000,0x46a2d8:0x3b81848e,0x46a2d4:0x40010204}.items():
        if original_bytes(image,p,4)!=struct.pack('<I',v):raise ValueError('Proved immutable cluster literal differs')
    address,size,text=text_section(image);owned=scratch/'physical';owned.mkdir();(owned/'text.bin').write_bytes(text)
    rows=corpus(image);(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in REGIONS));observer=owned/'observer'
    checked(['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_speech_cluster_mask_reference_tu15.cpp'),'-o',str(observer)])
    total=0
    for row in rows:
        regions=[(bank['address'],len(bank['before'])//2)for bank in row['banks']];(owned/'regions.txt').write_text(''.join(f'{p} {n}\n'for p,n in regions))
        sections=[f'.original 0x{address:x} : {{ *(.original) }}'];assembly=[f'.section .original,"ax"\n.incbin "{owned/"text.bin"}"']
        for index,bank in enumerate(row['banks']):
            if STACK<=bank['address']<STACK+0x10000:continue
            path=owned/f'bank{index}.bin';path.write_bytes(bytes.fromhex(bank['before']));assembly.append(f'.section .bank{index},"aw"\n.incbin "{path}"');sections.append(f'.bank{index} 0x{bank["address"]:x} : {{ *(.bank{index}) }}')
        stack=bytearray(0x10000);struct.pack_into('<I',stack,row['stack']-STACK,STOP)
        for index,value in enumerate(row['arguments']):struct.pack_into('<I',stack,SP-STACK+4+index*4,value)
        for at,value in row.get('stack_locals',[]):struct.pack_into('<I',stack,at-STACK,value)
        for bank in row['banks']:
            if STACK<=bank['address']<STACK+0x10000:
                values=bytes.fromhex(bank['before']);stack[bank['address']-STACK:bank['address']-STACK+len(values)]=values
        (owned/'stack.bin').write_bytes(stack)
        assembly.append(f'.section .stack,"aw"\n.incbin "{owned/"stack.bin"}"');sections.append(f'.stack 0x{STACK:x} : {{ *(.stack) }}')
        cw,sw,mxcsr=row['before_fp'];depth=len(row['before_x87']);sw=(sw&~0x3800)|(((((sw>>11)+depth)&7))<<11);environment=bytearray(28);struct.pack_into('<H',environment,0,cw);struct.pack_into('<H',environment,4,sw);struct.pack_into('<H',environment,8,0xffff);(owned/'environment.bin').write_bytes(environment)
        assembly.append('.code32\n.section .bridge,"ax"\n.globl _start\n_start:\n fninit\n fldenv environment\n ldmxcsr sse_control')
        for index in range(depth-1,-1,-1):assembly.append(f' fldt extended{index}')
        for index in range(8):assembly.append(f' movdqu vector{index},%xmm{index}')
        for reg,value in zip(('eax','ecx','edx','ebx','esp','ebp','esi','edi'),row['gp']):assembly.append(f' mov ${value},%{reg}')
        assembly.append(f' int3\n jmp *entry\n.balign 4\nentry: .long {row["entry"]}\nsse_control: .long {mxcsr}\nenvironment: .incbin "{owned/"environment.bin"}"')
        for index,vector in enumerate(row['xmm']):assembly.append(f'vector{index}: .long '+','.join(map(str,vector)))
        for index,value in enumerate(row['before_x87']):assembly.append(f'extended{index}: .byte '+','.join(str(byte)for byte in bytes.fromhex(value)))
        sections.append('.bridge 0x70000000 : { *(.bridge) }');(owned/'reference.s').write_text('\n'.join(assembly)+'\n');(owned/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n}\n')
        checked(['as','--32',str(owned/'reference.s'),'-o',str(owned/'reference.o')]);checked(['ld','-melf_i386','-T',str(owned/'link.ld'),str(owned/'reference.o'),'-o',str(owned/'reference')])
        try:checked([str(observer),str(owned/'reference'),hex(row['stop']),hex(address+size),str(owned/'regions.txt'),str(owned/'output.bin')]+([hex(row['capture_entry'])]if 'capture_entry'in row else []))
        except RuntimeError as error:raise RuntimeError(row['name']+': '+str(error))from error
        if 'capture_entry'in row:
            captured=(owned/'output.bin.capture').read_bytes();cw,sw,mxcsr,prefix_steps=struct.unpack_from('<4I',captured);position=16
            row['gp']=list(struct.unpack_from('<8I',captured,position));position+=32;row['xmm']=[list(struct.unpack_from('<4I',captured,position+i*16))for i in range(8)];position+=128
            row['before_x87']=[captured[position+i*16:position+i*16+10].hex()for i in range(len(row['before_x87']))];position+=128;row['before_fp']=[cw,sw,mxcsr]
            for bank,(_,length)in zip(row['banks'],regions):bank['before']=captured[position:position+length].hex();position+=length
            actual_cluster,=struct.unpack_from('<I',captured,position);position+=4;count,=struct.unpack_from('<I',captured,position);position+=4
            prefix_sites=list(struct.unpack_from('<'+'I'*count,captured,position));position+=count*4
            count,=struct.unpack_from('<I',captured,position);position+=4;prefix_stores=[]
            for _ in range(count):
                site,destination,length=struct.unpack_from('<III',captured,position);position+=12;prefix_stores.append(dict(instruction=site,address=destination,before=captured[position:position+length].hex(),written=captured[position+4:position+4+length].hex()));position+=8
            if position!=len(captured)or 0x203e9a not in prefix_sites:raise ValueError('Actual admission caller receipt differs')
            row['caller']=dict(entry=0x203e8b,call_site=0x203e9a,callee=row['capture_entry'],return_site=0x203e9f,continuation_stop=0x203ea3,returned_pointer_consumed_at=0x203e9f,prefix_steps=prefix_steps,prefix_sites=prefix_sites,prefix_engine_stores=prefix_stores,supplied_returns=[])
            row.update(entry=row['capture_entry'],arguments=[actual_cluster],stack=row['gp'][4],cluster=actual_cluster&65535)
            if row['cluster']>=32768:row['cluster']-=65536
        data=(owned/'output.bin').read_bytes();cw,sw,mxcsr,instructions=struct.unpack_from('<4I',data);offset=16;row['after_gp']=list(struct.unpack_from('<8I',data,offset));offset+=32;row['after_xmm']=[list(struct.unpack_from('<4I',data,offset+i*16))for i in range(8)];row['after_xmm0']=row['after_xmm'][0];offset+=128;row['after_x87']=[data[offset+i*16:offset+i*16+10].hex()for i in range(len(row['before_x87']))];offset+=128;row.update(after_fp=[cw,sw,mxcsr],instructions=instructions);total+=instructions
        for bank,(_,length)in zip(row['banks'],regions):bank['after']=data[offset:offset+length].hex();offset+=length
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['reads']=[]
        for _ in range(count):
            destination,length,stores_before=struct.unpack_from('<III',data,offset);offset+=12;row['reads'].append(dict(address=destination,stores_before=stores_before,value=data[offset:offset+length].hex()));offset+=4
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['writes']=[]
        for _ in range(count):
            site,destination,length=struct.unpack_from('<III',data,offset);offset+=12;row['writes'].append(dict(instruction=site,address=destination,before=data[offset:offset+length].hex(),written=data[offset+4:offset+4+length].hex()));offset+=8
        count,=struct.unpack_from('<I',data,offset);offset+=4;row['sites']=list(struct.unpack_from('<'+'I'*count,data,offset));offset+=count*4
        if 'caller'in row:
            if 0x203e9f not in row['sites']or row['after_gp'][0]!=0x502200:raise ValueError('Original caller did not consume the real mask pointer')
        if offset!=len(data):raise ValueError('Path heap observer output size differs')
    coverage={};reached=set().union(*(set(row['sites'])for row in rows))
    site_reference=json.loads((ROOT/'tests/fixtures/native_campaign_ai_speech_cluster_mask_tu15_sites.json').read_text())
    if site_reference['original_image_sha256']!=PIN:raise ValueError('Heap instruction inventory image pin differs')
    counts={0x203c00:63,0x252f30:46,0x253070:66,0x2531e0:26}
    if {int(r['entry'],16)for r in site_reference['routines']}!=set(counts):raise ValueError('Speech cluster mask closed graph differs')
    for routine in site_reference['routines']:
        entry=int(routine['entry'],16);expected=set()
        for line in routine['listing']:
            site,encoding,*_=line.split();site=int(site,16);encoding=bytes.fromhex(encoding)
            if original_bytes(image,site,len(encoding))!=encoding:raise ValueError('Heap original instruction inventory bytes differ')
            expected.add(site)
        if len(expected)!=counts[entry]:raise ValueError('Heap original instruction inventory count differs')
        unreachable=set()
        missing=(expected-unreachable)-reached
        if missing:raise ValueError('Speech queue reachable original sites missing: '+','.join(f'{p:08x}'for p in sorted(missing)))
        coverage[f'{entry:08x}']=dict(required_sites=len(expected-unreachable),reached_sites=len((expected-unreachable)&reached),unreachable_sites=sorted(unreachable),missing_sites=sorted(missing))
    return dict(image_sha256=PIN,backend='unchanged-tu15-physical-i386-interleaved-inputs-stores-fp',cases=rows,instructions=total,coverage=coverage,
                original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[])

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--native',type=Path)
    parser.add_argument('--compiler',default='c++');parser.add_argument('--sanitizers',action='store_true');args=parser.parse_args()
    if not ORIGINAL.is_file():
        args.output.mkdir(parents=True,exist_ok=True)
        save(args.output/'report.json',dict(passed=False,status='skipped',validation_not_run=True,
            reason='Required owned TU1.5 offline reference is unavailable',required_reference=str(ORIGINAL),retained_large_artifacts=[]))
        print('SKIP: required owned TU1.5 offline reference is unavailable')
        return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();args.output.mkdir(parents=True,exist_ok=True);report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[]);save(args.output/'report.json',report)
    sources=[ROOT/'include/halo2/native_campaign_ai_speech_cluster_mask_tu15.hpp',ROOT/'src/campaign/ai/native_campaign_ai_speech_cluster_mask_tu15.cpp',ROOT/'tests/native_campaign_ai_speech_cluster_mask_tu15_tests.cpp',ROOT/'tools/observe_native_campaign_ai_speech_cluster_mask_reference_tu15.cpp',Path(__file__).resolve(),ROOT/'tests/fixtures/native_campaign_ai_speech_cluster_mask_tu15_sites.json',ROOT/'include/halo2/native_campaign_audio_ducking.hpp',ROOT/'include/halo2/native_campaign_audio_leaves.hpp',ROOT/'include/halo2/native_campaign_audio_listeners.hpp',ROOT/'cmake/CampaignNativeAiSpeechClusterMask.cmake']
    report['source_hashes']={str(p.relative_to(ROOT)):sha(p)for p in sources}
    try:
        fixture=physical(scratch);path=scratch/'original-speech-cluster-mask.json';path.write_text(json.dumps(fixture,separators=(',',':'))+'\n')
        report.update(actual_original_caller_receipts=[dict(name=row['name'],**row['caller'])for row in fixture['cases']if 'caller'in row],caller_contract='actual signed cluster/private caller argument; retained BSP/cache geometry, source caller consumes returned mask pointer; arbitrary SIMD scratch is outside the actual caller result contract',original_cases=len(fixture['cases']),original_instructions=fixture['instructions'],ordered_original_reads=sum(len(c['reads'])for c in fixture['cases']),ordered_original_stores=sum(len(c['writes'])for c in fixture['cases']),coverage=fixture['coverage'],fixture_sha256=sha(path),fixture_bytes=path.stat().st_size,original_image_sha256=PIN,original_instruction_bytes_changed=False,supplied_dependency_returns=[],external_calls=[],interleaved_mutable_reads=True);save(args.output/'report.json',report)
        project=scratch/'project';project.mkdir();text=sources[1].read_text();variants={'native':None,
            'cache_hit':('if(std::uint16_t(cluster)==read(m,key,2))return bitmap;','if((read(m,key,2),false))return bitmap;'),
            'last_cluster':('if(next>=signed32(count))break;','if(next+1>=signed32(count))break;'),
            'threshold_equal':('native_campaign_ducking_detail::above(native_campaign_ducking_detail::scalar(threshold_bits),native_campaign_ducking_detail::scalar(scale))','!native_campaign_ducking_detail::above(native_campaign_ducking_detail::scalar(scale),native_campaign_ducking_detail::scalar(threshold_bits))'),
            'negative_bit_index':('std::uint32_t(signed32(index)>>5)','(index>>5u)'),
            'legacy_bit':('native_campaign_cluster_byte(source,std::int16_t(first),std::int16_t(second))>>7u','native_campaign_cluster_byte(source,std::int16_t(first),std::int16_t(second))&1u'),
            'stale_initial_count':('if(signed32(structure.cluster_count())>0)','if(signed32(initial_count)>0)'),
            'bitmap_clear':('write(m,bitmap+n*4u,0);','write(m,bitmap+n*4u,1);')}
        negative=project/'negative.py';negative.write_text("import subprocess,sys\nr=subprocess.run(sys.argv[1:],text=True,capture_output=True,timeout=60)\nassert r.returncode!=0 and any(message in r.stderr for message in ('ordered speech store','ordered speech input','speech result differs','speech FP differs','Speech input owner absent','Speech input extent absent')),(r.returncode,r.stdout,r.stderr)\nprint('Independent original rejected speech mutation:',r.stderr.strip())\n")
        cmake='cmake_minimum_required(VERSION 3.20)\nproject(native_campaign_speech_cluster_mask LANGUAGES CXX)\ninclude(CTest)\nfind_package(nlohmann_json CONFIG REQUIRED)\n';targets=[]
        for name,mutation in variants.items():
            core=sources[1]
            if mutation:
                if text.count(mutation[0])!=1:raise ValueError('Speech mutation location changed: '+name)
                core=project/(name+'.cpp');changed=text.replace(*mutation)
                if name=='stale_initial_count':changed=changed.replace('    const auto rounded=std::uint32_t(signed32(structure.cluster_count()+31u)>>5)*4u;','    const auto initial_count=structure.cluster_count();\n    const auto rounded=std::uint32_t(signed32(initial_count+31u)>>5)*4u;')
                core.write_text(changed)
            target='campaign_speech_cluster_mask_'+name;targets.append(target)
            cmake+=f'add_executable({target} "{sources[2]}" "{core}")\ntarget_include_directories({target} PRIVATE "{ROOT}/include")\ntarget_compile_features({target} PRIVATE cxx_std_20)\ntarget_compile_options({target} PRIVATE -fno-fast-math -ffp-contract=off -frounding-math)\ntarget_link_libraries({target} PRIVATE nlohmann_json::nlohmann_json)\n'
            if args.sanitizers:cmake+=f'target_compile_options({target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)\ntarget_link_options({target} PRIVATE -fsanitize=address,undefined)\n'
            command=f'"{sys.executable}" "{ROOT}/tools/run_test.py" -- '
            if mutation:command+=f'"{sys.executable}" "{negative}" '
            cmake+=f'add_test(NAME {target} COMMAND {command}$<TARGET_FILE:{target}> "{path}")\n'
        (project/'CMakeLists.txt').write_text(cmake);command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--source',str(project),'--output',str(args.output/'cmake'),'--scratch-root',str(scratch/'builds'),'--filter','^campaign_speech_cluster_mask_' ,'--jobs','2','--test-jobs','1','--cmake-arg=-DCMAKE_CXX_COMPILER='+args.compiler]
        for target in targets:command+=['--target',target]
        result=subprocess.run(command,check=False);report.update(command=command,exit_code=result.returncode)
        if args.native:
            actual=subprocess.run([str(args.native.resolve()),str(path)],text=True,capture_output=True);report['provided_native']=dict(path=str(args.native),exit_code=actual.returncode,stdout=actual.stdout[-4096:],stderr=actual.stderr[-4096:])
        report.update(passed=result.returncode==0 and(not args.native or actual.returncode==0),compiler=args.compiler,sanitizers=args.sanitizers,
            source_hashes_unchanged=all(sha(p)==report['source_hashes'][str(p.relative_to(ROOT))]for p in sources),mutations=['rebuild on same cached key','omit last cluster','admit equal/unordered threshold','logical rather than arithmetic mask word index','use bit0 instead of legacy bit7','retain count across bitmap clear','clear bitmap DWORD to1'],
            excluded=['whole203D80/117240 speech dispatch','unmapped input faults','unmasked SIMD traps','immutable dependency literal reads replaced by independently pinned constants','arbitrary SIMD scratch not consumed by actual caller'])
        report['passed']=report['passed']and report['source_hashes_unchanged']
    except BaseException as error:report['error']=str(error);raise
    finally:save(args.output/'report.json',report)
    print(json.dumps({k:report.get(k)for k in ('passed','original_cases','ordered_original_reads','ordered_original_stores','original_instructions')}));return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
