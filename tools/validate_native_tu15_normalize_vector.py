#!/usr/bin/env python3
"""Full state transport versus unchanged physical TU1.5/MP11 normalizers.

Requires run_test.py ownership. Retains a bounded receipt; executable copies,
native probes, raw reference banks and pipe data live under HALO2_TEST_SCRATCH.
"""
import argparse
import copy
import gzip
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
STACK,STOP,FIRST=0x62000000,0x63000000,0x1000100
PINS={'tu15':'21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
      'mp11':'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'}


def rows(image):
    fixture=json.loads(gzip.decompress((ROOT/'tests/fixtures/native_campaign_ai_vector_math_tu15.json.gz').read_bytes()))
    result=[copy.deepcopy(r) for r in fixture['cases'] if r['kind']=='normalize' and r['completed']]
    # Directed norm-sum rounding witnesses from the accepted light-tail proof.
    import validate_native_tu15_light_up_vector as light
    for item in light.cases():
        if item.get('entry')==0x30bd0:
            result.append(dict(name=item['name'],gp=[FIRST,0xaabbccdd,0x22222222,0x33333333,0,0x55555555,0x66666666,0x77777777],
                before_fp=[item['cw'],item['status'],item['mxcsr']],live_x87=[],
                banks=[dict(address=FIRST,before=struct.pack('<3I',*item['up']).hex()),
                       dict(address=0x46a0d0,before=struct.pack('<I',0x3f800000).hex()),
                       dict(address=0x46a0ec,before=struct.pack('<I',0x38d1b717).hex())]))
    seed=copy.deepcopy(result[0])
    for eps in (0,0x80000000,0xbf800000,0x7fc12345,0x7f812345,0xffc54321,0x7f800000,0x38d1b717):
        for v in ((0,0,0),(0x38d1b717,0,0),(0x3f800000,0,0),(1,0,0)):
            r=copy.deepcopy(seed);r['name']=f'epsilon-bank-{eps:x}-{v}'
            r['gp'][0]=FIRST;r['banks']=[dict(address=FIRST,before=struct.pack('<3I',*v).hex()),
                dict(address=0x46a0d0,before='0000803f'),dict(address=0x46a0ec,before=struct.pack('<I',eps).hex())]
            result.append(r)
    for n,r in enumerate(result):
        r.update(id=n,entry=0x30bd0,matrix=0,source=0,image=image,eflags=0xed7,
            xmm=[[0x6b000000+n*37+axis*4+lane for lane in range(4)]for axis in range(8)])
        r['gp'][4]=STACK+0x800
        if image=='mp11':
            for bank in r['banks']:
                bank['address']={0x46a0d0:0x465030,0x46a0ec:0x46504c}.get(bank['address'],bank['address'])
        # Wrong-profile decoys make a mistaken image selection observable.
        wrong_one,wrong_eps=(0x465030,0x46504c) if image=='tu15' else (0x46a0d0,0x46a0ec)
        r['banks'] += [dict(address=wrong_one,before='00000040'),dict(address=wrong_eps,before='0000803f')]
        stack=bytearray(4096);struct.pack_into('<I',stack,0x800,STOP)
        r['banks'].append(dict(address=STACK,before=stack.hex()))
    return result


def physical(image,inputs,scratch):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(0x30bd0,);common.geometry.STOP=STOP;meta=common.metadata(image)
    scratch.mkdir();section=image.section('.text');(scratch/'text.bin').write_bytes(image.section_bytes(section))
    (scratch/'metadata.json').write_text(json.dumps(meta));(scratch/'inputs.json').write_text(json.dumps(inputs,separators=(',',':')))
    (scratch/'reference.s').write_text(f'''.code32
.section .original,"ax"
.incbin "{scratch/'text.bin'}"
.section .constants15,"aw"
.space 4096
.section .constants11,"aw"
.space 4096
.section .data,"aw"
.space 8192
.section .stack,"aw"
.space 4096
.section .stop,"ax"
int3
.section .bridge,"ax"
.globl _start
_start:
fninit
int3
jmp _start
''')
    (scratch/'link.ld').write_text(f'ENTRY(_start)\nSECTIONS{{.original 0x{section.va:x}:{{*(.original)}} .constants15 0x46a000:{{*(.constants15)}} .constants11 0x465000:{{*(.constants11)}} .data 0x1000000:{{*(.data)}} .stack 0x{STACK:x}:{{*(.stack)}} .stop 0x{STOP:x}:{{*(.stop)}} .bridge 0x70000000:{{*(.bridge)}}}}\n')
    for command in [['as','--32',str(scratch/'reference.s'),'-o',str(scratch/'reference.o')],
        ['ld','-melf_i386','-T',str(scratch/'link.ld'),str(scratch/'reference.o'),'-o',str(scratch/'reference')],
        ['c++','-std=c++20','-O2',str(ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'),'-o',str(scratch/'observer')]]:
        p=subprocess.run(command,capture_output=True,text=True)
        if p.returncode:raise RuntimeError(p.stderr[-3000:])
    p=subprocess.run([str(scratch/'observer'),str(scratch/'reference'),str(scratch/'metadata.json'),str(scratch/'inputs.json'),str(scratch/'observed.json')],capture_output=True,text=True,timeout=150)
    if p.returncode:raise RuntimeError((p.stdout+p.stderr)[-3000:])
    proof=json.loads((scratch/'observed.json').read_text())
    assert not proof['instruction_patches'] and not proof['supplied_original_outcomes']
    return proof,meta['owners']


def compare(inputs,proof,native):
    process=subprocess.Popen([str(native)],text=True,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    digest=hashlib.sha256();stores=0;classes=set()
    def send(r):
        process.stdin.write(json.dumps(r,separators=(',',':'))+'\n');process.stdin.flush();line=process.stdout.readline()
        if not line:raise ValueError('Native stopped: '+process.stderr.read()[-3000:])
        return json.loads(line)
    try:
        for r,gold in zip(inputs,proof['cases'],strict=True):
            assert gold['completed'] and gold['live_x87_unchanged']
            a=send(r);name=r['image']+'/'+r['name']
            expected=dict(gp=gold['boundary_gp'],xmm=gold['boundary_xmm'],eflags=gold['boundary_eflags'],
                fp=gold['after_fp'],live_x87=r['live_x87'],rejected=False,host_x87_top=0,fs_base=0x7ffdf000)
            # The final stack residue is retained; the two transient stack
            # observer events and stack/RET reads remain literal-only.
            reads=[[e['address'],e['bytes']]for e in gold['reads']if not STACK<=e['address']<STACK+4096]
            writes=[[e['address'],e['written']]for e in gold['writes']if not STACK<=e['address']<STACK+4096]
            expected.update(reads=reads,writes=writes+[[r['gp'][4]-4,struct.pack('<I',gold['boundary_gp'][1]).hex()]])
            for key,value in expected.items():
                if a[key]!=value:raise ValueError(f'{name}: {key} native={a[key]} original={value}')
            for b in gold['banks']:
                if b not in a['banks']:raise ValueError(name+': retained bank differs')
            flags=a['eflags']&0x45;classes.add(flags);stores+=len(writes)
            digest.update(json.dumps(expected,sort_keys=True,separators=(',',':')).encode())
        # Explicit bounded rejections; the original entry cannot own >=4 live
        # x87 values or active MMX. Early input read exceptions leave no real
        # x87 temporary on the caller's host stack.
        for depth in (4,5,8):
            r=copy.deepcopy(inputs[0]);r['live_x87']=['0000000000000080ff3f']*depth
            a=send(r);assert a['rejected'] and not a['reads'] and not a['writes'] and a['host_x87_top']==0
        r=copy.deepcopy(inputs[0]);r['active_mmx']=True;a=send(r);assert a['rejected'] and not a['reads'] and not a['writes']
        for ordinal in range(8):
            r=copy.deepcopy(inputs[0]);r['reject_read']=ordinal;a=send(r)
            assert a['rejected'] and a['host_x87_top']==0
            nonstack_reads=[e for e in proof['cases'][0]['reads']if not STACK<=e['address']<STACK+4096]
            stop=nonstack_reads[ordinal]['instruction']
            prefix=[[e['address'],e['written']]for e in proof['cases'][0]['writes']
                if e['instruction']<stop and not STACK<=e['address']<STACK+4096]
            assert a['writes']==prefix,'Rejected resident nonstack write prefix changed'
        r=copy.deepcopy(inputs[0]);r['observe']=True;a=send(r)
        events=[[e['instruction'],e['address'],e['before'],e['written']]for e in proof['cases'][0]['writes']
            if not STACK<=e['address']<STACK+4096]
        assert a['observer_events']==events,'Ungated vector observer events changed'
        gate_count=qualify_gate(inputs[0],send)
        process.stdin.close();assert process.wait(timeout=10)==0,process.stderr.read()[-3000:]
    finally:
        if process.poll()is None:process.terminate();process.wait(timeout=10)
    assert classes=={0,1,0x40,0x45},classes
    return dict(cases=len(inputs),ordered_nonstack_stores=stores,comparison_sha256=digest.hexdigest(),comparison_flags=sorted(classes),
        guard_and_read_rejections=12,ungated_observer_cases=1,gate_cases=gate_count,complete_stack_banks_compared=len(inputs))


def qualify_gate(seed,send):
    row=copy.deepcopy(seed);row.update(gate=True,kernel=True,direct=True,live_x87=[])
    data=bytearray(8192);struct.pack_into('<3f',data,FIRST-0x1000000,3.,4.,5.)
    stack=bytearray(4096);struct.pack_into('<I',stack,0x800,STOP)
    row['banks']=[dict(address=0x1000000,before=data.hex()),dict(address=STACK,before=stack.hex())]
    for base,one,epsilon in [(0x46a000,0xd0,0xec),(0x465000,0x30,0x4c)]:
        bank=bytearray(4096);struct.pack_into('<I',bank,one,0x3f800000);struct.pack_into('<I',bank,epsilon,0x38d1b717)
        row['banks'].append(dict(address=base,before=bank.hex()))
    row['gp'][0]=FIRST;row['gp'][4]=STACK+0x800
    raw_row=copy.deepcopy(row);raw_row['gate']=False;raw_reference=send(raw_row)
    cases=[('ordinary',{},None),('cursor',dict(instruction='cursor'),None),('remap',dict(instruction='remap'),None),
        ('guarded',dict(guarded=True),None),('policy-refusal',dict(guarded=True,deny_policy=True),8),
        ('known-renderer',dict(instruction='cursor',position=0x324a1),None),
        ('known-renderer-pages',dict(instruction='cursor',position=0x324a1,missing_pages=[FIRST]),6),
        ('known-renderer-reference',dict(instruction='cursor',position=0x324a1,kernel=False),0),
        ('no-kernel',dict(kernel=False),0),('depth',dict(live_x87=['0000000000000080ff3f']*4),2),
        ('arbitrary',dict(instruction='arbitrary'),3),('uncovered-remap',dict(instruction='uncovered-remap'),3),
        ('null-cursor',dict(instruction='cursor',null_position=True),3),
        ('trace',dict(instruction='cursor',trace=True),4),('observer',dict(observe=True),5),
        ('no-direct',dict(direct=False),6),('vector-readonly',dict(read_only_pages=[FIRST]),6),
        ('stack-readonly',dict(read_only_pages=[STACK]),6),('missing-vector',dict(missing_pages=[FIRST]),6),
        ('missing-constant',dict(missing_pages=[0x465000 if row['image']=='mp11' else 0x46a000]),7)]
    if row['image']=='tu15':cases.append(('base-required',dict(required=True),None))
    if row['image']=='mp11':cases.append(('tu15-literal-in-mp11',dict(profile='tu15'),1))
    for name,changes,refusal in cases:
        r=copy.deepcopy(row);r.update(changes);a=send(r)
        assert not a['rejected'],name+': gate threw instead of declining'
        assert a['gate_taken']==(refusal is None),name+': gate admission changed'
        assert a['instruction_calls']==0,name+': native gate delivered an internal instruction'
        assert a['owner_calls']==0,name+': base required performed an external ABI round trip'
        if refusal is None:
            assert a['admitted']==1 and not any(a['refused']),name+': admitted counter differs'
            for key in ('gp','xmm','eflags','fp','live_x87','banks','writes'):
                assert a[key]==raw_reference[key],name+': admitted '+key+' differs from qualified transport'
            if r.get('instruction'):assert a['position']==0x30c37,name+': normalized exit PC differs'
            if name=='known-renderer':assert a['renderer_admitted']==1 and a['last_admitted_site']==0x324a1,'Renderer census missed actual source site'
        else:
            expected=[0]*9;expected[refusal]=1
            assert a['admitted']==0 and a['refused']==expected,name+': refusal counter differs'
            assert not a['writes'] and not a['reads'] and a['gp']==r['gp'],name+': declined gate changed source state'
            assert a['fp']==r['before_fp'],name+': declined gate changed source floating point'
        if name.startswith('known-renderer'):
            site=next(x for x in a['renderer_sites']if x['pc']==0x324a1)
            assert site['admitted']==int(refusal is None),'Per-site native admission count differs'
            assert site['first']==site['last']==int(refusal is None),'Per-site admission ordinals differ'
            expected_refusals=[0]*9
            if refusal is not None:
                expected_refusals[refusal]=1
                assert site['last_vector']==r['gp'][0]and site['last_stack']==r['gp'][4]and site['last_refusal']==refusal,'Per-site refusal lost its operands'
            assert site['refused']==expected_refusals,'Per-site refusal reason differs'
    for name,v,e in [('vector-straddle',0x1000ff8,STACK+0x800),('stack-straddle',FIRST,STACK),
        ('stack-vector-alias',FIRST,FIRST+4),('stack-epsilon-alias',FIRST,0x465050 if row['image']=='mp11' else 0x46a0f0)]:
        r=copy.deepcopy(row);r['gp'][0]=v;r['gp'][4]=e;a=send(r)
        assert not a['gate_taken'] and a['refused'][6]==1 and not a['writes'],name+': unsafe stack/vector admitted'
    for zero_epsilon,expected in [(False,0x30c3c),(True,0x30c37)]:
        r=copy.deepcopy(row);r['instruction']='cursor'
        b=bytearray.fromhex(r['banks'][0]['before']);struct.pack_into('<3I',b,FIRST-0x1000000,0,0,0);r['banks'][0]['before']=b.hex()
        if zero_epsilon:
            address=0x465000 if row['image']=='mp11' else 0x46a000;offset=0x4c if row['image']=='mp11' else 0xec
            bank=next(b for b in r['banks']if b['address']==address);raw=bytearray.fromhex(bank['before']);struct.pack_into('<I',raw,offset,0);bank['before']=raw.hex()
        a=send(r);assert a['gate_taken'] and a['position']==expected,'Skip/zero-normalize exit PC differs'
    r=copy.deepcopy(row);r['gp'][0]=0x10007fc;r['alias_stack_page']=True;a=send(r)
    assert not a['gate_taken'] and a['refused'][6]==1 and not a['writes'],'Aliased physical stack page admitted'
    # Production mounts these operands as exact four-byte immutable owners,
    # with no committed page or readable padding around either word.
    r=copy.deepcopy(row);r['constant_views']=True
    r['banks']=r['banks'][:2]+[dict(address=base+offset,before=struct.pack('<I',value).hex())
        for base,one,epsilon in [(0x46a000,0xd0,0xec),(0x465000,0x30,0x4c)]
        for offset,value in [(one,0x3f800000),(epsilon,0x38d1b717)]]
    transport=copy.deepcopy(r);transport['gate']=False;expected=send(transport)
    a=send(r);assert a['gate_taken'] and a['admitted']==1 and not any(a['refused']),'Bounded constant owners refused'
    for key in ('gp','xmm','eflags','fp','live_x87','banks','writes'):
        assert a[key]==expected[key],'Bounded constant '+key+' differs from physical-qualified transport'
    for name,changes,reason in [('no-capability',dict(constant_views=False),7),
        ('short-capability',dict(short_constant_view=True),7),('backing-alias',dict(constant_view_alias=True),6)]:
        case=copy.deepcopy(r);case.update(changes);a=send(case)
        assert not a['gate_taken'] and a['refused'][reason]==1 and not a['writes'] and not a['reads'],name+': unsafe bounded constant admitted'
        assert a['gp']==case['gp'] and a['fp']==case['before_fp'],name+': bounded refusal changed ABI'
    # Ordinary sub-page writable storage is admitted only by a guarded owner.
    small=copy.deepcopy(row);small.update(guarded=True,writable_views=True)
    source=bytes.fromhex(small['banks'][0]['before']);offset=FIRST-small['banks'][0]['address']
    small['banks'][0]=dict(address=FIRST,before=source[offset:offset+12].hex())
    for short_stack in (False,True):
        case=copy.deepcopy(small)
        if short_stack:
            case['banks'][1]=dict(address=case['gp'][4]-4,before=(bytes(4)+struct.pack('<I',STOP)).hex())
        transport=copy.deepcopy(case);transport['gate']=False;expected=send(transport);a=send(case)
        assert a['gate_taken']and a['admitted']==1 and not any(a['refused']),'Bounded writable owner refused'
        for key in ('gp','xmm','eflags','fp','live_x87','banks','writes'):
            assert a[key]==expected[key],'Bounded write '+key+' differs from physical-qualified transport'
    for name,changes in [('opaque',dict(writable_views=False)),('short',dict(short_writable_view=True)),
        ('alias',dict(writable_view_alias=True)),('raw',dict(guarded=False))]:
        case=copy.deepcopy(small);case.update(changes);a=send(case)
        assert not a['gate_taken']and a['refused'][6]==1 and not a['reads']and not a['writes'],name+': unsafe writable extent admitted'
        assert a['gp']==case['gp']and a['fp']==case['before_fp'],name+': write refusal changed ABI'
    legacy_required=0
    if row['image']=='mp11':
        r=copy.deepcopy(row);r['required']=True;a=send(r)
        assert a['owner_calls']==1 and a['admitted']==0 and not any(a['refused']) and a['rejected'], 'Legacy required owner was intercepted'
        assert not a['writes'] and not a['reads'] and a['gp']==r['gp'],'Legacy required refusal changed source state'
        legacy_required=1
    return len(cases)+17+legacy_required


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    p.add_argument('--tu15',type=Path,required=True);p.add_argument('--mp11',type=Path,required=True);a=p.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);results={}
    from oracle_smg_firing_1_1 import Xbe
    for key,path in [('tu15',a.tu15),('mp11',a.mp11)]:
        if not path.is_file():print('SKIP missing owned original '+key);return 77
        if hashlib.sha256(path.read_bytes()).hexdigest()!=PINS[key]:raise ValueError('Wrong owned original '+key)
        inputs=rows(key);proof,owners=physical(Xbe(path),inputs,scratch/key)
        results[key]=dict(**compare(inputs,proof,a.native),owners=owners,original_instructions=proof['original_instructions'],image_sha256=PINS[key])
    names=['include/halo2/native_tu15_normalize_vector.hpp','src/physics/runtime/native_tu15_normalize_vector_bridge.hpp',
        'src/campaign/ai/native_campaign_ai_vector_math_tu15.cpp','tests/native_tu15_normalize_vector_tests.cpp',
        'include/halo2/detail/native_tu15_arithmetic_environment.hpp',
        'src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp',
        'include/halo2/physics_contact_runtime.hpp','src/physics/runtime/physics_instruction_remap_tu15.hpp',
        'tools/observe_native_campaign_ai_vector_math_tu15.cpp','tools/validate_native_tu15_normalize_vector.py']
    a.report.write_text(json.dumps(dict(passed=True,results=results,instruction_patches=[],supplied_outcomes=[],
        source_sha256={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest()for n in names},
        scope='Full successful GP/XMM/EFLAGS/logical x87/FP, complete banks including final stack residue, ordered nonstack memory. Transient stack events and per-instruction PCs are literal-only. Depth/MMX guards are declared unsupported-owner refusals. Production renderer exercise is qualified separately.',
        whole_renderer_visual_parity_verified=False,large_artifacts_retained=[]),indent=2)+'\n')
    print('PASS full physical normalizer state:',sum(r['cases']for r in results.values()),'cases across TU1.5 and MP11')


if __name__=='__main__':raise SystemExit(main())
