#!/usr/bin/env python3
"""Measure the genuine current and fresh 293290 source producer contexts."""
import argparse
import hashlib
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from specialize_native_operands import specialize
from validate_native_model_marker_lookup_generation_tu15 import CHILD_WRAPPER

ROOT = Path(__file__).resolve().parents[1]
ENTRY = 0x293290
UNITS = (
    ('primary', 'src/physics/physics_live_support_tu15_generated.cpp',
     'generate_physics_live_support_tu15.py', '--manifest'),
    ('combat', 'src/gameplay/units/source_combat_action_tu15_generated.cpp',
     'generate_combat_action_tu15.py', '--directory'),
)
PATTERNS = tuple(map(re.compile, (
    r'^static void run_293290\(WorldState &s\) \{.*?^\}',
    r'^std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',
    r'^static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
), (re.M|re.S, re.M, re.M, re.M|re.S)))
SELECTED = re.compile(r'selected\((0x[0-9a-f]+u),s\.image\)')
LITERAL = re.compile(r's\.instruction_literal\((0x[0-9a-f]+u)\);')
BODY_PINS = (
    '80f4f97d339c22c9469bae67a3b4638860be3fac5f2c9ba205981c20f51e43d2',
    '02f6aa94c9fe69acfb537b49a294b38543e21931b2123a891e4618193c02bc6d',
)


def sha(value):
    return hashlib.sha256(value if isinstance(value, bytes) else value.encode()).hexdigest()


def parts(source):
    result=[]
    for pattern in PATTERNS:
        matches=pattern.findall(source)
        if len(matches)!=1:raise ValueError('Missing or ambiguous format-four owner context: '+pattern.pattern)
        result.append(matches[0])
    return tuple(result)


def identity_projection(body,resolver):
    mapping=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
    operands=SELECTED.findall(body)
    if any(mapping.get(value)!=value for value in operands):
        raise ValueError('Format-four owner has a nonidentity TU1.5 selected operand')
    return SELECTED.sub(r'\1',body),len(operands)


def dispatch_arm(dispatcher,pc):
    token=f'{pc:#x}u'
    candidates=(f'case {token}:run_{pc:x}(s);return;',
                f'if(target==selected({token},s.image)){{run_{pc:x}(s);return;}}',
                f'if(target=={token}){{run_{pc:x}(s);return;}}')
    found=[arm for arm in candidates for _ in range(dispatcher.count(arm))]
    if len(found)>1:raise ValueError('Ambiguous actual format-four dispatch arm')
    return found[0] if found else None


def generate(scratch,unit):
    owner,_,generator,argument=unit
    output=scratch/(owner+'-fresh.cpp');imports=scratch/(owner+'-imports.json')
    destination=scratch/(owner+'-manifest.json' if owner=='primary' else owner+'-generation')
    command=[sys.executable,'-B','-c',CHILD_WRAPPER,str(imports),str(ROOT/'tools'/generator),
             '--output',str(output),argument,str(destination)]
    result=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=600)
    if result.returncode:raise ValueError('Genuine '+owner+' generation failed: '+(result.stdout+result.stderr)[-9000:])
    return output.read_text(),[Path(value)for value in json.loads(imports.read_text())],command[5:]


def measure(scratch, report):
    if sha(NEW_DEFAULT.read_bytes())!=NEW_PIN:raise ValueError('Original format-four code image drift')
    code=_Code(Xbe(NEW_DEFAULT),'.text');sites=sorted(_trace(code,ENTRY,{ENTRY}).code)
    if len(sites)!=221:raise ValueError('Original format-four instruction set changed')
    frontiers={pc:pc+code.at(pc).size for pc in sites if code.at(pc).mnemonic in ('push','pop','call') or
               (code.at(pc).mnemonic not in ('lea','ret') and any(op.type==3 for op in code.at(pc).operands))}
    if not set(frontiers.values())<=set(sites):raise ValueError('Format-four retirement successor left the original owner')
    report['retirement_successors']={hex(pc):hex(next_pc)for pc,next_pc in frontiers.items()}
    report['original_code_sha256']=sha(b''.join(bytes(code.at(pc).bytes)for pc in sites))
    report['original_sse']={hex(pc):[code.at(pc).mnemonic,code.at(pc).op_str,bytes(code.at(pc).bytes).hex()]for pc in sites
                            if code.at(pc).mnemonic in ('mulps','addps','subps','mulss','addss')}
    current={(unit[0]):(ROOT/unit[1]).read_text() for unit in UNITS}
    canonical=parts(current['primary'])[0]
    for unit,pin in zip(UNITS,BODY_PINS,strict=True):
        if sha(parts(current[unit[0]])[0])!=pin:raise ValueError('Current exact format-four source body changed')
    tool_before={p:sha(p.read_bytes())for p in (ROOT/'tools').rglob('*.py')}
    with ThreadPoolExecutor(max_workers=2) as pool:
        generated=list(pool.map(lambda unit:generate(scratch,unit),UNITS))
    for unit,(fresh,imports,command) in zip(UNITS,generated,strict=True):
        owner,path,_,_=unit;record=dict(owner=owner,source=path,command=command,variants=[]);report['owners'].append(record)
        for stage,source in (('current',current[owner]),('fresh',fresh)):
            for form,text in (('raw',source),('prepared',specialize(source)[0])):
                body,resolver,dispatcher,state=parts(text)
                projected,count=identity_projection(body,resolver)
                normalized=LITERAL.sub(r'if(s.services.instruction){s.services.instruction(\1);}',projected)
                if normalized!=canonical:
                    import difflib
                    record['body_delta']=list(difflib.unified_diff(canonical.splitlines(),normalized.splitlines(),n=0))[:24]
                calls=re.findall(r'(?:s\.services\.instruction|s\.instruction_literal)\((?:selected\()?0x([0-9a-f]+)u',body)
                if [int(pc,16)for pc in calls]!=sites:raise ValueError('Format-four prepared statement order changed')
                caller,=re.findall(r'^static void run_284290\(WorldState &s\) \{.*?^\}',text,re.S|re.M)
                if caller.count('run_293290(s);return;')!=1:
                    raise ValueError('Owning source lacks its actual local 293290 tail call')
                local_dispatch=dispatch_arm(dispatcher,ENTRY)
                if not local_dispatch:
                    raise ValueError('Actual Primary/CombatAction 293290 dispatch contract changed')
                local_children=[pc for pc in (0x2963d0,0x296410,0x2cec60)
                                if dispatch_arm(dispatcher,pc)]
                if stage=='current' and local_children!=([0x2963d0,0x296410] if owner=='primary' else []):
                    raise ValueError('Actual format-four child dispatch ownership changed')
                (scratch/f'{owner}-{stage}-{form}.cpp').write_text(text)
                record['variants'].append(dict(stage=stage,form=form,context=[sha(v)for v in (body,resolver,dispatcher,state)],
                                               source_sha256=sha(text),identical_projection=normalized==canonical,
                                               selected_operand_occurrences=count,caller_body_sha256=sha(caller),
                                               local_dispatch=local_dispatch,local_children=list(map(hex,local_children))))
        used=[p for p in imports if p in tool_before]
        if any(sha(p.read_bytes())!=tool_before[p]for p in used) or (ROOT/path).read_text()!=current[owner]:
            raise ValueError('Format-four source/generation dependency changed during measurement')
        record['generation_tools']={str(p.relative_to(ROOT)):tool_before[p]for p in used}
    return report


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:parser.error('Use tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,private_unbound=True,owners=[],owned_scratch=str(scratch),retained_large_artifacts=[])
    try:
        measure(scratch,report);report['passed']=True
    except BaseException as error:report['error']=repr(error);raise
    finally:(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0


if __name__=='__main__':raise SystemExit(main())
