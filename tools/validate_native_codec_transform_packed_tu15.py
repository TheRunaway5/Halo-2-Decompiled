#!/usr/bin/env python3
"""Compare 283E10's packed phase and real children to unchanged TU1.5."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import shlex
import subprocess
import sys
from pathlib import Path

from native_codec_transform_packed_original import ENTRIES, physical_original_packed
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_transform_chain_tu15 import HARNESS, REPEATS, REGIONS, ROOT, cases, original_owners, sha
from validate_native_codec_transform_chain_tu15 import composition_owners, inverse_owners, product_owners

ENTRY, EMMS, AFTER_EMMS, STOP = 0x283e10,0x283f1b,0x283f1d,0x283f7b
FRAME = 'src/physics/runtime/native_tu15_codec_transform_packed_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_transform_packed_kernel.hpp'
PATTERNS = ((0,0,0,0),(-32768,32767,-1,1),(1,-2,3,-4),(0,0,0,1),
            (32767,-32768,32767,-32768),(1,1,1,1),(-12345,22222,30000,-789))
DEFECTS = (
    ('missing-emms', 's_.emms();s_.fp.top=0;', '(void)s_;'),
    ('lost-pre-movlh-upper-pair', 'x[1]=_mm_movelh_ps(converted,_mm_movehl_ps(x[1],x[1]));', 'x[1]=converted;'),
    ('wrong-norm-pair', '_mm_shuffle_ps(value,value,78)', '_mm_shuffle_ps(value,value,17)'),
    ('wrong-reciprocal-estimate', 'rsqrtps %0,%0', 'sqrtps %0,%0'),
    ('wrong-second-child-formal', 'g[0]=g[4]+116u;', 'g[0]=g[4]+112u;'),
)


def harness(scratch):
    text=(ROOT/HARNESS).read_text()
    changes=(
        ('#include "physics/runtime/native_tu15_codec_transform_composition_frame.hpp"',
         '#include "physics/runtime/native_tu15_codec_transform_chain_frame.hpp"\n#include "'+FRAME.removeprefix('src/')+'"'),
        ('namespace {\nstruct CompositionMemory', 'namespace {\nstruct EmmsBoundary {};\nstruct CompositionMemory'),
        ('return pc==0x12058eu||pc==0x1205a3u;', 'return pc==0x283eb0u||pc==0x12058eu||pc==0x1205a3u;'),
        ('bool interrupted{};','bool interrupted{},at_emms{};'),
        ('''                namespace compose=halo2::native_codec_transform_composition_detail;
                if(use_default)compose::run_private(state);
                else compose::run_private(state,[&](auto&,auto pc){
                    check(memory.pc==pc,"Composition cursor differs");if(noisy)hostile();
                });''',
         '''                namespace ancestry=halo2::native_codec_transform_chain_detail;
                namespace packed=halo2::native_codec_transform_packed_detail;
                const auto run=[&](auto before) {
                    check(ancestry::run_private_prefix(state,before)==ancestry::Boundary::before_packed_decode,
                          "Packed source row did not reach its actual ancestry frontier");
                    packed::run_private_suffix(state,[&](auto& s,auto pc) {
                        before(s,pc);
                        if(pc==0x283f1du&&row.contains("stop_after_emms"))throw EmmsBoundary{};
                    });
                };
                if(use_default)run(packed::Noop{});
                else run([&](auto&,auto pc){
                    check(memory.pc==pc,"Packed cursor differs");if(noisy)hostile();
                });'''),
        ('} catch(const Fault&) {interrupted=true;}', '} catch(const Fault&) {interrupted=true;}\n'
         '              catch(const EmmsBoundary&) {at_emms=true;}'),
        ('            const auto actual=packet(state,row.at("expected").at("entry"));',
         '            check(at_emms==row.contains("stop_after_emms"),name+": packed EMMS retirement differs");\n'
         '            const auto actual=packet(state,row.at("expected").at("entry"));'),
    )
    for old,new in changes:
        if text.count(old)!=1:raise ValueError('Exact packed composition harness seam drift: '+old)
        text=text.replace(old,new,1)
    path=scratch/'codec_transform_packed.cpp';path.write_text(text);return path


def seeds(image,smoke):
    rows=[]
    for row in cases(image,smoke):
        mode=int(row['name'].split('-')[1])
        if mode not in (4,5,6,8,9,11,12):continue
        n=len(rows);row.update(id=n,name='packed-'+row['name'],stop_before_entries=[STOP],max_steps=1300)
        source=next(bank for bank in row['banks'] if bank['address']==0x61000000)
        data=bytearray.fromhex(source['before'])
        offset=0x304+(28 if mode==11 else 0)
        for lane,value in enumerate(PATTERNS[n%len(PATTERNS)]):
            data[offset+lane*2:offset+lane*2+2]=(value&0xffff).to_bytes(2,'little')
        source['before']=data.hex();rows.append(row)
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append')
    for arg in ('smoke','mmx','prefixes','sanitizers','semantic-mutants'):p.add_argument('--'+arg,action='store_true')
    args=p.parse_args()
    if 'HALO2_TEST_SCRATCH'not in os.environ:p.error('Use tools/run_test.py')
    if platform.system()!='Linux'or platform.machine().lower()not in ('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    before={path.resolve():sha(path)for tree in ('tools','src','include','tests')for path in (ROOT/tree).rglob('*')
            if path.is_file()and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used={ROOT/FRAME,ROOT/KERNEL,ROOT/HARNESS,Path(__file__).resolve(),ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[],
                retirement='after actual 283F1B EMMS before 283F1D MOVLHPS',
                end='after actual second 120570 return, before 283F7B output ancestry load',
                intermediate_mmx_qualified=False,output_or_bitplane_publication_qualified=False)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        owners=(original_owners(),composition_owners(),inverse_owners(),product_owners())
        parent=sorted(_trace(code,ENTRY,set(ENTRIES)).code)
        if len(parent)!=196 or any(pcs!=parent for pcs in owners[0].values()):
            raise ValueError('Original packed parent/source sites changed')
        if [(pc,code.at(pc).mnemonic,code.at(pc).op_str)for pc in (EMMS,0x283f3a,0x283f76)]!=[
                (EMMS,'emms',''),(0x283f3a,'rsqrtps','xmm0, xmm0'),(0x283f76,'call','0x120570')]:
            raise ValueError('Exact packed original instruction/child identity changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old=common.metadata;common.ENTRIES=ENTRIES
        def metadata(ref):
            result=old(ref)
            for site in result['sites']:
                pc=site['instruction'];operation=code.at(pc).mnemonic
                if operation in ('pop','ret'):
                    if site['operands']:raise ValueError('Unexpected explicit packed POP/RET operand')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if pc==0x283e6b:
                    if operation!='test' or code.at(pc).op_str!='dword ptr [edi + esi*4], ebx' or len(site['operands'])!=1:
                        raise ValueError('Exact earlier membership TEST changed')
                    site['operands'][0].update(read=True,write=False)
                if pc in REPEATS:
                    if operation!='rep movsd':raise ValueError('Exact packed child REP changed')
                    site['operands']=[dict(base=6,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False),
                                      dict(base=7,index=-1,scale=1,displacement=0,bytes=4,read=False,write=True)]
            return result
        common.metadata=metadata
        trace,stores=[],[]
        for site in metadata(image)['sites']:
            pc=site['instruction'];implicit=code.at(pc).mnemonic in ('push','call')
            if site['operands']or implicit:trace.append(dict(entry=pc,gp=True,xmm=True))
            if pc not in REPEATS and (implicit or any(op['write']for op in site['operands'])):stores.append(pc)
        rows=seeds(image,args.smoke);normal=len(rows);proofs=[]
        if args.mmx:
            rng=random.Random(ENTRY^EMMS)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                rows.append(row)
        opaque_count=len(rows)-normal
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            result,proof=physical_original_packed(image,batch,folder,entries=ENTRIES,
                regions=REGIONS+(((0x64000000,4096),)if args.mmx else()),
                trace_calls=trace,trace_extended=True,opaque_input=args.mmx)
            proofs.append(proof);return result['cases']
        original=measure(rows,'normal')
        pending,seen=[],set()
        for row,observed in zip(rows,original,strict=True):
            # Keep separate original stops after EMMS. They expose preserved
            # upper XMM lanes and actual empty TOP0 before MOVLHPS.
            if args.smoke or row['id']%4==0:
                witness=copy.deepcopy(row)
                witness.update(id=0,name=row['name']+'-actual-emms',stop_after_emms=True,stop_before_entries=[AFTER_EMMS])
                pending.append(witness)
            if args.prefixes:
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[stream]):
                        pc=event['instruction']
                        if not 0x283ee9<=pc<STOP:continue
                        signature=('active_mmx'in row,phase,pc)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(id=0,name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
        for index,row in enumerate(pending,len(rows)):row['id']=index
        rows+=pending;original+=measure(pending,'retired-and-prefixes')
        covered=set();retired=0;boundary_packets=[]
        for row,observed in zip(rows,original,strict=True):
            covered.update(observed['executed_sites'])
            if observed['completed']!=('phase'not in row):raise ValueError('Actual packed-phase completion differs')
            pc=observed['boundary']['instruction']if 'phase'in row else observed['stopped_at']
            if row.get('stop_after_emms'):
                if pc!=AFTER_EMMS or EMMS not in observed['executed_sites'] or observed['boundary_live_x87'] or 'active_mmx'in observed or observed['after_fp'][1]&0x3800:
                    raise ValueError('Actual EMMS retirement witness changed')
                retired+=1
                if len(boundary_packets)<4:
                    boundary_packets.append(dict(name=row['name'],status=observed['after_fp'][1],xmm=observed['boundary_xmm']))
            elif 'phase'not in row and (pc!=STOP or EMMS not in observed['executed_sites']):
                raise ValueError('Actual packed owner did not reach its genuine child-return frontier')
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],
                fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if 'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected packed trace arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Packed fixture bank order changed')
                bank['after']=after['after']
        scope={pc for pc in parent if 0x283ee9<=pc<STOP}
        if covered&scope!=scope or STOP in covered or not retired:
            raise ValueError('Original packed-phase scope changed')
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        mutant_fixture=scratch/'mutant-fixture.json'
        if args.semantic_mutants:
            upper_witness=next((row for row in rows if row.get('stop_after_emms') and
                row['expected']['xmm'][1][2:]!=row['expected']['xmm'][2][:2]),None)
            if upper_witness is None:raise ValueError('Actual original EMMS upper-pair defect witness missing')
            wanted={'packed-chain-4-0','packed-chain-4-1','packed-chain-4-4','packed-chain-12-0',upper_witness['name']}
            focused=[row for row in rows if row['name']in wanted]
            if len(focused)!=len(wanted):raise ValueError('Packed defect witnesses missing')
            mutant_fixture.write_text(json.dumps(dict(cases=focused,write_sites=stores),separators=(',',':')))
            report['upper_pair_defect_witness']=dict(name=upper_witness['name'],
                original_upper=upper_witness['expected']['xmm'][1][2:],
                original_converted=upper_witness['expected']['xmm'][2][:2])
        cpp=harness(scratch)
        report.update(physical_paths=len(rows),normal_paths=normal,opaque_paths=opaque_count,exact_emms_witnesses=retired,
                      packed_phase_sites=len(scope),fault_paths=sum('phase'in row for row in rows),original=proofs,
                      authentic_after_emms_packets=boundary_packets,generated_harness_sha256=sha(cpp))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),
                 '-MD','-MF',str(dep)]
            if args.sanitizers:cmd.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Packed-phase compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Packed-phase original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(result)
            print(f'{compiler} full packed phase passed',flush=True)
            if args.semantic_mutants:
                for name,old,new in DEFECTS:
                    text=(ROOT/FRAME).read_text()
                    if text.count(old)!=1:raise ValueError('Packed defect seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name)
                    header=overlay/FRAME
                    header.parent.mkdir(parents=True)
                    header.write_text(text.replace(old,new,1));mutant=overlay/'probe'
                    changed=[cmd[0],'-I'+str(overlay/'src'),*cmd[1:]]
                    changed[changed.index('-o')+1]=str(mutant);changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Packed defect compile failed: '+name+' '+done.stderr[-8000:])
                    done=subprocess.run([str(mutant),str(mutant_fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'packed-chain-' not in diagnostic:
                        raise ValueError('Packed defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected packed defect {name}',flush=True)
        if (original_owners(),composition_owners(),inverse_owners(),product_owners())!=owners:
            raise ValueError('Exact original packed/child body drift')
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
