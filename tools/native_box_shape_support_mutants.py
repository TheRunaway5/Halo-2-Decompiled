"""Owned source-order/bitwise negative controls for actual box support."""
import subprocess
def run_mutants(root,scratch,command,inputs):
    kernel='include/halo2/native_tu15_box_shape_support_kernel.hpp';frame='src/physics/runtime/native_tu15_box_shape_support_frame.hpp';result={}
    for name,path,relative,old,new in[
        ('omit_direction_sign',kernel,'halo2/native_tu15_box_shape_support_kernel.hpp','extent[n]^result.masked_direction[n]','extent[n]'),
        ('reverse_feature_bits',kernel,'halo2/native_tu15_box_shape_support_kernel.hpp','(result.point[n]>>31u)<<n','(result.point[n]>>31u)<<(2u-n)'),
        ('leave_payload_lane_unselected',kernel,'halo2/native_tu15_box_shape_support_kernel.hpp','n<4;++n','n<3;++n'),
        ('query_before_initial_output_store',frame,'physics/runtime/native_tu15_box_shape_support_frame.hpp','store_vector(0x30679du,g[0],s_.xmm[0]);\n        s_.xmm[1]=vector(0x3067a0u,0x461960u);s_.xmm[2]=vector(0x3067a7u,g[1]);','s_.xmm[2]=vector(0x3067a7u,g[1]);store_vector(0x30679du,g[0],s_.xmm[0]);\n        s_.xmm[1]=vector(0x3067a0u,0x461960u);'),
    ]:
        text=(root/path).read_text()
        if text.count(old)!=1:raise ValueError('Box-support mutation seam drift '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Box-support mutant compile failed '+name+': '+built.stderr[-5000:])
        with inputs.open('rb')as inp:run=subprocess.run([str(overlay/'probe')],stdin=inp,capture_output=True,text=True)
        if not run.returncode:raise ValueError('Box-support mutant accepted '+name)
        result[name]=dict(rejected=True,diagnostic=run.stderr[-5000:])
    return result
