"""Preserve legacy pair defects after the public algorithm moves into shared views."""
import hashlib
from pathlib import Path
from validate_native_solver_hull_support_pair_value_tu15 import FUNCTION,OLD_PIN,NEW_PIN

ROOT=Path(__file__).resolve().parents[1]
PAIR_CONTROLS="(('wrong_transpose_order','add(add(xterm,yterm),zterm)','add(add(zterm,yterm),xterm)'),('wrong_final_translation','add(zterm,translation)','add(zterm,zterm)'),('wrong_second_output','point_pair+0x10u);','point_pair+0x20u);'))"

def retarget_pair_mutants(source):
    # Initializer/query descendants deliberately replace this exact tuple with
    # their own owner defects. Their construction and include paths stay intact.
    if PAIR_CONTROLS not in source:return source
    public=(ROOT/'src/physics/native_tu15_solver_hull_support.cpp').read_text()
    matches=list(FUNCTION.finditer(public))
    if len(matches)!=1:raise ValueError('Missing complete legacy public pair owner')
    pin=hashlib.sha256(matches[0][0].encode()).hexdigest()
    if pin==OLD_PIN:return source
    if pin!=NEW_PIN:raise ValueError('Legacy pair mutation target has unknown complete public body')
    begin=source.index('  variants={}\n');end=source.index("\n  (project/'negative.py')",begin)
    controls=(
        ('wrong_transpose_order','halo2/native_tu15_solver_hull_support_pair_kernel.hpp','    d.xy_sum=view.add(d.x_term,d.y_term);\n    d.transformed=view.add(d.xy_sum,d.z_term);','    d.xy_sum=view.add(d.z_term,d.y_term);\n    d.transformed=view.add(d.xy_sum,d.x_term);',1),
        ('wrong_final_translation','halo2/native_tu15_solver_hull_support_pair_kernel.hpp','view.add(d.final_z_term,d.translation)','view.add(d.final_z_term,d.final_z_term)',2),
        ('wrong_second_output','halo2/native_tu15_solver_hull_support_pair_value.hpp','d.second_output=point_pair+16u;','d.second_output=point_pair+32u;',1),
    )
    replacement="  variants={}\n  for name,relative,needle,replacement,count in "+repr(controls)+":\n   header=(ROOT/'include'/relative).read_text();assert header.count(needle)==count\n   target=project/name/'include'/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(needle,replacement));variants[name]=text;(project/(name+'.cpp')).write_text(text)"
    source=source[:begin]+replacement+source[end:]
    old='target_include_directories(moving_{name} PRIVATE "{project}/include")'
    new='target_include_directories(moving_{name} BEFORE PRIVATE "{project}/{name}/include" "{project}/include")'
    if source.count(old)!=1 or source.count('files=[source,')!=1:raise ValueError('Legacy pair CMake/hash construction drift')
    source=source.replace(old,new,1).replace('files=[source,',"files=[ROOT/'include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp',ROOT/'include/halo2/native_tu15_solver_hull_support_pair_value.hpp',source,",1)
    return source
