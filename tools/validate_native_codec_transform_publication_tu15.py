#!/usr/bin/env python3
"""Compare the complete private 283E10 raw frame with unchanged TU1.5."""
import argparse
import copy
import inspect
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_codec_transform_packed_original import ENTRIES, packed as packed_observer
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_transform_chain_tu15 import (
    HARNESS, REPEATS as CHILD_REPEATS, REGIONS, ROOT, STOP, cases, original_owners, sha,
    composition_owners, inverse_owners, product_owners,
)
from validate_native_codec_transform_packed_tu15 import PATTERNS, seeds as packed_seeds

ENTRY, EMMS = 0x283e10,0x283f1b
REPEATS = {*CHILD_REPEATS,0x283fa7}
RMW = {0x283fc8,0x283ff2}
OUTPUT_REGIONS = ((0x50e000,8192),(0x56e000,4096))
ALL_REGIONS = tuple(sorted((*REGIONS,*OUTPUT_REGIONS)))
FRAME = 'src/physics/runtime/native_tu15_codec_transform_publication_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_transform_publication_kernel.hpp'
DEFECTS = (
    ('cached-row-flags', FRAME, (
        ('low_byte(0x283fadu,0u,g[6]+2u);','low_byte(0x283fadu,0u,g[6]+2u);const auto cached_flags=g[0]&255u;'),
        ('s_.flags.right(g[1]&255u,1u,1u,false)','s_.flags.right(cached_flags,1u,1u,false)'),
        ('s_.flags.right(g[1]&255u,2u,1u,false)','s_.flags.right(cached_flags,2u,1u,false)'),
    )),
    ('missing-rmw-retirement', FRAME, (('[&]{s_.flags=committed;}','[]{}'),)),
    ('wrong-third-plane', FRAME, (('io_.store(0x284029u,0x56e040u+g[0]','io_.store(0x284029u,0x56e020u+g[0]'),)),
    ('missing-published-marker', FRAME, (('io_.store(0x28402fu,g[4]+27u,1u,1u);','io_.store(0x28402fu,g[4]+27u,1u,0u);'),)),
    ('single-input-row', KERNEL, (('} while(ancestry.advance_rejected());','} while(false);'),)),
)


def physical_original_full(*args,opaque_input=False,**kwargs):
    if tuple(kwargs.get('entries',()))!=ENTRIES:
        raise ValueError('Wrong original publication/child owner set')
    source=inspect.getsource(packed_observer.physical_original_rotation)
    old_topology='Original packed rotation did not complete its EMMS interval");}\''
    changes=(
        ('!=(0x296180,)',f'!={ENTRIES!r}'),
        ('pc<=0x2961a5u','!executed.contains(0x283f1bu)'),
        ('std::uint32_t(regs.rip)<=0x2961a5u','!executed.contains(0x283f1bu)'),
        ('replacement=\'if(finished){check(fp.cwd', 'replacement=\'if(finished){if(executed.contains(0x283f1bu)){check(fp.cwd'),
        (old_topology,'Original 283E10 did not retire EMMS");}else{\'+topology[len(\'if(finished){\'):-1]+\'}}\''),
    )
    for old,new in changes:
        if source.count(old)!=1:
            raise ValueError('Complete publication observer seam drift: '+old)
        source=source.replace(old,new,1)
    namespace=dict(vars(packed_observer));namespace['__file__']=__file__
    exec(compile(source,__file__,'exec'),namespace)
    result,proof=namespace['physical_original_rotation'](*args,opaque_input=opaque_input,**kwargs)
    proof['topology']='Exact retired EMMS or unchanged input topology on genuine no-decode returns'
    return result,proof


def harness(scratch):
    text=(ROOT/HARNESS).read_text()
    changes=(
        ('physics/runtime/native_tu15_codec_transform_composition_frame.hpp',FRAME.removeprefix('src/')),
        ('return pc==0x12058eu||pc==0x1205a3u;',
         'return pc==0x12058eu||pc==0x1205a3u||pc==0x283eb0u||pc==0x283fa7u||pc==0x283fc8u||pc==0x283ff2u;'),
        ('namespace compose=halo2::native_codec_transform_composition_detail;',
         'namespace compose=halo2::native_codec_transform_publication_detail;'),
        ('compose::run_private(', 'compose::run_private_complete('),
        ('            check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": composition ordered I/O differs");',
         '            if(memory.reads!=row.at("reads")||memory.writes!=row.at("writes")) {\n'
         '                for(const auto* key:{"reads","writes"}) {\n'
         '                    const J& expected=row.at(key);\n'
         '                    const J& actual=std::string_view(key)=="reads"?memory.reads:memory.writes;\n'
         '                    if(expected==actual)continue;\n'
         '                    std::cerr<<key<<" counts "<<expected.size()<<"/"<<actual.size()<<\'\\n\';\n'
         '                    for(std::size_t n=0;n<std::min(expected.size(),actual.size());++n)if(expected[n]!=actual[n]) {\n'
         '                        std::cerr<<"expected "<<expected[n].dump()<<" actual "<<actual[n].dump()<<\'\\n\';break;\n'
         '                    }\n'
         '                }\n'
         '                throw std::runtime_error(name+": publication ordered I/O differs");\n'
         '            }'),
    )
    for old,new in changes:
        count=2 if old=='compose::run_private(' else 1
        if text.count(old)!=count:raise ValueError('Publication harness seam drift: '+old)
        text=text.replace(old,new)
    cpp=scratch/'codec_transform_publication.cpp';cpp.write_text(text);return cpp


def publication_rows(image,smoke):
    rng=random.Random(0x283f7b)
    rows=packed_seeds(image,smoke)
    rows.extend(row for row in cases(image,smoke) if int(row['name'].split('-')[1]) in (0,1,2,3,7,10))
    for row in tuple(rows):
        if row['name'].startswith('packed-chain-4-'):
            for mode in (13,14,15,16):
                other=copy.deepcopy(row);other['name']=other['name'].replace('chain-4-','chain-'+str(mode)+'-',1)
                rows.append(other)
    for n,row in enumerate(rows):
        mode,profile=map(int,row['name'].split('-')[-2:])
        row.update(id=n,name='publication-'+row['name'],max_steps=1900)
        row.pop('stop_before_entries',None)
        # Keep stopped_at in the shared observer, but allow the genuine RET.
        row['stop_before_entries']=[]
        banks={part['address']:bytearray.fromhex(part['before'])for part in row['banks']}
        banks.update({base:bytearray(rng.randbytes(size))for base,size in OUTPUT_REGIONS})
        def put(address,value,width=4):
            for base,bank in banks.items():
                if base<=address<=base+len(bank)-width:
                    bank[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                    return
            raise ValueError('Publication input escaped declared backing')
        ancestry,alternative,records,description,transforms=(0x61000800,0x61000980,0x61000300,0x61000200,0x61000c00)
        if mode in (4,5,6,8,9,11,12,13,14,15,16):
            start=6 if mode==6 else -1 if mode==4 and profile==0 else 17 if mode==15 else 18 if mode==16 else 3
            for index in (1,2,4):put(ancestry+index*32+6,start,2)
            for table in (ancestry,alternative):
                for index,parent in ((3,4 if mode in (5,8,13) else -1),(4,-1),(6,3)):
                    put(table+index*32+4,parent,2)
            row_address=records+(28 if mode==11 else 0)
            put(row_address+2,(0,7,2,5,1,6,3,4)[profile%8],1)
            if mode==12:
                bp=row['returned_bp'];put(bp,0x6c6c1234);put(bp+4,STOP)
                put(bp+12,description);put(bp+16,0);put(bp+20,6);put(bp+24,transforms)
            if mode==13:
                put(description+100,2);put(records+28,1,2);put(records+30,7,1)
                for lane,value in enumerate(PATTERNS[(profile+3)%len(PATTERNS)]):
                    put(records+32+lane*2,value,2)
                row['count']=2;row['two_decodes']=True
            if mode==14:
                at=0x50ef10+3*32
                data=banks[0x61000000][records-0x61000000:records-0x61000000+28]
                for offset,value in enumerate(data):put(at+offset,value,1)
                put(at+2,7,1);put(description+104,at)
                row['row_output_alias']=at
            if mode in (15,16):
                # The actual earlier OR changes the source flag byte used
                # by the next plane. The later destination starts cleared.
                at,target,flags=(0x56e000,0x56e020,1) if mode==15 else (0x56e020,0x56e040,2)
                data=banks[0x61000000][records-0x61000000:records-0x61000000+28]
                for offset,value in enumerate(data):put(at+offset,value,1)
                put(at+2,flags,1);put(target,0);put(description+104,at)
                put(row['gp'][4]+16,20);put(ancestry+start*32+4,-1,2)
                row['bitplane_alias']=dict(source=at,expected_flags=3 if mode==15 else 6,target=target,mask=1<<start)
        row['banks']=[dict(address=base,before=data.hex())for base,data in sorted(banks.items())]
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append')
    for option in ('smoke','prefixes','mmx','sanitizers','semantic-mutants'):p.add_argument('--'+option,action='store_true')
    args=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/FRAME,ROOT/KERNEL,ROOT/HARNESS,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                complete_raw_candidate=True,readonly_gp_xmm_before_provider_domain=True,
                raw_rmw_retirement='Read and write are one original instruction; flags retire after write.',
                post_read_write_provider_failure_qualified=False,retained_source_suffix_qualified=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        source=(original_owners(),composition_owners(),inverse_owners(),product_owners())
        parent=sorted(_trace(code,ENTRY,set(ENTRIES)).code)
        if len(parent)!=196 or any(pcs!=parent for pcs in source[0].values()):
            raise ValueError('Exact original publication parent changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old=common.metadata;common.ENTRIES=ENTRIES
        def metadata(ref):
            result=old(ref)
            for site in result['sites']:
                pc=site['instruction'];operation=code.at(pc).mnemonic
                if operation in ('pop','ret'):
                    if site['operands']:raise ValueError('Unexpected explicit publication POP/RET operand')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if pc==0x283e6b:
                    if operation!='test' or code.at(pc).op_str!='dword ptr [edi + esi*4], ebx' or len(site['operands'])!=1:
                        raise ValueError('Exact membership TEST changed')
                    site['operands'][0].update(read=True,write=False)
                if pc in REPEATS:
                    if operation!='rep movsd':raise ValueError('Exact publication REP changed')
                    site['operands']=[dict(base=6,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False),
                                      dict(base=7,index=-1,scale=1,displacement=0,bytes=4,read=False,write=True)]
                if pc in RMW and (operation!='or' or len(site['operands'])!=1 or not
                                   (site['operands'][0]['read'] and site['operands'][0]['write'])):
                    raise ValueError('Exact publication OR RMW changed')
            return result
        common.metadata=metadata
        trace,stores=[],[]
        for site in metadata(image)['sites']:
            pc=site['instruction'];implicit=code.at(pc).mnemonic in ('push','call')
            if site['operands']or implicit:trace.append(dict(entry=pc,gp=True,xmm=True))
            if pc not in REPEATS|RMW and (implicit or any(op['write']for op in site['operands'])):stores.append(pc)
        rows=publication_rows(image,args.smoke);normal=len(rows);proofs=[]
        if args.mmx:
            rng=random.Random(ENTRY^0xa11)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                rows.append(row)
        opaque_count=len(rows)-normal
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            observed,proof=physical_original_full(image,batch,folder,entries=ENTRIES,
                regions=ALL_REGIONS+(((0x64000000,4096),)if args.mmx else()),
                trace_calls=trace,trace_extended=True,opaque_input=args.mmx)
            proofs.append(proof);return observed['cases']
        original=measure(rows,'normal')
        if args.prefixes:
            pending,seen=[],set()
            for row,observed in zip(rows,original,strict=True):
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[stream]):
                        pc=event['instruction']
                        if not 0x283f7b<=pc<0x28404a:continue
                        if phase=='before_write'and pc in REPEATS|RMW:continue
                        signature=('active_mmx'in row,phase,pc)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(id=len(rows)+len(pending),name=row['name']+f'-{phase}-{pc:x}',phase=phase,
                                      fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            rows+=pending;original+=measure(pending,'publication-prefixes')
        covered=set();counts=dict(empty=0,normal=0,changed_row_flags=0,two_decodes=0,bitplane_feedback=0)
        for row,observed in zip(rows,original,strict=True):
            covered.update(observed['executed_sites'])
            if observed['completed']!=('phase'not in row):raise ValueError('Original publication completion differs')
            if 'phase'in row:pc=observed['boundary']['instruction']
            else:
                if observed['stopped_at']!=STOP:raise ValueError('Original publication did not take a real RET')
                pc=0x284081 if row['count']<=0 else 0x284076
                if pc not in observed['executed_sites']:raise ValueError('Original publication RET identity changed')
                counts['empty' if row['count']<=0 else 'normal']+=1
                if row.get('two_decodes'):
                    if sum(packet['entry']==0x283f04 for packet in observed['calls'])!=2:
                        raise ValueError('Actual double-decode publication witness failed')
                    counts['two_decodes']+=1
                if (alias:=row.get('row_output_alias')):
                    actual=next(bank for bank in observed['banks']if bank['address']==0x50e000)
                    flags=bytes.fromhex(actual['after'])[alias+2-0x50e000]
                    if flags!=7:counts['changed_row_flags']+=1
                if (alias:=row.get('bitplane_alias')):
                    after=bytes.fromhex(next(bank for bank in observed['banks']if bank['address']==0x56e000)['after'])
                    source_offset=alias['source']+2-0x56e000;target_offset=alias['target']-0x56e000
                    if after[source_offset]!=alias['expected_flags'] or not(int.from_bytes(after[target_offset:target_offset+4],'little')&alias['mask']):
                        raise ValueError('Genuine original cross-plane flag feedback changed: '+row['name'])
                    counts['bitplane_feedback']+=1
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],
                fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if 'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected publication trace arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Publication fixture bank order changed')
                bank['after']=after['after']
        if covered&set(parent)!=set(parent) or not all(counts.values()):
            raise ValueError('Complete original publication witness/coverage missing: '+str(sorted(set(parent)-covered)))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        mutant_fixture=scratch/'mutant-fixture.json'
        if args.semantic_mutants:
            names={'publication-packed-chain-5-1','publication-packed-chain-13-1',
                   'publication-packed-chain-15-1','publication-packed-chain-16-1'}
            focused=[row for row in rows if row['name']in names]
            if len(focused)!=len(names):raise ValueError('Exact publication compiled defect witnesses missing')
            mutant_fixture.write_text(json.dumps(dict(cases=focused,write_sites=stores),separators=(',',':')))
        cpp=harness(scratch)
        report.update(physical_paths=len(rows),normal_paths=normal,opaque_paths=opaque_count,parent_sites=len(parent),
            child_sites=len(covered-set(parent)),fault_paths=sum('phase'in row for row in rows),
            authentic_publication=counts,original=proofs,generated_harness_sha256=sha(cpp))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),
                 '-MD','-MF',str(dep)]
            if args.sanitizers:cmd.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Publication compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Original publication comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(result)
            print(f'{compiler} complete publication comparison passed',flush=True)
            if args.semantic_mutants:
                for name,relative,replacements in DEFECTS:
                    text=(ROOT/relative).read_text()
                    for old,new in replacements:
                        if text.count(old)!=1:raise ValueError('Publication defect seam changed: '+name)
                        text=text.replace(old,new,1)
                    overlay=scratch/(Path(compiler).name+'-'+name)
                    header=overlay/relative;header.parent.mkdir(parents=True);header.write_text(text)
                    mutant=overlay/'probe'
                    changed=[cmd[0],'-I'+str(overlay/'include'),'-I'+str(overlay/'src'),*cmd[1:]]
                    changed[changed.index('-o')+1]=str(mutant);changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Publication mutant compile failed: '+name+' '+done.stderr[-8000:])
                    done=subprocess.run([str(mutant),str(mutant_fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'publication-'not in diagnostic:
                        raise ValueError('Publication defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected publication defect {name}',flush=True)
        if (original_owners(),composition_owners(),inverse_owners(),product_owners())!=source:
            raise ValueError('Exact publication/child owner drift')
        report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:
        used.update(Path(module.__file__).resolve()for module in tuple(sys.modules.values())
            if getattr(module,'__file__',None)and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path]for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name)for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
