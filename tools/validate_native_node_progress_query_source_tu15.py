#!/usr/bin/env python3
"""Compare 1CF0E0 with the genuine 1E3150 child and complete retained source."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess

from oracle_smg_firing_1_1 import Xbe
from specialize_native_operands import specialize
from validate_native_node_descriptor_lookup_tu15 import corpus as descriptor_inputs, prepare as prepare_descriptor, INPUTS
from validate_native_tu15_prt_helpers import PIN

ROOT=Path(__file__).resolve().parents[1]
UNIT='src/campaign/ai/source_ai_frame_tu15_generated.cpp'
DRIVER='tests/native_tu15_node_progress_query_source_tests.cpp'
FRAME='src/gameplay/animation/native_tu15_node_progress_query_source_frame.hpp'
BODY=re.compile(r'^static void run_1cf0e0\(WorldState &s\) \{.*?^\}',re.M|re.S)
PINS=('71035e9cf81f9d7a7d92906845f1937656c3c9f8c71072df8e64988d52f1441d',
      '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
      'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
      '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b')
DEFECTS=(
    ('wrong-sentinel','return !s.flags.zf;','return s.flags.zf;'),
    ('wrong-child-return','push(0x1cf108u,0x1cf10du);child(s);','push(0x1cf108u,0x1cf10eu);child(s);'),
    ('unsigned-divisor','std::int32_t(word)','std::int32_t(std::uint16_t(word))'),
    ('wrong-lower-bound','if(!below_or_equal()){','if(below_or_equal()){'),
    ('wrong-scratch-reload','s.xmm[0]=scalar(s.gp[4]);','s.xmm[0]=scalar(s.gp[4]+4u);'),
)


def sha(data):
    if isinstance(data,Path):data=data.read_bytes()
    if isinstance(data,str):data=data.encode()
    return hashlib.sha256(data).hexdigest()


def one(pattern,text,flags=re.M|re.S):
    items=pattern.findall(text) if isinstance(pattern,re.Pattern) else re.findall(pattern,text,flags)
    if len(items)!=1:raise ValueError('Missing or ambiguous node progress source context')
    return items[0]


def prepare(scratch,require_binding=False):
    import native_node_progress_query_replacements as binding
    actual=(ROOT/UNIT).read_text()
    if require_binding:
        binding.installed_entries(actual)
        output,receipt=specialize(actual)
        if receipt.get('native_node_progress_query_entries')!=binding.installed_entries(output):
            raise ValueError('Actual AIFrame specializer missed 1CF0E0')
    raw=binding.remove_node_progress_query(actual);current=one(BODY,raw)
    resolver=one(r'^std::uint32_t selected\([^\n]+',raw)
    sign=one(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',raw)
    dispatcher=one(r'^static void dispatch\([^\n]+',raw)
    if tuple(map(sha,(current,resolver,sign,dispatcher)))!=PINS:
        raise ValueError('Complete AIFrame node progress source changed')
    literals=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',current))
    table=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
    if any(table.get(value)!=value for value in literals):raise ValueError('Node progress nonidentity operand')
    prepared=one(BODY,specialize(raw)[0])
    raw_sites=re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)',current)
    lit_sites=re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)',prepared)
    native=set(re.findall(r'0x1cf(?:0[e-f]|1[0-4])[0-9a-f]u',(ROOT/FRAME).read_text()))
    if len(set(raw_sites))!=34 or raw_sites!=lit_sites or set(raw_sites)!=native:
        raise ValueError('Complete node progress callback boundaries changed')
    controls=prepare_descriptor(scratch,True)
    descriptor=(scratch/'node_descriptor_lookup_bodies.inc').read_text()
    fragments=['using namespace halo2::physics_world_runtime_detail;',resolver,sign,'namespace genuine {']
    for name in ('reference1','specialized_reference1'):
        start=descriptor.index('namespace '+name+' {\n')
        following=re.search(r'^namespace ',descriptor[start+1:],re.M)
        end=start+1+following.start()if following else len(descriptor)
        fragments.append(descriptor[start:end])
    fragments.append('}')
    for name,source,child in (('raw',current,'reference1'),('literal',prepared,'specialized_reference1')):
        if source.count('run_1e3150(s);')!=1:raise ValueError('Missing genuine 1E3150 local edge')
        fragments.append(f'static void {name}_child(WorldState&s){{genuine::{child}::run_1e3150(s);}}')
        fragments.append(source.replace('run_1cf0e0(',name+'_parent(',1).replace('run_1e3150(s);',name+'_child(s);',1))
    (scratch/'node_progress_query_reference.inc').write_text('\n'.join(fragments)+'\n')
    return dict(raw_body_sha256=PINS[0],prepared_body_sha256=sha(prepared),resolver_sha256=PINS[1],
                exact_identity_operands=len(literals),retained_child='AIFrame1E3150',
                existing_child_binding_controls=controls,no_child_result_supplied=True)


def corpus(image):
    initial=descriptor_inputs()
    rows=copy.deepcopy(initial[:96]+initial[192:196])
    for n,row in enumerate(rows):
        data={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        data[0x46a000]=bytearray(4096)
        def put(address,value,width=4):
            for base,bank in data.items():
                if base<=address<=base+len(bank)-width:
                    offset=address-base;bank[offset:offset+width]=value.to_bytes(width,'little');return
            raise ValueError('Node progress fixture escaped declared bank')
        holder=0x61000a00;descriptor=row['gp'][2]
        put(holder,0xa55a0001);put(holder+4,row['matrix'])
        put(holder+0x1c,(0x3e800000,0xbf800000,0x40800000,0,0x7fc0a55a,0x7f800000)[n%6])
        put(0x61002000+16+8,descriptor)
        put(0x51df20,0,1)
        for node in (0x61004000+row['id']%4,0x61006000+row['id']%4):
            put(node+0x14,(0,1,1,0xffff,8,0x7fff)[n%6],2)
        put(0x46a0d0,int.from_bytes(image.read(0x46a0d0,4),'little'))
        row['gp'][6]=holder
        row.update(id=n,name=f'node-progress-{n}',entry=0x1cf0e0)
        row['banks']=[dict(address=base,before=bank.hex())for base,bank in sorted(data.items())]
    return rows


def local_corpus(image):
    rows=corpus(image)
    for n,row in enumerate(rows):
        banks={bank['address']:bytearray.fromhex(bank['before']) for bank in row['banks']}
        def put(address,value,width=4):
            for base,data in banks.items():
                offset=address-base
                if 0<=offset<=len(data)-width:
                    data[offset:offset+width]=value.to_bytes(width,'little')
                    return
            raise ValueError('AIFrame local fixture escaped declared banks')
        owner,destination=0x610009c0,0x61000b80
        row['gp'][3],row['gp'][7]=owner,destination
        put(owner+0x6e,0 if n%5==0 else 2,1)
        if n%5==1:put(owner+0x40,0xffffffff)
        if n%5==2:put(owner+0x46,0xffff,2)
        values=(0x3f800000,0x3f000000,0xbf800000)
        for i in range(3):
            put(owner+0x84+4*i,values[(n+i)%3]);put(destination+0x10+4*i,values[(n+i+1)%3])
        put(row['gp'][4]+4,values[n%3])
        row['banks']=[dict(address=base,before=data.hex()) for base,data in sorted(banks.items())]
    return rows


def prepare_local(scratch):
    import native_node_progress_query_replacements as binding
    actual=(ROOT/UNIT).read_text()
    binding.installed_entries(actual)
    prepared,receipt=specialize(actual)
    if receipt.get('native_node_progress_query_entries')!=binding.installed_entries(prepared):
        raise ValueError('Actual AIFrame specializer missed local 1CF0E0 route')
    caller=re.compile(r'^static void run_1d63c0\(WorldState &s\) \{.*?^\}',re.M|re.S)
    fragments=[]
    for name,text,literal in (
        ('candidate_raw',actual,False),('candidate_literal',prepared,True),
        ('retained_raw',binding.remove_node_progress_query(actual),False),
        ('retained_literal',binding.remove_node_progress_query(prepared),True),
    ):
        parent=one(caller,text)
        if parent.count('run_1cf0e0(s);')!=1:raise ValueError('Actual AIFrame local call changed')
        child='literal_child' if literal else 'raw_child'
        fragments.extend(('namespace '+name+' {',
            'using namespace halo2::physics_world_runtime_detail;',
            'using reference::selected;using reference::sign_extend;',
            'static void run_1e3150(WorldState&s){reference::'+child+'(s);}',
            one(BODY,text),parent,'}'))
    (scratch/'node_progress_local_fixture.inc').write_text('\n'.join(fragments)+'\n')
    return dict(caller='1d63c0',node='1cf0e0',descriptor='1e3150',exact_installed_prefix=True)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--compiler',action='append')
    p.add_argument('--sanitizers',action='store_true')
    p.add_argument('--semantic-mutants',action='store_true')
    p.add_argument('--require-binding',action='store_true')
    p.add_argument('--local-parent',action='store_true')
    a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Use tools/run_test.py')
    if a.local_parent and a.semantic_mutants:p.error('Run source defects separately from the installed local caller')
    if not a.xbe.is_file():return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    paths=set(INPUTS)|{UNIT,DRIVER,FRAME,str(Path(__file__).relative_to(ROOT)),
        'tools/validate_native_node_descriptor_lookup_tu15.py','tools/specialize_native_operands.py',
        'tools/native_node_progress_query_replacements.py','src/gameplay/animation/native_tu15_node_progress_query_source_owner.hpp'}
    before={name:sha(ROOT/name)for name in sorted(paths)}
    headers={x.resolve():sha(x)for d in ('include','src','tests')for x in(ROOT/d).rglob('*')if x.suffix in('.hpp','.h','.inc')}
    r=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],production_bound=False)
    try:
        if sha(a.xbe)!=PIN:raise ValueError('Wrong original TU1.5 authority')
        image=Xbe(a.xbe)
        call=image.read(0x1cf108,5)
        if call[0]!=0xe8 or 0x1cf10d+int.from_bytes(call[1:],'little',signed=True)!=0x1e3150:
            raise ValueError('Actual 1CF108 child edge changed')
        r['authority']=prepare(scratch,a.require_binding or a.local_parent)
        if a.local_parent:r['local_authority']=prepare_local(scratch)
        fixture=scratch/'inputs.json'
        fixture.write_text(json.dumps((local_corpus if a.local_parent else corpus)(image),separators=(',',':')))
        r.update(fixture_sha256=sha(fixture),compilers=[])
        authored=(ROOT/FRAME).read_text()
        for compiler in a.compiler or ['g++']:
            directory=scratch/('source-'+Path(compiler).name)
            directory.mkdir()
            flags=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math']
            if a.sanitizers:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            if a.local_parent:flags.append('-DHALO2_NODE_PROGRESS_AI_LOCAL=1')
            suffix=[*('-I'+str(ROOT/d)for d in ('include','src','tests')),'-I'+str(scratch),str(ROOT/DRIVER)]
            binary=directory/'node-progress';dep=directory/'dependencies.d'
            command=[*flags,*suffix,'-o',str(binary),'-MD','-MF',str(dep)]
            built=subprocess.run(command,capture_output=True,text=True,timeout=1200)
            if built.returncode:raise ValueError('Node progress source compile failed: '+built.stderr[-8500:])
            for name in shlex.split(dep.read_text().replace(chr(92)+'\n',' ').split(':',1)[1]):
                file=Path(name).resolve()
                if file in headers:before[str(file.relative_to(ROOT))]=headers[file]
            env={key:value for key,value in os.environ.items() if not key.startswith(('HALO2_REFERENCE_','HALO2_VERIFY_'))}
            run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200,env=env)
            if run.returncode:raise ValueError('Node progress source comparison failed: '+(run.stdout+run.stderr)[-8000:])
            result=dict(compiler=compiler,result=json.loads(run.stdout),compiled_defects=[])
            r['compilers'].append(result)
            if a.local_parent:
                baseline=result['result'];controls={}
                if not baseline.get('native_entries'):raise ValueError('Installed local AIFrame default did not enter 1CF0E0')
                for name,key in (('own-reference','HALO2_REFERENCE_NODE_PROGRESS_QUERY'),
                                 ('unrelated-reference','HALO2_REFERENCE_DEVICE_PROPERTY_QUERY')):
                    control=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200,
                                           env={**env,key:'1'})
                    if control.returncode:raise ValueError('Local AIFrame control failed: '+control.stderr[-8000:])
                    actual=json.loads(control.stdout)
                    expected={**baseline,'native_entries':0} if name=='own-reference' else baseline
                    if actual!=expected:raise ValueError('Local AIFrame reference control crossed owners: '+name)
                    controls[name]=actual
                result['local_reference_controls']=controls
            if a.semantic_mutants:
                for label,old,new in DEFECTS:
                    if authored.count(old)!=1:raise ValueError('Node progress defect anchor changed: '+label)
                    mutant=directory/label
                    path=mutant/Path(FRAME).relative_to('src')
                    path.parent.mkdir(parents=True)
                    path.write_text(authored.replace(old,new,1))
                    binary=mutant/'defect'
                    built=subprocess.run([*flags,'-I'+str(mutant),*suffix,'-o',str(binary)],
                                         capture_output=True,text=True,timeout=1200)
                    if built.returncode:raise ValueError('Node progress defect compile failed: '+built.stderr[-6500:])
                    run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=600,env=env)
                    if run.returncode!=1 or 'Node progress retained source differs'not in run.stderr:
                        raise ValueError('Node progress control did not reject a source difference: '+label)
                    result['compiled_defects'].append(dict(label=label,diagnostic=run.stderr.strip()[:2000]))
                    print(compiler,label,'rejected',flush=True)
        if len(r['compilers'])==1:r.update(r['compilers'][0])
        r['passed']=True
    except BaseException as error:r['error']=repr(error);raise
    finally:
        r['source_before']=before;r['source_after']={n:sha(ROOT/n)for n in before}
        if r['source_before']!=r['source_after']:r.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(r,indent=2,sort_keys=True)+'\n')
    return 0 if r['passed']else 1


if __name__=='__main__':raise SystemExit(main())
