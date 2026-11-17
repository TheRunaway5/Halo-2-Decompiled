#!/usr/bin/env python3
"""Compare private projectile query source transports with the complete actual owner body."""
import argparse
import hashlib
import json
import os
import platform
import re
import shlex
import subprocess
import sys
from pathlib import Path

import native_projectile_function_query_replacements as binding
from oracle_smg_firing_1_1 import Xbe
from specialize_native_operands import specialize
from validate_native_projectile_function_query_frame_tu15 import PIN, corpus

ROOT = Path(__file__).resolve().parents[1]
UNIT = 'src/gameplay/units/source_combat_action_tu15_generated.cpp'
SYMBOLS = 'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'
JOIN = 'src/rendering/effects/source_object_function_symbols_tu15.inc'
FRAME = 'src/gameplay/objects/native_tu15_projectile_function_query_source_frame.hpp'
TEST = 'tests/native_tu15_projectile_function_query_source_tests.cpp'
RAW_PIN = '1cc7aa2f4564c9854dd07cce158fb1f3149425562bf797d61c0154ba378d4d3f'
PREPARED_PIN = 'c5d1d0c81a46d4c0422ebaa88db32ff361eba6b68aa4470259503cf4b80d0fbc'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def body(text):
    found = re.findall(r'^static void run_100e70\(WorldState &s\) \{.*?^\}', text, re.M | re.S)
    if len(found) != 1:
        raise ValueError('Missing or ambiguous complete projectile function body')
    return found[0]


def prepare(scratch,require_binding):
    text = (ROOT / UNIT).read_text()
    installed=bool(binding.authenticate(text)[3])
    if require_binding:binding.installed_entries(text)
    from native_object_definition_mode_query_replacements import remove_object_definition_mode_query
    from native_object_table_ratio_replacements import remove_object_table_ratio
    text=binding.remove_projectile_function_query(remove_object_definition_mode_query(remove_object_table_ratio(text)))
    raw = body(text)
    if digest(raw.encode()) != RAW_PIN:
        raise ValueError('Projectile raw owner body changed')
    prepared = body(specialize(text)[0])
    if digest(prepared.encode()) != PREPARED_PIN:
        raise ValueError('Projectile prepared owner body changed')
    raw_sites = re.findall(r's\.services\.instruction\((0x[0-9a-f]+u)\)', raw)
    prepared_sites = re.findall(r's\.instruction_literal\((0x[0-9a-f]+u)\)', prepared)
    frame_sites = set(re.findall(r'0x100[ef][0-9a-f]{2}u', (ROOT / FRAME).read_text()))
    if len(set(raw_sites)) != 91 or raw_sites != prepared_sites or set(raw_sites) != frame_sites:
        raise ValueError('Projectile callback sites or formats changed')
    literal = sorted(set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)', raw)))
    resolver, = re.findall(r'^std::uint32_t selected\([^\n]+$', text, re.M)
    table = dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);', resolver))
    if len(literal) != 25 or any(table.get(key) != key for key in literal):
        raise ValueError('Projectile owner literals are no longer identity selected')
    names = {}
    symbols = re.findall(r'\bSymbol\{(0x[0-9a-f]+u),(0x[0-9a-f]+u),"([^"]+)"\}', (ROOT / SYMBOLS).read_text())
    joined = re.findall(r'ObjectFunctionSymbol\{0x[0-9a-f]+u,0x[0-9a-f]+u,0x[0-9a-f]+u,(0x[0-9a-f]+u),"([^"]+)"\}', (ROOT / JOIN).read_text())
    for key, name in [(mcc, name) for _, mcc, name in symbols] + joined:
        if names.setdefault(int(key[:-1], 16), name) != name:
            raise ValueError('Conflicting authenticated symbol names')
    if len(symbols) != 10 or not joined:
        raise ValueError('Projectile symbol table or authenticated join missing')
    source = ('static std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage){switch(p){'
        + ''.join(f'case {v}:return {v};' for v in literal)
        + 'default:throw std::runtime_error("Undeclared private projectile literal");}}\n')
    source += ('static std::string projectile_fixture_name(std::uint32_t p){switch(p){'
        + ''.join(f'case 0x{k:x}u:return {json.dumps(name)};' for k, name in sorted(names.items()))
        + 'default:throw std::runtime_error("Undeclared authenticated projectile name");}}\n')
    source += ('static std::uint32_t projectile_fixture_key(std::uint32_t p){switch(p){'
        + ''.join(f'case {key}:return {mcc};' for key, mcc, _ in symbols[5:])
        + 'default:return p;}}\n')
    source += binding.order_fallback(raw).replace('run_100e70', 'retained_projectile_raw', 1) + '\n'
    source += binding.order_fallback(prepared).replace('run_100e70', 'retained_projectile_literal', 1) + '\n'
    (scratch / 'projectile_function_query_reference.inc').write_text(source)
    return dict(raw_body_sha256=RAW_PIN, prepared_body_sha256=digest(prepared.encode()),
                source_resolver_sha256=digest(resolver.encode()), identity_literals=len(literal),
                sites=len(raw_sites), authenticated_name_ids=len(names),
                production_bound=installed, other_source_operations_changed=False,
                fallback_adjustment='Only 100F61 uses the original destination-first MULSS operand order',
                bound_service='actual SourceEffectCompiledSymbolsTu15')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbe', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--compiler', action='append')
    p.add_argument('--extended-fp', action='store_true')
    p.add_argument('--sanitizers', action='store_true')
    p.add_argument('--semantic-mutants', action='store_true')
    p.add_argument('--require-binding', action='store_true')
    a = p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        p.error('Use tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686') or not a.xbe.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    a.output.mkdir(parents=True, exist_ok=False)
    names = {UNIT, SYMBOLS, JOIN, FRAME, TEST, 'include/halo2/native_tu15_projectile_function_query_kernel.hpp'}
    for module in tuple(sys.modules.values()):
        path = getattr(module, '__file__', None)
        if path and (resolved := Path(path).resolve()).is_relative_to(ROOT / 'tools'):
            names.add(str(resolved.relative_to(ROOT)))
    pins = {name:digest((ROOT / name).read_bytes()) for name in sorted(names)}
    headers = {p.resolve():digest(p.read_bytes()) for tree in ('include','src','tests')
               for p in (ROOT / tree).rglob('*') if p.suffix in ('.h','.hpp','.inc')}
    report = dict(passed=False, production_bound=False, owned_scratch=str(scratch),
                  source_before=pins, retained_large_artifacts=[])
    try:
        if digest(a.xbe.read_bytes()) != PIN:
            raise ValueError('Wrong original reference image')
        report['source_authority'] = prepare(scratch,a.require_binding)
        report['production_bound'] = report['source_authority']['production_bound']
        fixture = scratch / 'inputs.json'
        fixture.write_text(json.dumps(corpus(Xbe(a.xbe), a.extended_fp), separators=(',',':')))
        report['fixture_sha256'] = digest(fixture.read_bytes())
        report['compilers'] = []
        for compiler in a.compiler or ('g++','clang++'):
            binary = scratch / Path(compiler).name
            common = [compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                      *('-I'+str(ROOT / tree) for tree in ('include','src','tests')),'-I'+str(scratch)]
            if a.sanitizers:
                common += ['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            commands=[];objects=[]
            for index,source in enumerate((TEST,SYMBOLS)):
                obj=binary.with_suffix(f'.{index}.o');dep=obj.with_suffix('.d');objects.append(str(obj))
                command=common+['-MD','-MF',str(dep),'-c',str(ROOT / source),'-o',str(obj)]
                built=subprocess.run(command,capture_output=True,text=True,timeout=600);commands.append(command)
                if built.returncode:
                    raise ValueError('Projectile source compile failed: '+built.stderr[-8000:])
                for item in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                    path=Path(item).resolve()
                    if path in headers:pins[str(path.relative_to(ROOT))]=headers[path]
            command=common+objects+['-o',str(binary)]
            linked=subprocess.run(command,capture_output=True,text=True,timeout=600);commands.append(command)
            if linked.returncode:
                raise ValueError('Projectile source link failed: '+linked.stderr[-8000:])
            tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=1200)
            if tested.returncode:
                raise ValueError('Projectile source comparison failed: '+(tested.stdout+tested.stderr)[-9000:])
            mutations=[]
            if a.semantic_mutants:
                original=(ROOT / FRAME).read_text()
                for label,before,after in (
                    ('literal-key','source_projectile_function_symbol_tu15(s_.memory,pc,key)','key'),
                    ('skip-live-branch','step(0x100f94u);if(s_.flags.zf)zero();','if(s_.flags.zf)zero();'),
                    ('late-push-esp','s_.push(0x100ea9u,s_.reg(3,4,0));',
                     'physics_world_runtime_detail::w(s_.memory,s_.observe,0x100ea9u,g[4]-4u,4,s_.reg(3,4,0));g[4]-=4u;'),
                    ('cached-return-byte','step(0x100fb1u);s_.set(0,s_.reg(3,1,0),1,0);',
                     'step(0x100fb1u);s_.set(0,1u,1,0);'),
                    ('commuted-multiply','ordered_multiply(s_.xmm[0],scalar(s_.gp[2]+0x194u))',
                     'ordered_multiply(scalar(s_.gp[2]+0x194u),s_.xmm[0])'),
                ):
                    if original.count(before) != 1:
                        raise ValueError('Missing projectile source defect seam '+label)
                    overlay=scratch/(Path(compiler).name+'-'+label)
                    header=overlay/FRAME;header.parent.mkdir(parents=True);header.write_text(original.replace(before,after,1))
                    mutant=overlay/'bad'
                    command=common[:1]+['-I'+str(overlay/'src')]+common[1:]+[str(ROOT/TEST),objects[1],'-o',str(mutant)]
                    built=subprocess.run(command,capture_output=True,text=True,timeout=600)
                    if built.returncode:
                        raise ValueError('Projectile source defect compile failed '+label+': '+built.stderr[-8000:])
                    result=subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=1200)
                    diagnostic=(result.stdout+result.stderr)[-8000:]
                    if not result.returncode or 'Projectile retained source differs' not in diagnostic:
                        raise ValueError('Projectile source defect survived/unrecognized '+label+': '+diagnostic)
                    mutations.append(dict(name=label,rejected=True,diagnostic=diagnostic,command=command))
            report['compilers'].append(dict(compiler=compiler, commands=commands, result=json.loads(tested.stdout),
                                            compiled_mutations=mutations))
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        report['source_after']={name:digest((ROOT / name).read_bytes()) for name in pins}
        if report['source_after'] != pins:report.update(passed=False, source_drift=True)
        (a.output / 'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
