#!/usr/bin/env python3
"""Qualify phase-one 283E10 with real original/native children, before MMX."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from validate_native_codec_inverse_transform_tu15 import owner_bodies as inverse_owners
from validate_native_codec_transform_composition_tu15 import ROOT, original_owners as composition_owners, product_owners, sha

ENTRY, PACKED, STOP = 0x283e10,0x283ee9,0x63000000
ENTRIES = (ENTRY,0x120570,0x120740,0x1201e0)
REPEATS = {0x283eb0,0x12058e,0x1205a3}
REGIONS = ((0x46a000,4096),(0x61000000,8192),(0x62000000,8192))
FRAME = 'src/physics/runtime/native_tu15_codec_transform_chain_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_transform_chain_kernel.hpp'
HARNESS = 'tests/native_tu15_codec_transform_composition_tests.cpp'
OWNERS = {
    'src/physics/physics_live_support_tu15_generated.cpp':'a1effb5931bce53df82da55ab92fa43bc619ab8f6144380e1248d50fec976efd',
    'src/campaign/ai/source_ai_frame_tu15_generated.cpp':'95147817e456cb4204db4f0e5595839055ccc424262345794e172fd765dccd1c',
}
DEFECTS = (
    ('wrong-membership-mask',FRAME,'s_.flags.logic(g[0]&31u,4u)','s_.flags.logic(g[0]&30u,4u)'),
    ('forward-only-stage',FRAME,'g[6]+=delta;g[7]+=delta;--g[1];','g[6]+=4u;g[7]+=delta;--g[1];'),
    ('single-parent-only',KERNEL,'while(view.has_parent())view.compose_parent();','if(view.has_parent())view.compose_parent();'),
    ('wrong-inverse-scratch',FRAME,'g[1]=g[4]+112u;g[2]=g[4]+80u;','g[1]=g[4]+116u;g[2]=g[4]+80u;'),
    ('stale-returned-bp',FRAME,
     'scope_.child([&]{native_codec_transform_composition_detail::run_private(s_,std::ref(before_));});',
     'const auto incoming_bp=g[5];\n        scope_.child([&]{native_codec_transform_composition_detail::run_private(s_,std::ref(before_));});\n'
     '        g[5]=incoming_bp;'),
)


def original_owners():
    result={}
    for name,pin in OWNERS.items():
        bodies=re.findall(r'^static void run_283e10\(WorldState &s\) \{.*?^\}',(ROOT/name).read_text(),re.S|re.M)
        if len(bodies)!=1 or hashlib.sha256(bodies[0].encode()).hexdigest()!=pin:
            raise ValueError('Exact 283E10 composition-chain source owner changed: '+name)
        result[name]=list(dict.fromkeys(int(pc,16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',bodies[0])))
    return result


def harness(scratch):
    text=(ROOT/HARNESS).read_text()
    for old,new in (
        ('physics/runtime/native_tu15_codec_transform_composition_frame.hpp',FRAME.removeprefix('src/')),
        ('return pc==0x12058eu||pc==0x1205a3u;', 'return pc==0x283eb0u||pc==0x12058eu||pc==0x1205a3u;'),
        ('bool interrupted{};', 'bool interrupted{};unsigned stopped{};'),
        ('namespace compose=halo2::native_codec_transform_composition_detail;', 'namespace compose=halo2::native_codec_transform_chain_detail;'),
        ('if(use_default)compose::run_private(state);', 'if(use_default)stopped=unsigned(compose::run_private_prefix(state));'),
        ('else compose::run_private(state,[&](auto&,auto pc){', 'else stopped=unsigned(compose::run_private_prefix(state,[&](auto&,auto pc){'),
        ('check(memory.pc==pc,"Composition cursor differs");if(noisy)hostile();\n                });',
         'check(memory.pc==pc,"Chain cursor differs");if(noisy)hostile();\n                }));'),
        ('            const auto actual=packet(state,row.at("expected").at("entry"));',
         '            if(!interrupted)check(stopped==row.at("boundary_kind"),name+": incorrect ancestry composition boundary");\n'
         '            const auto actual=packet(state,row.at("expected").at("entry"));'),
        ('            check(memory.reads==row.at("reads")&&memory.writes==row.at("writes"),name+": composition ordered I/O differs");',
         '            if(memory.reads!=row.at("reads")||memory.writes!=row.at("writes")) {\n'
         '                for(const auto* key:{"reads","writes"}) {\n'
         '                    const J& expected=row.at(key);\n'
         '                    const J& observed=std::string_view(key)=="reads"?memory.reads:memory.writes;\n'
         '                    if(expected==observed)continue;\n'
         '                    std::cerr<<key<<" counts "<<expected.size()<<"/"<<observed.size()<<\'\\n\';\n'
         '                    for(std::size_t n=0;n<std::min(expected.size(),observed.size());++n)if(expected[n]!=observed[n]) {\n'
         '                        std::cerr<<"expected "<<expected[n].dump()<<" actual "<<observed[n].dump()<<\'\\n\';break;\n'
         '                    }\n'
         '                }\n'
         '                throw std::runtime_error(name+": ancestry ordered I/O differs");\n'
         '            }'),
    ):
        if text.count(old)!=1:raise ValueError('Shared ancestry composition harness seam changed: '+old)
        text=text.replace(old,new,1)
    cpp=scratch/'codec_transform_chain.cpp';cpp.write_text(text);return cpp


def cases(image,smoke):
    rng=random.Random(ENTRY);rows=[]
    live=[(0x8000000000000000|rng.getrandbits(60)).to_bytes(8,'little').hex()+'ff3f'for _ in range(8)]
    constants=image.read(0x46a000,4096)
    root,description,records,remap,planes,indices,ancestry,transforms=(
        0x61000100,0x61000200,0x61000300,0x61000500,0x61000600,0x61000700,0x61000800,0x61000c00)
    values=(0,0x80000000,1,0x80000001,0x00800000,0x3f800000,0xbf800000,0x3e800000,
            0x7f7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc23456,0x7f812345,0xff823456,0x3eaaaaab)
    record_sets=((),(),(-1,),(-1,7),(2,),(2,),(2,),(1,),(1,),(-1,),(1,),(7,2),(2,))
    for mode,record_set in enumerate(record_sets):
        for profile in range(4 if smoke else 16):
            n=len(rows);sp=0x62000400+n%16
            bank={base:bytearray(constants if base==0x46a000 else rng.randbytes(size))for base,size in REGIONS}
            def put(address,value,width=4):
                for base,data in bank.items():
                    if base<=address<=base+len(data)-width:
                        data[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')
                        return
                raise ValueError('Chain input escaped declared backing')
            mapped=remap if 7<=mode<=10 else 0
            count=(-3 if mode==1 else len(record_set))
            for lane,word in enumerate((STOP,root,description,mapped,6,transforms)):put(sp+lane*4,word)
            put(root+16,ancestry);put(description+100,count);put(description+104,records)
            put(remap+12,indices);put(remap+20,planes);put(planes,0 if mode==7 else 2)
            put(planes-4,0x80000000);put(indices-2,2,2);put(indices+2,-1 if mode==10 else 2,2)
            for index,node in enumerate(record_set):put(records+index*28,node,2)
            for index in range(6):
                parent=3 if index==2 and mode in (5,6,8,9,12) else 4 if index==3 and mode in (6,8) else -1
                put(ancestry+index*32+8,parent,2)
                for lane in range(8):put(transforms+index*32+lane*4,values[(profile+index*3+lane*7)%len(values)])
            gp=[rng.getrandbits(32)for _ in range(8)];gp[4]=sp
            cw=(0x7f,0x27f,0x37f)[mode%3]|((profile%4)<<10)
            sw=((n%8)<<11)|((n<<7)&0x4700)|(n&0x3f)
            mx=0x1f80|((profile%4)<<13)|(n&0x3f)|(0,0x40,0x8000,0x8040)[mode%4]
            returned_alias={}
            if mode==12:
                # First-stage backward REP copies these words into the outer
                # scratch, then the real 120570 right-output alias copies
                # them onto its own saved registers. No child packet is set.
                alternate_bp,alternate_root,alternate_ancestry=sp+0xc0,root+0x80,ancestry+0x180
                put(alternate_bp+8,alternate_root);put(alternate_root+16,alternate_ancestry)
                put(alternate_ancestry+4*32+8,-1,2)
                put(transforms+2*32-4,ancestry+4*32)
                put(transforms+2*32-8,alternate_bp)
                put(transforms+2*32-12,4*32)
                put(transforms+2*32-16,0x6c6c6c6c)
                returned_alias=dict(returned_bp=alternate_bp,returned_si=4*32)
            df=0x400 if mode==12 or profile&2 and mode in (0,1,2,3,4,7,10)else 0
            rows.append(dict(id=n,name=f'chain-{mode}-{profile}',entry=ENTRY,gp=gp,matrix=root,source=description,
                count=count,eflags=0x202|(rng.getrandbits(12)&0x8d5)|df,
                xmm=[[rng.getrandbits(32)for _ in range(4)]for _ in range(8)],
                before_fp=[cw,sw,mx],live_x87=live[:(0,1,4,8)[mode%4]],max_steps=850,stop_before_entries=[PACKED],
                banks=[dict(address=base,before=data.hex())for base,data in bank.items()],**returned_alias))
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
                phase='entry to 283EE9 after genuine inverse or genuine no-row RET',source_owner_pins=OWNERS,
                repeat_before_write_contract_qualified=False,readonly_gp_xmm_before_provider_domain=True)
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        source=original_owners();children=(composition_owners(),inverse_owners(),product_owners())
        parent=sorted(_trace(code,ENTRY,set(ENTRIES)).code)
        if len(parent)!=196 or any(pcs!=parent for pcs in source.values()):
            raise ValueError('Exact original ancestry composition sites changed')
        if [(pc,code.at(pc).op_str)for pc in parent if code.at(pc).mnemonic=='call']!=[
                (0x283ec7,'0x120570'),(0x283ee4,'0x120740'),(0x283f76,'0x120570')]:
            raise ValueError('Exact genuine ancestry children changed')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        old=common.metadata;common.ENTRIES=ENTRIES
        def metadata(ref):
            result=old(ref)
            for site in result['sites']:
                pc=site['instruction'];operation=code.at(pc).mnemonic
                if operation in ('pop','ret'):
                    if site['operands']:raise ValueError('Unexpected explicit ancestry POP/RET operand')
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
                if pc==0x283e6b:
                    # Capstone does not mark either operand of this TEST.
                    # The authenticated memory operand is read before flags.
                    if operation!='test' or code.at(pc).op_str!='dword ptr [edi + esi*4], ebx' or len(site['operands'])!=1:
                        raise ValueError('Exact ancestry membership TEST changed')
                    site['operands'][0].update(read=True,write=False)
                if pc in REPEATS:
                    if operation!='rep movsd':raise ValueError('Exact ancestry REP changed')
                    site['operands']=[dict(base=6,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False),
                                      dict(base=7,index=-1,scale=1,displacement=0,bytes=4,read=False,write=True)]
            return result
        common.metadata=metadata
        trace,stores=[],[]
        for site in metadata(image)['sites']:
            pc=site['instruction'];implicit=code.at(pc).mnemonic in ('push','call')
            if site['operands']or implicit:trace.append(dict(entry=pc,gp=True,xmm=True))
            if pc not in REPEATS and (implicit or any(op['write']for op in site['operands'])):stores.append(pc)
        rows=cases(image,args.smoke);normal=len(rows);proofs=[]
        def measure(batch,label):
            folder=scratch/label;folder.mkdir()
            opaque='active_mmx'in batch[0];runner=physical_original_mmx if opaque else physical_original_dense
            observed,proof=runner(image,batch,folder,entries=ENTRIES,
                regions=REGIONS+(((0x64000000,4096),)if opaque else()),trace_calls=trace,trace_extended=True)
            proofs.append(proof);return observed['cases']
        original=measure(rows,'normal')
        if args.mmx:
            rng=random.Random(ENTRY^0xff);opaque=[]
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal]);row.update(id=len(rows)+len(opaque),name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                opaque.append(row)
            rows+=opaque;original+=measure(opaque,'opaque')
        if args.prefixes:
            seen,pending=set(),[]
            for row,observed in zip(rows,original,strict=True):
                for phase,stream in (('before_read','reads'),('before_write','writes'),('after_write','writes')):
                    for ordinal,event in enumerate(observed[stream]):
                        pc=event['instruction']
                        if phase=='before_write'and pc in REPEATS:continue
                        signature=('active_mmx'in row,phase,pc)
                        if signature in seen:continue
                        seen.add(signature);prefix=copy.deepcopy(row)
                        prefix.update(id=0,name=row['name']+f'-{phase}-{pc:x}',phase=phase,fault=dict(phase=phase,ordinal=ordinal))
                        pending.append(prefix)
            for opaque in (False,True):
                subset=[row for row in pending if ('active_mmx'in row)==opaque]
                if not subset:continue
                for index,row in enumerate(subset,len(rows)):row['id']=index
                rows+=subset;original+=measure(subset,'prefix-'+str(opaque))
        covered,returns,returned_aliases=set(),[0,0,0],[]
        for row,observed in zip(rows,original,strict=True):
            covered.update(observed['executed_sites'])
            if observed['completed']!=('phase'not in row):raise ValueError('Original ancestry phase completion differs')
            if 'phase'in row:
                pc=observed['boundary']['instruction']
            elif observed['stopped_at']==PACKED:
                pc=PACKED;row['boundary_kind']=2;returns[2]+=1
                if 'returned_bp'in row:
                    packet,= [packet for packet in observed['calls'] if packet['entry']==0x283ecc]
                    if packet['gp'][5:7]!=[row['returned_bp'],row['returned_si']]:
                        raise ValueError('Actual ancestry child failed to return the backed saved-register alias')
                    returned_aliases.append(dict(name=row['name'],gp=packet['gp']))
            elif observed['stopped_at']==STOP:
                pc=0x284081 if row['count']<=0 else 0x284076
                row['boundary_kind']=int(row['count']>0);returns[row['boundary_kind']]+=1
            else:raise ValueError('Original left the exact ancestry phase')
            row['expected']=dict(entry=pc,gp=observed['boundary_gp'],eflags=observed['boundary_eflags'],xmm=observed['boundary_xmm'],
                fp=observed['after_fp'],live_x87=observed.get('boundary_live_x87',[]))
            if 'active_mmx'in observed:row['expected']['active_mmx']=observed['active_mmx']
            row.update(reads=observed['reads'],writes=observed['writes'],calls=observed['calls'])
            for packet in row['calls']:
                if packet.pop('arguments'):raise ValueError('Unexpected ancestry trace arguments')
            for bank,after in zip(row['banks'],observed['banks'],strict=True):
                if bank['address']!=after['address']:raise ValueError('Ancestry fixture bank order changed')
                bank['after']=after['after']
        scope={pc for pc in parent if pc<PACKED or pc>=0x28404a}
        if covered&set(parent)!=scope or any(count==0 for count in returns)or not returned_aliases:
            raise ValueError('Ancestry phase path coverage incomplete: '+str(sorted(scope-covered)))
        fixture=scratch/'fixture.json';fixture.write_text(json.dumps(dict(cases=rows,write_sites=stores),separators=(',',':')))
        mutant_fixture=scratch/'mutant-fixture.json'
        if args.semantic_mutants:
            negative_rows=[row for row in rows if 'phase'not in row and
                           (re.fullmatch(r'chain-\d+-0',row['name'])or row['name']=='chain-4-2')]
            mutant_fixture.write_text(json.dumps(dict(cases=negative_rows,write_sites=stores),separators=(',',':')))
        cpp=harness(scratch)
        report.update(physical_paths=len(rows),normal_paths=normal,original_parent_sites=len(scope),
            original_child_sites=len(covered-scope),fault_paths=sum('phase'in row for row in rows),
            boundary_counts=dict(empty=returns[0],rejected=returns[1],before_packed=returns[2]),original=proofs,
            generated_harness_sha256=sha(cpp),returned_saved_register_aliases=returned_aliases)
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name;dep=binary.with_suffix('.d')
            cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                 '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(ROOT/'tests'),str(cpp),'-o',str(binary),
                 '-MD','-MF',str(dep)]
            if args.sanitizers:cmd.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(cmd,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Ancestry compile failed: '+done.stderr[-12000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Ancestry original comparison failed: '+(done.stdout+done.stderr)[-12000:])
            result=dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(result)
            print(f'{compiler} full ancestry comparison passed',flush=True)
            if args.semantic_mutants:
                for name,relative,old,new in DEFECTS:
                    text=(ROOT/relative).read_text()
                    if text.count(old)!=1:raise ValueError('Ancestry defect seam changed: '+name)
                    overlay=scratch/(Path(compiler).name+'-'+name)
                    header=overlay/relative;header.parent.mkdir(parents=True)
                    header.write_text(text.replace(old,new,1));mutant=overlay/'probe'
                    changed=[cmd[0],'-I'+str(overlay/'include'),'-I'+str(overlay/'src'),*cmd[1:]]
                    changed[changed.index('-o')+1]=str(mutant);changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    done=subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if done.returncode:raise ValueError('Ancestry mutant compile failed: '+name+' '+done.stderr[-8000:])
                    done=subprocess.run([str(mutant),str(mutant_fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'chain-'not in diagnostic:
                        raise ValueError('Ancestry defect survived: '+name+' '+diagnostic[-7000:])
                    result['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
                    print(f'{compiler} rejected ancestry defect {name}',flush=True)
        if original_owners()!=source or (composition_owners(),inverse_owners(),product_owners())!=children:
            raise ValueError('Exact ancestry/child source owner drift')
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
