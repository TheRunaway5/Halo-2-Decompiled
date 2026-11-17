#!/usr/bin/env python3
"""Compare private retained 120570 to both complete source owners and real child."""
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

from validate_native_codec_inverse_transform_source_tu15 import ORDERED, exact
from validate_native_codec_quaternion_product_tu15 import OWNERS as PRODUCT_OWNERS
from validate_native_codec_transform_composition_tu15 import (
    ENTRY, FRAME, KERNEL, OWNERS, PRODUCT, REPEATS, RET, ROOT, STACK_READS, cases, original_owners, product_owners, sha,
)
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

SOURCE = 'src/physics/runtime/native_tu15_codec_transform_composition_source_frame.hpp'
TEST = 'tests/native_tu15_codec_transform_composition_source_tests.cpp'


def retained_sources(code,scratch):
    blocks,contexts = [],{}
    for index,(name,pin) in enumerate(OWNERS.items()):
        text = (ROOT/name).read_text()
        body = exact(text,r'static void run_120570\(WorldState &s\) \{.*?\n\}')
        child = exact(text,r'static void run_1201e0\(WorldState &s\) \{.*?\n\}')
        if hashlib.sha256(body.encode()).hexdigest()!=pin or hashlib.sha256(child.encode()).hexdigest()!=PRODUCT_OWNERS[name]:
            raise ValueError('Exact composition or product body changed')
        state = exact(text,r'struct LiveState : physics_world_runtime_detail::WorldState \{.*?\n\};\nusing WorldState=LiveState;')
        resolver = exact(text,r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}')
        def ordered(source):
            lines,sites = [],[]
            for line in source.splitlines():
                operators = re.findall(ORDERED,line)
                if operators:
                    pc = int(re.search(r's.services.instruction\(0x([0-9a-f]+)u\)',line)[1],16)
                    if len(operators)!=1 or code.at(pc).mnemonic!=operators[0]+'ss':
                        raise ValueError('Original composition ordered operand changed')
                    sites.append(pc)
                    line = re.sub(ORDERED,lambda match:'exact_'+match[1]+'(',line)
                lines.append(line)
            return '\n'.join(lines),sites
        parent_ordered,parent_sites = ordered(body)
        child_ordered,child_sites = ordered(child)
        if len(child_sites)!=28 or body.count('run_1201e0(s);')!=1:
            raise ValueError('Actual product scope changed')
        helpers = ''.join(f'static __m128 exact_{name}(__m128 left,__m128 right){{asm volatile("{name}ss %1,%0":"+x"(left):"x"(right):"memory");return left;}}\n'
                          for name in ('add','sub','mul'))
        blocks.append(f'namespace composition_source{index}{{using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
            +state+'\n'+resolver+'\n'+helpers
            +child.replace('run_1201e0(','legacy_product(',1)+'\n'
            +child_ordered.replace('run_1201e0(','ordered_product(',1)+'\n'
            +body.replace('run_120570(','legacy(',1).replace('run_1201e0(s);','legacy_product(s);',1)+'\n'
            +parent_ordered.replace('run_120570(','ordered(',1).replace('run_1201e0(s);','ordered_product(s);',1)+'\n}\n')
        contexts[name] = dict(body=pin,product=PRODUCT_OWNERS[name],resolver=hashlib.sha256(resolver.encode()).hexdigest(),
            state=hashlib.sha256(state.encode()).hexdigest(),parent_ordered_sites=list(map(hex,parent_sites)),child_ordered_sites=list(map(hex,child_sites)))
    generated = ''.join(blocks)
    (scratch/'codec_transform_composition_sources.inc').write_text(generated)
    return contexts,hashlib.sha256(generated.encode()).hexdigest()


def publication(image,code):
    import oracle_native_campaign_screen_lifecycle_tu15 as common
    common.ENTRIES=(ENTRY,PRODUCT)
    parent = set(_trace(code,ENTRY,{ENTRY,PRODUCT}).code)
    successors,stores = {},[]
    for site in common.metadata(image)['sites']:
        pc = site['instruction']
        inst = code.at(pc)
        implicit_store = inst.mnemonic in ('push','call')
        memory = bool(site['operands']) or implicit_store or pc in STACK_READS or pc in REPEATS
        if pc not in REPEATS and (implicit_store or any(op['write'] for op in site['operands'])):
            stores.append(pc)
        if pc in parent and pc!=RET and memory:
            # For a completed source REP element, re-enter the real repeat
            # callback at its original PC, including the final count-zero call.
            successors[pc] = pc if pc in REPEATS else pc+inst.size
    if len(parent)!=114 or len(stores)<20 or not {0x1205ad,*REPEATS}<=successors.keys():
        raise ValueError('Complete composition publication metadata changed')
    return successors,stores


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    for option in ('smoke','mmx','sanitizers'):parser.add_argument('--'+option,action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    if platform.system()!='Linux' or platform.machine().lower() not in ('x86_64','amd64'):return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True,exist_ok=False)
    before = {path.resolve():sha(path) for tree in ('tools','include','src','tests') for path in (ROOT/tree).rglob('*')
              if path.is_file() and path.suffix in ('.py','.cpp','.hpp','.inc','.h')}
    used = {ROOT/KERNEL,ROOT/FRAME,ROOT/SOURCE,ROOT/TEST,Path(__file__).resolve()}
    report = dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],
                  readonly_gp_xmm_fp_metadata_host_controls_status=True,compilers=[])
    try:
        if sha(NEW_DEFAULT)!=NEW_PIN:raise ValueError('Exact original English TU1.5 changed')
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
        original=original_owners();product=product_owners()
        contexts,generated=retained_sources(code,scratch)
        successors,stores=publication(image,code)
        rows=cases(image,args.smoke);normal=len(rows)
        if args.mmx:
            rng=random.Random(ENTRY)
            for mask in range(16 if args.smoke else 256):
                row=copy.deepcopy(rows[mask%normal])
                row.update(name=row['name']+f'-opaque-{mask}',live_x87=[])
                row['before_fp'][1]&=~0x3800
                row['active_mmx']=dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()
                    for n in range(8)])
                rows.append(row)
        report.update(contexts=contexts,generated_source_sha256=generated,normal_rows=normal,opaque_rows=len(rows)-normal,retirements=successors)
        fixture=scratch/'fixture.json'
        fixture.write_text(json.dumps(dict(cases=rows,successors=successors,write_sites=stores),separators=(',',':')))
        for compiler in args.compiler or ('g++','clang++'):
            binary=scratch/Path(compiler).name
            dep=binary.with_suffix('.d')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                     '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-I'+str(scratch),str(ROOT/TEST),'-o',str(binary),
                     '-MD','-MF',str(dep)]
            if args.sanitizers:command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            done=subprocess.run(command,capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Retained composition compile failed: '+done.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path=Path(value).resolve()
                if path in before:used.add(path)
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:raise ValueError('Retained composition failed: '+(done.stdout+done.stderr)[-10000:])
            report['compilers'].append(dict(compiler=compiler,result=json.loads(done.stdout)))
        if original_owners()!=original or product_owners()!=product or retained_sources(code,scratch)!=(contexts,generated):
            raise ValueError('Exact composition/child source context drift')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None) and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before']!=report['source_after']:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
