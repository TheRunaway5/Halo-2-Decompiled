#!/usr/bin/env python3
"""Compare private typed copy/sentinel transport to authenticated retained source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import subprocess
import sys

from specialize_native_operands import MAPPING, INSTRUCTION, specialize

ROOT = Path(__file__).resolve().parents[1]
FRAME = 'src/physics/runtime/native_tu15_contact_copy_leaf_source_frame.hpp'
TEST = 'tests/native_tu15_contact_copy_leaf_source_tests.cpp'
KERNEL = 'include/halo2/native_tu15_contact_copy_leaf_kernel.hpp'
MAPPED_OPERAND = '(s.image==SourcePhysicsContactImage::MapPack11?0x487ac4u:0x48cdc4u)'
WORLD_OBSERVATION_PIN = '6a3c44db3858c1e20f481c04996af7858d8b6b948fe71e73b952c244411fc8a2'
UNITS = {
    'contact': ('State','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e',
        '915e87f07fb9cbb1d74ef1e80f5bbaf0b9cb4d8ea9582eeaf4e688df0ceb8fc9',
        {'300100':'22f919d512e17bdc21fffad32c6ad93c5d1dc9e5fd9d1a6ae7858c606bc04835',
         '307fe0':'0a3e551935d3ba8b3cb6a615d63e41ccac1fdfe66fca1994eb48d88adf26fa24'}),
    'world': ('ObservedWorldState','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2',
        'c5ea0c5e4c241d9a0b448bea12cb9fc24cc8a208b0d861a57d22dd9dc0fc143c',
        {'300100':'81a58207a0a195ca2d91d4268e69b80c22c34abcbdc5b25d7a9d92b9d5862065',
         '307fe0':'d51530585e5c6dd4791485b18083a8b630b91ab8dc1aa972e775eb1977b35e54'}),
}


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def digest(text): return hashlib.sha256(text.encode()).hexdigest()
def unit(name): return f'src/physics/runtime/physics_{name}_runtime_generated.cpp'


def unique(pattern, text, label):
    matches=list((re.compile(pattern,re.S) if isinstance(pattern,str) else pattern).finditer(text))
    if len(matches)!=1: raise ValueError(f'Expected one authenticated {label}; got {len(matches)}')
    return matches[0][0]


def references(scratch):
    sections=[]; evidence={}
    for name,(state,resolver_pin,dispatch_pin,pins) in UNITS.items():
        raw=(ROOT/unit(name)).read_text()
        resolver=unique(MAPPING,raw,name+' resolver')
        dispatcher=unique(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',raw,name+' dispatcher')
        if (digest(resolver),digest(dispatcher))!=(resolver_pin,dispatch_pin):
            raise ValueError(f'{name} full resolver/dispatcher changed')
        if resolver.count('case 0x487ac4u:return 0x48cdc4u;')!=1:
            raise ValueError(f'{name} sentinel image remapping changed')
        observation=''
        if name=='world':
            observation=unique(r'// BEGIN authenticated physics world observation addresses\n.*?(?=// END authenticated physics world observation addresses)',raw,'World observation map')
            if digest(observation)!=WORLD_OBSERVATION_PIN: raise ValueError('World observation helper changed')
        prepared,receipt=specialize(raw)
        if digest(prepared)!=receipt['specialized_sha256']:
            raise ValueError('Prepared source digest does not identify returned bytes')
        bodies={}; prepared_bodies={}
        for entry,body_pin in pins.items():
            pattern=r'static void run_'+entry+r'\('+state+r' &s\) \{.*?\n\}'
            body=unique(pattern,raw,name+' '+entry+' raw body')
            output=unique(pattern,prepared,name+' '+entry+' prepared body')
            if digest(body)!=body_pin: raise ValueError(f'{name} {entry} complete raw body drift')
            expected=body.replace('selected(0x487ac4u,s.image)',MAPPED_OPERAND)
            if name=='contact':
                expected,count=INSTRUCTION.subn(lambda match:f's.instruction_literal({match[1]});',expected)
                if count!={'300100':29,'307fe0':2}[entry]: raise ValueError('Contact literal site count differs')
            if expected!=output: raise ValueError(f'{name} {entry} prepared body includes unexpected changes')
            bodies[entry]=body; prepared_bodies[entry]=output
        for format_label,selected_bodies in [('raw',bodies),('prepared',prepared_bodies)]:
            sections.append('namespace '+name+'_'+format_label+' {\n'+observation+resolver+'\n'+'\n'.join(selected_bodies.values())+'\n}\n')
        evidence[name]=dict(raw_file_sha256=digest(raw),prepared_file_sha256=digest(prepared),
            resolver_sha256=resolver_pin,dispatcher_sha256=dispatch_pin,raw_body_sha256=pins,
            prepared_body_sha256={entry:digest(body) for entry,body in prepared_bodies.items()},
            observation_helper_sha256=digest(observation) if observation else None,
            prepared_protocol='literal' if name=='contact' else 'direct selected/canonical')
    (scratch/'contact_copy_source_reference.inc').write_text(''.join(sections))
    return evidence


def change(text, old, new):
    if text.count(old)!=1: raise ValueError('Exact private source-copy defect seam drift: '+old)
    return text.replace(old,new,1)


def defects():
    original=(ROOT/FRAME).read_text()
    return {
        'direct-callback-lost': change(original,'s.services.instruction(pc);','s.instruction_literal(pc);'),
        'atomic-store-substituted': change(original,
            'physics_step_detail::vw(s_.memory, s_.observe, pc(canonical), s_.gp[0] + field, s_.xmm[0]);',
            'const auto bytes=std::bit_cast<std::array<std::byte,16>>(s_.xmm[0]); s_.memory.write(s_.gp[0]+field,bytes);'),
        'stale-callback-vector-value': change(change(original,'unsigned field, const NativeTu15ContactRecordVector&,',
            'unsigned field, const NativeTu15ContactRecordVector& cached,'),'s_.gp[0] + field, s_.xmm[0]);',
            's_.gp[0] + field, std::bit_cast<std::remove_cvref_t<decltype(s_.xmm[0])>>(cached));'),
        'selected-world-pc-lost': change(original,'return actual;','return canonical;'),
    }


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--mutations',action='store_true')
    args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ: parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'): return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    before={path.resolve():sha(path) for tree in ('include','src','tests','tools')
            for path in (ROOT/tree).rglob('*') if path.suffix in ('.hpp','.h','.inc','.cpp','.py') and path.is_file()}
    deps={ROOT/name for name in (FRAME,TEST,KERNEL,unit('contact'),unit('world'))}
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],compilers=[])
    try:
        def audit(event,args):
            if event=='open' and isinstance(args[0],str) and args[0].endswith('.py'):
                path=Path(args[0]).resolve()
                if path.is_relative_to(ROOT/'tools'): deps.add(path)
        sys.addaudithook(audit)
        report['source_authority']=references(scratch)
        generated_reference=(scratch/'contact_copy_source_reference.inc').resolve()
        report['extracted_reference_sha256']=sha(generated_reference)
        for module in tuple(sys.modules.values()):
            if path:=getattr(module,'__file__',None):
                path=Path(path).resolve()
                if path.is_relative_to(ROOT/'tools'): deps.add(path)
        variants=defects() if args.mutations else {}
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name; depfile=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O1','-g0','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),
                     '-MMD','-MF',str(depfile)]
            if args.sanitizers: command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=180)
            if result.returncode: raise ValueError('Source-copy build failed: '+result.stderr[-7000:])
            for name in shlex.split(depfile.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(name).resolve()
                if path.is_relative_to(ROOT) and path!=generated_reference: deps.add(path)
            tested=subprocess.run([str(binary)],cwd=ROOT,capture_output=True,text=True,timeout=120)
            if tested.returncode: raise ValueError('Source-copy baseline failed: '+(tested.stdout+tested.stderr)[-4000:])
            row=dict(compiler=compiler,command=command,result=json.loads(tested.stdout),compiled_defects=[])
            report['compilers'].append(row)
            for label,source in variants.items():
                overlay=scratch/('mutation-'+label); target=overlay/FRAME.removeprefix('src/'); target.parent.mkdir(parents=True,exist_ok=True)
                target.write_text(source); negative=scratch/(Path(compiler).name+'-'+label)
                cmd=[*command[:1],'-I'+str(overlay),*command[1:]]
                cmd[cmd.index('-o')+1]=str(negative);cmd[cmd.index('-MF')+1]=str(negative.with_suffix('.d'))
                built=subprocess.run(cmd,cwd=ROOT,capture_output=True,text=True,timeout=180)
                if built.returncode: raise ValueError(f'{label} defect build failed: '+built.stderr[-7000:])
                failed=subprocess.run([str(negative)],cwd=ROOT,capture_output=True,text=True,timeout=120)
                diagnostic=failed.stdout+failed.stderr
                if failed.returncode!=1 or not any(marker in diagnostic for marker in (
                    'source-copy ordered callback/provider/observer ABI differs','source-copy final ABI/FP/cursor differs',
                    'source-copy committed bytes differ')):
                    raise ValueError(f'{label} defect was not meaningfully rejected: '+diagnostic[-4000:])
                row['compiled_defects'].append(dict(name=label,exit_code=failed.returncode,sha256=digest(source),
                    diagnostic=diagnostic[-1500:],command=cmd))
        if sha(generated_reference)!=report['extracted_reference_sha256']:
            raise ValueError('Extracted authenticated reference bytes changed during comparisons')
        if not any(path.name=='physics_contact_runtime_internal.hpp' for path in deps) or \
           not any(path.name=='physics_world_runtime_internal.hpp' for path in deps):
            raise ValueError('Both actual State definitions must appear in the C++ input depfile')
        report.update(passed=True,scope='Private ABI/transport comparisons against four extracted complete source bodies from authentic current raw/prepared Contact and ObservedWorldState owners, with actual retained Memory/vw/cursor functions. No default, real World producer admission, other image, or physical MOVAPS claim.')
    except BaseException as error:
        report['error']=repr(error);raise
    finally:
        report['inputs_before']={str(path.relative_to(ROOT)):before.get(path) for path in sorted(deps)}
        report['inputs_after']={str(path.relative_to(ROOT)):sha(path) for path in sorted(deps)}
        if report['inputs_before']!=report['inputs_after']: report.update(passed=False,input_drift=True)
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__': raise SystemExit(main())
