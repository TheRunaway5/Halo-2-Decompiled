#!/usr/bin/env python3
"""Compare native BC720 with physical TU1.5 code on actual entry BEFORE banks.

Only capture-entry.json and its BEFORE files are inputs. All remaining callees
execute unchanged original instructions on each side's current mutable banks.
No native AFTER image, callee result, RNG, allocation or admission is supplied.
Run inside tools/run_test.py; binaries and raw banks remain in its owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tarfile
import re
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import complete_body
from replay_live_phantom_hardware_tu15 import ASM, reference_text_gaps


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


VARIANTS=['actual','model-none','type6-detached','type6-flag2','type6-refresh']+[f'count-{n}' for n in (0,1,2,3,4,7,8,12,15,16)]
MODEL_CASES={
    'actual':{},'count-zero':{'model_count':0},'count-negative':{'model_count':0xffffffff},
    'requested-none':{'formal_words':{4:0xffffffff}},'skip-lower':{'formal_words':{4:0}},
    'force-lower':{'formal_words':{4:0,5:1}},'force-upper-byte':{'formal_words':{4:0,5:0x100}},
    'all-regions':{'formal_words':{3:0xffffffff}},'other-region':{'formal_words':{3:1}},
    'filter-miss':{'formal_words':{3:0x7fffffff}},'variant-none':{'formal_words':{2:0xffffffff}},
    'equal-state':{'formal_words':{4:2}},
    'property-set-0':{'formal_words':{6:0,7:1}},'property-clear-0':{'formal_words':{6:0,7:0}},
    'property-set-31':{'formal_words':{6:31,7:1}},'property-set-32':{'formal_words':{6:32,7:1}},
    'property-clear-32':{'formal_words':{6:32,7:0}},'property-negative':{'formal_words':{6:0xfffffffe,7:1}},
    'mapping-none':{'mapping_byte':255},
    'authored-region1-state1':{'formal_words':{3:1,4:1,5:1},
        'assert_authored_effect':{'model':3850651255,'variant':0,'region':1,'state':1,'effect':3853600420,'parameter':100670144}},
    'duplicate-permutations-2':{'duplicate_permutations':2},
    'duplicate-permutations-5':{'duplicate_permutations':5},
    'duplicate-permutations-32':{'duplicate_permutations':32},
}


def before_patches(capture,d,variant):
    if variant=='actual':return []
    def read(address,width):
        for row in d['regions']:
            if row['address']<=address and address+width<=row['address']+row['size']:
                start=address-row['address']
                return int.from_bytes((capture/row['before_file']).read_bytes()[start:start+width],'little')
        raise ValueError(f'Unowned declared input {address:x}')
    patches=[]
    def put(address,value,width):patches.append(dict(address=address,value=value,width=width))
    sp=d['before']['gp'][4];obj=read(sp+4,4)
    header=read(read(0x4ecbd0,4)+68,4)+(obj&65535)*12
    body=read(header+8,4);definition=read(body,4)
    tag=read(read(0x4f0414,4)+(definition&65535)*16+8,4)
    if variant=='current-state-zero':
        extent=read(body+280,2);extent=extent-65536 if extent&32768 else extent
        offset=read(body+282,2);offset=offset-65536 if offset&32768 else offset
        count=int(extent/10);region=read(sp+8,4);assert 0<=region<count
        state=(body+offset+2*count+region*8)&0xffffffff
        put(state+1,0,1);put(state+4,0xffffffff,4)
    elif variant=='model-none':put(tag+56,0xffffffff,4)
    elif variant.startswith('count-'):put(body+280,int(variant[6:])*10,2)
    else:
        put(header+3,6,1)
        put(body+26,0xffff if variant=='type6-detached' else 0,2)
        if variant!='type6-detached':put(body+300,(read(body+300,1)|2) if variant=='type6-flag2' else (read(body+300,1)&~2),1)
    return patches


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    inputs=parser.add_mutually_exclusive_group(required=True)
    inputs.add_argument('--capture',type=Path)
    inputs.add_argument('--archive',type=Path,help='Retained actual-native-before.tar.zst with its verified retention.json')
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',default='c++')
    parser.add_argument('--variant',default='actual',choices=VARIANTS+['current-state-zero'])
    parser.add_argument('--matrix',action='store_true',help='Also execute explicit component BEFORE variants')
    parser.add_argument('--prefix-type-branches',action='store_true',help='Stop declared Warthog type mutations before model-state application; these are not valid complete scenery objects')
    parser.add_argument('--mutation',choices=['copy-stride','state-base','snapshot-pointer','requested-formal'],
        help='Explicit negative control in the private source copy; never edits production')
    parser.add_argument('--trace-model-owner',action='store_true',help='Observe genuine 1704C0 instructions/callees for the next native conversion')
    parser.add_argument('--native-model-owner',action='store_true',help='Compare the native 1704C0 draft with original instructions and genuine remaining callees')
    parser.add_argument('--model-case',choices=MODEL_CASES,default='actual')
    parser.add_argument('--model-matrix',action='store_true',help='Explicit live-frame/formal/data component inputs for the model-state kernel')
    parser.add_argument('--live-x87',type=int,choices=(0,2),default=0,help='Explicit caller FP input: retain two exact live80-bit values')
    parser.add_argument('--model-mutation',choices=('decreasing-force','property-wrap','candidate-stride','effect-packet','effect-handle-store','late-count-read'),
        help='Private negative control for native model state; never modifies production')
    args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    work=scratch/('region-transition-'+args.output.name)
    work.mkdir()
    args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,original_xbe_sha256=NEW_PIN,actual_native_entry_before=True,
        captured_native_after_used=False,supplied_original_outcomes=False,
        instruction_patches=[],retained_large_artifacts=[],scratch_work=str(work))
    try:
        if args.archive:
            archive=args.archive.resolve(strict=True)
            retention=json.loads((archive.parent/'retention.json').read_text())
            assert archive.stat().st_size==retention['archive_bytes'] and sha(archive)==retention['archive_sha256']
            assert retention['member_roundtrip_verified'] and not retention['captured_native_after_retained']
            capture=work/'capture';capture.mkdir();total=members=0
            process=subprocess.Popen(['zstd','-dc',str(archive)],stdout=subprocess.PIPE)
            try:
                with tarfile.open(fileobj=process.stdout,mode='r|') as stream:
                    for member in stream:
                        assert member.isfile() and (member.name=='capture-entry.json' or re.fullmatch(r'before-[0-9]+\.bin',member.name))
                        total+=member.size;members+=1
                        assert members<=20000 and total<=256*1024*1024 and member.size<=128*1024*1024
                        target=capture/member.name;assert not target.exists()
                        with target.open('wb') as output:shutil.copyfileobj(stream.extractfile(member),output)
                assert process.wait()==0
            finally:
                process.stdout.close()
                if process.poll() is None:process.kill();process.wait()
            assert members==retention['members']
            assert sha(capture/'capture-entry.json')==retention['capture_manifest_sha256']
            report['required_before_archive']=dict(path=str(archive),sha256=sha(archive),members=members,expanded_bytes=total)
        else:capture=args.capture.resolve(strict=True)
        assert sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT)
        entry_path=capture/'capture-entry.json'
        d=json.loads(entry_path.read_text())
        assert d['entry']==0xbc720 and d['reference']=='English TU1.5 only'
        assert set(d)=={'entry','return','reference','boundary','before','regions'}
        initial=dict(d['before']);assert initial['x87_values']==[]
        if args.live_x87:
            initial['x87_status']=(initial['x87_status']&~0x3800)|(6<<11)
            initial['x87_values']=['00000000000000a0ff3f','00000000000000c000c0']
            report['declared_live_caller_x87']=initial['x87_values']
        env=bytearray(108)
        tag=0xffff;top=(initial['x87_status']>>11)&7
        for n,value in enumerate(initial['x87_values']):
            tag&=~(3<<(((top+n)&7)*2));env[28+n*10:38+n*10]=bytes.fromhex(value)
        for offset,value in ((0,initial['x87_control_word']),(4,initial['x87_status']),(8,tag)):
            struct.pack_into('<H',env,offset,value)
        data=struct.pack('<10I',*initial['gp'],initial['eflags'],initial['mxcsr'])+env
        data+=b''.join(struct.pack('<4I',*row) for row in initial['xmm'])
        assert len(data)==276
        (work/'input.bin').write_bytes(data)
        regions=sorted(d['regions'],key=lambda row:row['address'])
        _,gaps,auth=reference_text_gaps(image,capture,regions,work)
        report['code_authentication']=auth
        extra_banks=[]
        if args.model_matrix or MODEL_CASES[args.model_case].get('duplicate_permutations'):
            arena=0x2100000
            while any(row['address']<arena+65536 and arena<row['address']+row['size'] for row in regions):arena+=65536
            assert arena<0x8000000
            path=work/'declared-model-arena.bin';path.write_bytes(bytes(65536))
            row=dict(address=arena,size=65536,before_file=path.name,reference_file=path)
            regions.append(row);extra_banks.append(dict(address=arena,size=65536))
            report['declared_extra_input_banks']=extra_banks
        assembly=[];sections=[];bank_hashes=[]
        for index,row in enumerate(sorted([*regions,*gaps],key=lambda row:row['address'])):
            path=row.get('reference_file',capture/row.get('before_file',''))
            assert path.stat().st_size==row['size']
            assert not row['address']<=0x70000000<row['address']+row['size']
            assembly.append(f'.section .bank{index},"awx"\n.incbin "{path}"\n')
            sections.append(f'.bank{index} 0x{row["address"]:x} : {{ *(.bank{index}) }}')
            if 'before_file' in row:
                bank_hashes.append(dict(address=row['address'],size=row['size'],sha256=sha(path)))
        asm=ASM.replace('EMIT_BANKS','').replace('ENTRY',str(d['entry']))
        asm=asm.replace('INPUT_FILE',str(work/'input.bin')).replace('FS_BASE',str(initial['fs_base']))
        (work/'bridge.s').write_text(asm+'\n'+'\n'.join(assembly))
        (work/'link.ld').write_text('ENTRY(_start)\nSECTIONS {\n'+'\n'.join(sections)+'\n.bridge 0x70000000 : { *(.bridge) }\n}\n')
        project=work/'source'
        closure={str(path.relative_to(ROOT))for folder in ('include','src')for path in (ROOT/folder).rglob('*.hpp')if path.is_file()}
        closure.add('tools/observe_native_region_transition_tu15.cpp')
        for name in sorted(closure):
            path=project/name;path.parent.mkdir(parents=True,exist_ok=True)
            path.write_bytes((ROOT/name).read_bytes())
        if args.mutation:
            path=project/'src/gameplay/objects/source_region_transition_native_tu15_internal.hpp'
            old,new={
                'copy-stride':('0xfffffffcu:4u','0xfffffff8u:8u'),
                'state-base':('s.gp[5]+std::uint32_t(count)*2u','s.gp[5]+std::uint32_t(count)'),
                'snapshot-pointer':('s.gp[0]=s.gp[4]+32u;call(0xbc80au','s.gp[0]=s.gp[4]+28u;call(0xbc80au'),
                'requested-formal':('at(0xbc7d7u);s.gp[1]=r(s.memory,s.gp[4]+56u,4)','at(0xbc7d7u);s.gp[1]=r(s.memory,s.gp[4]+52u,4)')
            }[args.mutation]
            text=path.read_text();assert text.count(old)==1
            report['negative_control_mutation']=dict(name=args.mutation,path=str(path.relative_to(project)),original_sha256=sha(path))
            path.write_text(text.replace(old,new))
        if args.model_mutation:
            name='src/gameplay/objects/source_model_region_state_native_tu15_internal.hpp'
            if args.model_mutation=='decreasing-force':name='include/halo2/native_tu15_model_region_state.hpp'
            path=project/name
            old,new={
                'decreasing-force':('state.desired<state.current && !storage.allow_lower_state()','state.desired<state.current && storage.allow_lower_state()'),
                'property-wrap':('at(0x170556u);s.gp[3]=s.flags.shift(s.gp[3],s.gp[1]&255u,4)','at(0x170556u);s.gp[3]=s.flags.shift(s.gp[3],s.gp[1]&7u,4)'),
                'candidate-stride':('s.gp[0]=s.flags.product(s.gp[0],0x1cu)','s.gp[0]=s.flags.product(s.gp[0],0x18u)'),
                'effect-packet':('s.gp[0]=0x30005a6u','s.gp[0]=0x30005a7u'),
                'effect-handle-store':('at(0x17069du),s.gp[5]+3u,4,s.gp[0]','at(0x17069du),s.gp[5]+4u,4,s.gp[0]'),
                'late-count-read':('at(0x1706bdu);s.gp[0]=r(s.memory,s.gp[1]+0x70u,4)','at(0x1706bdu);s.gp[0]=9u'),
            }[args.model_mutation]
            text=path.read_text();assert text.count(old)==1
            report['negative_model_mutation']=dict(name=args.model_mutation,path=name,original_sha256=sha(path))
            path.write_text(text.replace(old,new))
        commands=[['as','--32',str(work/'bridge.s'),'-o',str(work/'bridge.o')],
            ['ld','-melf_i386','-T',str(work/'link.ld'),str(work/'bridge.o'),'-o',str(work/'original-hardware')],
            [args.compiler,'-std=c++20','-O2','-msse3','-MMD','-MF',str(work/'observer.d'),'-I'+str(project/'include'),'-I'+str(project/'src'),
             str(project/'tools/observe_native_region_transition_tu15.cpp'),'-o',str(work/'observer')]]
        report['commands']=commands
        for index,command in enumerate(commands):
            result=subprocess.run(command,capture_output=True,text=True,timeout=180)
            (args.output/f'build-{index}.log').write_text((result.stdout+result.stderr)[-65536:])
            assert result.returncode==0,f'Build command {index} failed'
        dependencies=(work/'observer.d').read_text().replace('\\\n',' ').split(':',1)[1].split()
        report['compiled_source_hashes']={str(Path(name).relative_to(project)):sha(name) for name in dependencies if Path(name).is_relative_to(project)}
        code=_Code(image,'.text');body=complete_body(code,0xbc720)
        gp={'eax':0,'ecx':1,'edx':2,'ebx':3,'esp':4,'ebp':5,'esi':6,'edi':7}
        instructions={}
        for pc in sorted(body.code):
            item=code.at(pc);operands=[]
            if item.mnemonic!='lea' and not item.mnemonic.startswith('rep'):
                for op in item.operands:
                    if op.type==3:
                        operands.append(dict(base=gp.get(item.reg_name(op.mem.base),-1),
                            index=gp.get(item.reg_name(op.mem.index),-1),scale=op.mem.scale,
                            displacement=op.mem.disp,width=op.size,access=op.access))
            row=dict(bytes=bytes(item.bytes).hex(),mnemonic=item.mnemonic,size=item.size,memory=operands)
            if item.mnemonic=='call':
                assert item.operands[0].type==2
                row['target']=item.operands[0].imm
            instructions[str(pc)]=row
        metadata=dict(entry=d['entry'],return_address=d['return'],before=initial,instructions=instructions)
        metadata['additional_comparison_regions']=extra_banks
        if args.trace_model_owner or args.native_model_owner:
            model={}
            for pc in sorted(complete_body(code,0x1704c0).code):
                item=code.at(pc);operands=[]
                if item.mnemonic!='lea':
                    for op in item.operands:
                        if op.type==3:
                            operands.append(dict(base=gp.get(item.reg_name(op.mem.base),-1),
                                index=gp.get(item.reg_name(op.mem.index),-1),scale=op.mem.scale,
                                displacement=op.mem.disp,width=op.size,access=op.access))
                row=dict(bytes=bytes(item.bytes).hex(),mnemonic=item.mnemonic,size=item.size,memory=operands)
                if item.mnemonic=='call':
                    assert item.operands[0].type==2
                    target=item.operands[0].imm;callee=complete_body(code,target)
                    pops={code.at(site).operands[0].imm if code.at(site).operands else 0 for site in callee.code if code.at(site).mnemonic.startswith('ret')}
                    assert len(pops)==1
                    row.update(target=target,return_bytes=next(iter(pops)))
                model[str(pc)]=row
            metadata['model_instructions']=model
        metadata['native_model_owner']=args.native_model_owner
        report['capture_entry_sha256']=sha(entry_path)
        report['before_bank_manifest_sha256']=hashlib.sha256(json.dumps(bank_hashes,sort_keys=True).encode()).hexdigest()
        report['before_bank_bytes']=sum(row['size']for row in regions)
        report['source_hashes']={name:sha(project/name)for name in (
            'include/halo2/native_tu15_region_transition.hpp',
            'src/gameplay/objects/source_region_transition_native_tu15_internal.hpp',
            'tools/observe_native_region_transition_tu15.cpp')}
        if args.native_model_owner:
            for name in ('include/halo2/native_tu15_model_region_state.hpp','src/gameplay/objects/source_model_region_state_native_tu15_internal.hpp'):
                report['source_hashes'][name]=sha(project/name)
        report['tool_sha256']=sha(__file__)
        report['variants']={}
        for variant in VARIANTS if args.matrix else [args.variant]:
            for model_case in MODEL_CASES if args.model_matrix else [args.model_case]:
                name=variant if model_case=='actual'else variant+'--'+model_case
                metadata['before_patches']=before_patches(capture,d,variant)
                model_inputs=dict(MODEL_CASES[model_case])
                if model_inputs.get('duplicate_permutations'):model_inputs['declared_arena']=extra_banks[0]['address']
                metadata['model_component_inputs']=model_inputs
                metadata['stop_before']=0xbc7f9 if args.prefix_type_branches and variant.startswith('type6-')else 0
                (work/'metadata.json').write_text(json.dumps(metadata))
                destination=args.output/f'physical-{name}.json'
                command=[str(work/'observer'),str(work/'original-hardware'),str(entry_path),str(work/'metadata.json'),str(destination)]
                report['commands'].append(command)
                result=subprocess.run(command,capture_output=True,text=True,timeout=240)
                (args.output/f'execution-{name}.log').write_text((result.stdout+result.stderr)[-65536:])
                physical=json.loads(destination.read_text())
                report['variants'][name]=dict(passed=result.returncode==0 and physical['passed'],returncode=result.returncode,
                    declared_component_inputs=variant!='actual' or model_case!='actual' or bool(args.live_x87),physical_report=str(destination),
                    complete_parent_return=physical.get('complete_parent_return',False),model_component_inputs=model_inputs,
                    declared_before_patches=metadata['before_patches'],error=physical.get('error'))
                print(json.dumps(dict(variant=name,**report['variants'][name])),flush=True)
        report['passed']=all(row['passed']for row in report['variants'].values())
    except Exception as error:
        report['error']=str(error)
    finally:
        report['removed_scratch_bytes']=sum(path.stat().st_size for path in work.rglob('*')if path.is_file())
        shutil.rmtree(work)
        report['scratch_work_removed']=not work.exists()
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report.get(key)for key in ('passed','error','before_bank_bytes','returncode')}))
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
