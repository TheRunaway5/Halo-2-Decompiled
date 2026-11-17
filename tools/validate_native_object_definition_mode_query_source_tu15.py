#!/usr/bin/env python3
"""Compare 165590 with actual current/fresh CombatAction source and callbacks."""
import re
import subprocess
import sys

from specialize_native_operands import specialize
from validate_native_flight_property_query_source_tu15 import ROOT, STATE, SourceProfile, digest, main, one
from validate_native_object_definition_mode_query_frame_tu15 import corpus

UNIT='src/gameplay/units/source_combat_action_tu15_generated.cpp'
GENERATOR='tools/generate_combat_action_tu15.py'
FRAME='src/gameplay/objects/native_tu15_object_definition_mode_query_source_frame.hpp'
DRIVER='tests/native_tu15_object_definition_mode_query_source_tests.cpp'
BODY=re.compile(r'^static void run_165590\(WorldState &s\) \{.*?^\}',re.M|re.S)
CONTEXT=(STATE,re.compile(r'^std::uint32_t selected[^\n]+',re.M),
         re.compile(r'^static void dispatch[^\n]+',re.M),
         re.compile(r'^void source_run_combat_action_tu15\(.*?^\}',re.M|re.S),
         re.compile(r'^bool source_combat_action_tu15_owns_entry[^\n]+',re.M))
CURRENT_PIN='b87005071aecb83264930dfe7eb08fc7d71b4b5e258ac1db6dea42dd657ba2c7'


def prepare(scratch, require_binding=False):
    import native_object_definition_mode_query_replacements as binding
    from native_object_table_ratio_replacements import remove_object_table_ratio
    fresh=scratch/'fresh.cpp'
    command=[sys.executable,'-B',str(ROOT/GENERATOR),'--output',str(fresh),'--directory',str(scratch/'generation')]
    generated=subprocess.run(command,cwd=ROOT,capture_output=True,text=True,timeout=900)
    if generated.returncode:raise ValueError('Genuine CombatAction generation failed: '+generated.stderr[-8000:])
    actual=[(ROOT/UNIT).read_text(),fresh.read_text()]
    if require_binding:
        for text in actual:
            binding.installed_entries(text)
            output,receipt=specialize(text)
            if receipt.get('native_object_definition_mode_query_entries')!=binding.installed_entries(output):
                raise ValueError('Actual 165590 specialization missing')
    raw=[binding.remove_object_definition_mode_query(remove_object_table_ratio(text)) for text in actual]
    prepared=[specialize(text)[0] for text in raw]
    bodies=[one(BODY,text,'165590 source body') for text in raw+prepared]
    if digest(bodies[0])!=CURRENT_PIN:raise ValueError('Exact current 165590 body changed')
    contexts=[[one(pattern,text,'165590 complete owner') for pattern in CONTEXT] for text in raw+prepared]
    expected=[]
    for body,parts in zip(bodies,contexts,strict=True):
        resolver=parts[1]
        used=set(re.findall(r'selected\((0x[0-9a-f]+u),s\.image\)',body))
        mapping=dict(re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolver))
        if any(mapping.get(value)!=value for value in used):raise ValueError('165590 nonidentity operand')
        expected.append(re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',
                               r'if(s.services.instruction){s.services.instruction(\1);}',
                               re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',body)))
    if any(body!=expected[0] for body in expected[1:]):raise ValueError('165590 source profiles differ beyond identity retag')
    sites=[re.findall(r's\.(?:services\.instruction|instruction_literal)\((0x[0-9a-f]+u)\)',body) for body in bodies]
    native=set(re.findall(r'0x1655[0-9a-f]{2}u',(ROOT/FRAME).read_text()))
    if len(set(sites[0]))!=23 or any(row!=sites[0] for row in sites) or set(sites[0])!=native:
        raise ValueError('165590 source callback boundaries changed')
    result=[]
    for variant,name in enumerate(('current','fresh')):
        result+=['namespace '+name+' {','using namespace halo2::physics_world_runtime_detail;',
                 contexts[variant][0],contexts[variant][1],
                 bodies[variant].replace('run_165590(','raw(',1),
                 bodies[variant+2].replace('run_165590(','literal(',1),'}',
                 'static void run_reference('+name+'::WorldState&s,unsigned v){if(v&2u)'+name+'::literal(s);else '+name+'::raw(s);}']
    (scratch/'object_definition_mode_query_reference.inc').write_text('\n'.join(result)+'\n')
    return dict(generation=command,body_sha256=[digest(body) for body in bodies],
                context_sha256=[[digest(value) for value in row] for row in contexts],sites=23,
                canonical_identity_projection=True,registry_service=False)


PROFILE=SourceProfile(UNIT,GENERATOR,FRAME,DRIVER,prepare,corpus,'Object definition mode query',(
    ('wrong-mode','s.flags.minus(s.reg(6,4,0),9u,4);','s.flags.minus(s.reg(6,4,0),8u,4);'),
    ('skip-datum','step(0x1655a8u);return !s.flags.zf;','step(0x1655a8u);return false;'),
    ('wide-return','s.set(0,1u,1,0);','s.set(0,1u,4,0);'),
    ('wrong-word','s.flags.minus(read(g[2]+0x290u,2),1u,2);','s.flags.minus(read(g[2]+0x290u,2),0u,2);'),
),extra_pins=('tools/validate_native_object_definition_mode_query_source_tu15.py',
             'tools/native_object_definition_mode_query_replacements.py',
             'tools/native_object_table_ratio_replacements.py',
             'tools/native_projectile_function_query_replacements.py'))

if __name__=='__main__':
    raise SystemExit(main(PROFILE))
