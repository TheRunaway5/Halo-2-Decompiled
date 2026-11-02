#!/usr/bin/env python3
"""Genuine1478 typed31D930 input through the sole native-only joint helper.

Independent original thiscall2/RET8 assembly, all captured banks and ordered
retained operands/stores/FP. Profile0 is the actual captured FP; other profiles
vary only the declared rounding controls. No callback result is supplied.
"""
import inspect
from pathlib import Path
import validate_native_look_reorientation_actual_tu15 as common
import validate_native_joint_constraint_build_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-linear-jacobian-host-stage-20261102/staged'
UNITS=('native_tu15_joint_constraint_build.cpp','native_tu15_joint_constraint_schema.cpp','native_tu15_joint_constraint_angular_row.cpp','native_tu15_joint_constraint_motor.cpp','native_tu15_joint_constraint_limits.cpp','native_tu15_constraint_angle.cpp')
def observer_source():
 text=(ROOT/'tools/observe_native_look_reorientation_actual_tu15.cpp').read_text()
 text=text.replace('typed.at("control_index")','typed.at("constraint")').replace('typed.at("forward")','typed.at("jacobian_input")')
 text=text.replace('instructions<64','instructions<4096').replace('0x0200f004u','0x0200f00cu').replace('Original18A2A0 RET0','Original31D930 RET8')
 text=text.replace('NativeTu15LookReorientationBindingActivity','NativeTu15BoundJointConstraintBuildActivity')
 text=text.replace('native_tu15_reorient_bound_look_controller(memory,index,forward,{fp.control_word,fp.status,fp.mxcsr,0},true,observe,&activity);','native_tu15_build_bound_joint_constraint(memory,index,{forward},{typed.at("builder_packet")},fp,0,observe,&activity);')
 text=text.replace('activity.reorientation','activity.parent').replace('activity.parent.yaw_wraps','activity.parent.limits').replace('"yaw_wraps"','"limits"')
 return text
def main():
 text=inspect.getsource(common.main)
 text=text.replace("typed['owner']==0x18a2a0","typed['owner']==0x31d930")
 text=text.replace('body=complete_body(code,0x18a2a0)','body=complete_body(code,0x31d930)').replace("'0018a2a0.txt'","'0031d930.txt'")
 text=text.replace('meta=parent.metadata(image)','meta=parent.metadata(image)')
 text=text.replace("case=dict(entry=0x18a2a0,receiver=typed['control_index'],arguments=[]","case=dict(entry=0x31d930,receiver=typed['constraint'],arguments=[typed['jacobian_input'],typed['builder_packet']]")
 text=text.replace('halo2/native_tu15_look_reorientation_binding.hpp','halo2/native_tu15_joint_constraint_build_binding.hpp')
 text=text.replace("(ROOT/'tools/observe_native_look_reorientation_actual_tu15.cpp').read_text()","observer_source()")
 old="str(ROOT/'src/physics/native_tu15_look_reorientation.cpp')"
 assert text.count(old)==1
 text=text.replace(old,','.join("str(ROOT/'src/physics'/"+repr(name)+")"for name in UNITS)+",str(STAGE/'src/physics/native_tu15_linear_jacobian_emitter.cpp')")
 begin=text.index("  files=(");end=text.index(";report.update(",begin)
 files=tuple('src/physics/'+name for name in UNITS)+('include/halo2/native_tu15_joint_constraint_build.hpp','include/halo2/native_tu15_joint_constraint_build_binding.hpp','include/halo2/native_tu15_linear_jacobian_host.hpp','src/physics/native_tu15_joint_constraint_numeric_internal.hpp','tools/validate_native_joint_constraint_build_actual_tu15.py','tools/observe_native_look_reorientation_actual_tu15.cpp','tools/validate_native_look_reorientation_actual_tu15.py')
 text=text[:begin]+'  files='+repr(files)+text[end:]
 text=text.replace("source_hashes={n:sha(ROOT/n)for n in files}","source_hashes={n:sha(ROOT/n)for n in files},linear_host_source_sha256=sha(STAGE/'src/physics/native_tu15_linear_jacobian_emitter.cpp')")
 environment=dict(common.__dict__,parent=parent,STAGE=STAGE,observer_source=observer_source,__doc__=__doc__);exec(text,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
