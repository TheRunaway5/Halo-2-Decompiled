#!/usr/bin/env python3
"""Strict current/fresh MapWorld diagnostic overlays and full-unit syntax."""
import argparse
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import re
import runpy
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import native_diagnostic_stream_replacements as binding
UNIT='src/runtime/source_map_world_support_tu15_generated.cpp'


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');parser.add_argument('--require-binding',action='store_true');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);canonical=sha(ROOT/UNIT)
    tools={p.resolve():sha(p)for p in(ROOT/'tools').rglob('*.py')}
    headers={p.resolve():sha(p)for tree in('src','include')for p in(ROOT/tree).rglob('*')if p.suffix in('.h','.hpp','.inc')}
    report=dict(passed=False,owned_scratch=str(scratch),retained_large_artifacts=[],routes=[])
    try:
        raw=scratch/'fresh.cpp';manifest=scratch/'generation.json';saved=sys.argv
        try:
            sys.argv=[str(ROOT/'tools/generate_map_world_support_tu15.py'),'--output',str(raw),'--manifest',str(manifest)]
            with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(sys.argv[0],run_name='__main__')
        finally:sys.argv=saved
        current=(ROOT/UNIT).read_text();fresh=raw.read_text()
        from specialize_native_operands import specialize
        for name,source in(('current',current),('fresh',fresh)):
            authenticated=binding.authenticate(source);bound=binding.replace_diagnostic_stream(source)
            if args.require_binding and bound!=source:raise ValueError(name+' diagnostic production/generated binding missing')
            if binding.replace_diagnostic_stream(bound)!=bound:raise ValueError('Diagnostic binding is nonidempotent '+name)
            controls=[]
            candidates=[]
            for match,entry,body in authenticated:
                mutated=body.replace('0x32ba80u','0x32ba81u',1)if entry==0x2e2d10 else body.replace('s.set(1,','s.set(2,',1)if entry in(0x2e3080,0x2e30e0)else body.replace('s.set(0,','s.set(2,',1)
                if mutated==body:raise ValueError('Missing complete diagnostic body mutant')
                candidates.append((hex(entry),source[:match.start()]+mutated+source[match.end():]))
            for context in('selected','dispatch',*binding.PUBLIC_PINS):
                function=binding.function(source,context)
                candidates.append((context,source.replace(function,function.replace('{','{if(false)return;',1),1)))
            for control,mutated in candidates:
                try:binding.replace_diagnostic_stream(mutated)
                except ValueError:controls.append(dict(name=control,rejected=True))
                else:raise ValueError('Diagnostic complete-context mutation survived '+control)
            prepared,receipt=specialize(bound)
            if specialize(prepared)[0]!=prepared:raise ValueError('Diagnostic repeated preparation differs '+name)
            syntax=[];dependencies=set()
            for shape,text in(('raw',bound),('prepared',prepared)):
                path=scratch/(name+'-'+shape+'.cpp');path.write_text(text);dep=path.with_suffix('.d')
                command=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(dep),str(path)]
                checked=subprocess.run(command,capture_output=True,text=True,timeout=240)
                if checked.returncode:raise ValueError('Diagnostic whole-unit syntax failed '+name+'/'+shape+': '+checked.stderr[-7000:])
                dependencies.update(Path(x).resolve()for x in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]))
                syntax.append(dict(shape=shape,passed=True,sha256=sha(path)))
            report['routes'].append(dict(name=name,source_sha256=binding.digest(source),
                body_sha256={hex(entry):binding.digest(body)for _,entry,body in authenticated},
                bound_body_sha256={hex(entry):binding.digest(binding.function(bound,f'run_{entry:x}'))for entry in binding.ENTRIES},
                prepared_body_sha256={hex(entry):binding.digest(binding.function(prepared,f'run_{entry:x}'))for entry in binding.ENTRIES},
                syntax=syntax,strict_source_mutations=controls,
                header_sha256={str(p.relative_to(ROOT)):headers[p]for p in dependencies if p in headers}))
        used={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)};used.add(Path(__file__).resolve())
        for path in used:
            if path in tools and sha(path)!=tools[path]:raise ValueError('Diagnostic generation preparation drift '+str(path.relative_to(ROOT)))
        for route in report['routes']:
            for path,pin in route['header_sha256'].items():
                if sha(ROOT/path)!=pin:raise ValueError('Diagnostic generation header drift '+path)
        if sha(ROOT/UNIT)!=canonical:raise ValueError('Diagnostic canonical source drift')
        report.update(passed=True,canonical_sha256=canonical,generation_manifest_sha256=sha(manifest),
            preparation_sha256={str(p.relative_to(ROOT)):tools[p]for p in used if p in tools})
    except Exception as error:report['error']=str(error)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({key:report[key]for key in('passed','error')if key in report}));return 0 if report['passed']else 1


if __name__=='__main__':raise SystemExit(main())
