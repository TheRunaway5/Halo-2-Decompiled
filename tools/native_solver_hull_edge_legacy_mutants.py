"""Private genuine include-overlay controls for the shared public313470 candidate."""
import hashlib
from pathlib import Path
from validate_native_solver_hull_edge_value_tu15 import FUNCTION,OLD_PIN
ROOT=Path(__file__).resolve().parents[1]
NEW_PIN='f2310926f3894fbad1f53de774bb6660a12a59abea437ed5c526ca9c7aa6fd95'
CONTROLS="(('wrong_direction','xor_bits(axis,sign)','axis'),('wrong_choice','if(!(relation&0x100u)){','if(relation&0x100u){'),('wrong_scalar_output','output+0x30u,value','output+0x34u,value'))"
def retarget_edge_mutants(source):
    if CONTROLS not in source:raise ValueError('Complete legacy Edge defect tuple changed')
    public=(ROOT/'src/physics/native_tu15_solver_hull_edge.cpp').read_text();matches=list(FUNCTION.finditer(public))
    if len(matches)!=1:raise ValueError('Missing complete legacy public Edge owner')
    pin=hashlib.sha256(matches[0][0].encode()).hexdigest()
    if pin==OLD_PIN:return source
    if pin!=NEW_PIN:raise ValueError('Unknown complete public Edge body for legacy migration')
    begin=source.index('  variants={}\n');end=source.index("\n  (project/'negative.py')",begin)
    controls=(('wrong_direction','return _mm_xor_ps(a,b);','return a;',1),('wrong_choice','return !(relation&0x100u);','return bool(relation&0x100u);',1),('wrong_scalar_output','output+0x30u,d.selected_projection','output+0x34u,d.selected_projection',2))
    replacement="  variants={}\n  for name,needle,replacement,count in "+repr(controls)+":\n   relative='halo2/native_tu15_solver_hull_edge_value.hpp';header=(ROOT/'include'/relative).read_text();assert header.count(needle)==count\n   target=project/name/'include'/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(needle,replacement));variants[name]=text;(project/(name+'.cpp')).write_text(text)"
    source=source[:begin]+replacement+source[end:]
    old='target_include_directories(moving_{name} PRIVATE "{project}/include")';new='target_include_directories(moving_{name} BEFORE PRIVATE "{project}/{name}/include" "{project}/include")'
    if source.count(old)!=1 or source.count('files=[source,')!=1:raise ValueError('Legacy Edge compiler project construction drift')
    return source.replace(old,new,1).replace('files=[source,',"files=[ROOT/'include/halo2/native_tu15_solver_hull_edge_kernel.hpp',ROOT/'include/halo2/native_tu15_solver_hull_edge_value.hpp',source,",1)

def inject_edge_generation(text):
    """Apply the mutation hook only to the innermost emitted compiler main."""
    outer=" namespace=dict(parent.__dict__,cases=cases,driver_source=driver_source,__doc__=__doc__);exec(text,namespace);return namespace['main']()"
    inner=" environment=dict(base.__dict__,driver_source=driver_source,__doc__=__doc__);exec(source,environment);return environment['main']()"
    if text.count(outer)!=1:raise ValueError('Exact nested legacy Edge generation seam changed')
    replacement=' text=text.replace('+repr(inner)+','+repr(' source=retarget_edge_mutants(source)\n'+inner)+',1)\n'
    replacement+=" namespace=dict(parent.__dict__,cases=cases,driver_source=driver_source,retarget_edge_mutants=retarget_edge_mutants,__doc__=__doc__);exec(text,namespace);return namespace['main']()"
    return text.replace(outer,replacement,1)
