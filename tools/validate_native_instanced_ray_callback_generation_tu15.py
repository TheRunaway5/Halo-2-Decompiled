#!/usr/bin/env python3
"""Regenerate both complete callback owners and compile raw/prepared translation units."""
import argparse
import contextlib
import hashlib
import io
import json
import os
from pathlib import Path
import runpy
import shlex
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report',type=Path,required=True)
    parser.add_argument('--compiler',default='clang++');args=parser.parse_args()
    os.chdir(ROOT)
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);report=dict(passed=False,
        owned_scratch=str(scratch),retained_large_artifacts=[],units=[])
    import native_instanced_ray_callback_replacements as binding
    from native_instanced_ray_callback_inputs import UNITS,PREPARED
    from validate_native_camera_instance_bitmap_generation_tu15 import generate_ai
    from specialize_native_operands import specialize
    source_pins={path:sha(ROOT/path)for path in UNITS}
    tool_pins={path.resolve():sha(path)for path in(ROOT/'tools').glob('*.py')}
    header_pins={path.resolve():sha(path)for tree in('src','include')for path in(ROOT/tree).rglob('*')if path.suffix in('.h','.hpp','.inc')}
    try:
        for name,path in zip(('primary','ai'),UNITS,strict=True):
            output=scratch/(name+'.cpp');row=dict(source=path,name=name);report['units'].append(row)
            if name=='ai':command,provenance=generate_ai(scratch);row['reference_inputs']=provenance
            else:
                command=[sys.executable,'-B',str(ROOT/'tools/generate_physics_live_support_tu15.py'),
                    '--output',str(output),'--manifest',str(scratch/'primary-generation')]
                old_argv=sys.argv
                try:
                    sys.argv=command[2:]
                    with contextlib.redirect_stdout(io.StringIO()):runpy.run_path(command[2],run_name='__main__')
                finally:sys.argv=old_argv
            source=output.read_text();canonical=(ROOT/path).read_text()
            assert binding.replace_instanced_ray_callbacks(source)==source,'Fresh callback native hook missing'
            assert binding.replace_instanced_ray_callbacks(canonical)==canonical,'Canonical callback binding missing'
            owners=binding.authenticate(source)
            for entry,match,body in owners:
                assert binding.PREFIXES[entry]in match[0]
                offset='0x10u'if entry==0x24def0 else '0x4u'
                changed=source.replace(match[0],match[0].replace(offset,'0x14u'if entry==0x24def0 else '0x8u',1),1)
                assert changed!=source
                try:binding.replace_instanced_ray_callbacks(changed)
                except ValueError:pass
                else:raise ValueError('Fresh callback whole-body drift admitted')
            prepared,receipt=specialize(source)
            assert tuple(binding.digest(pattern.search(prepared)[0])for pattern in binding.BODIES.values())==PREPARED
            assert specialize(prepared)[0]==prepared
            specialized=scratch/(name+'-prepared.cpp');specialized.write_text(prepared);syntax=[];dependencies=set()
            for artifact in(output,specialized):
                depfile=artifact.with_suffix('.d')
                command_line=[args.compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MD','-MF',str(depfile),str(artifact)]
                result=subprocess.run(command_line,capture_output=True,text=True,timeout=240)
                assert result.returncode==0,result.stderr[-6000:]
                syntax.append(dict(command=command_line,sha256=sha(artifact),passed=True))
                dependencies.update(Path(path).resolve()for path in shlex.split(depfile.read_text().replace('\\\n',' ').split(':',1)[1]))
            row.update(passed=True,generation_command=command,whole_unit_equal=source==canonical,
                callback_bodies={hex(entry):binding.digest(body)for entry,_,body in owners},
                prepared_bodies=PREPARED,raw_and_prepared_syntax=syntax,
                exact_native_prefixes_present=True,fresh_body_mutations_rejected=2,
                headers_sha256={str(path.relative_to(ROOT)):header_pins[path]for path in dependencies if path in header_pins})
        used={Path(module.__file__).resolve()for module in tuple(sys.modules.values())if getattr(module,'__file__',None)}
        used.add(Path(__file__).resolve())
        report['tools_sha256']={str(path.relative_to(ROOT)):pin for path,pin in tool_pins.items()if path in used}
        for path,pin in report['tools_sha256'].items():assert sha(ROOT/path)==pin,'Generation tool drift '+path
        for path,pin in source_pins.items():assert sha(ROOT/path)==pin,'Canonical source drift '+path
        for row in report['units']:
            for path,pin in row['headers_sha256'].items():assert sha(ROOT/path)==pin,'Generation header drift '+path
        report.update(passed=True,canonical_source_sha256=source_pins,original_instruction_or_outcome_replacement=False)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
