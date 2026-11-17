"""Owned data/ABI/lazy-argument negative controls for the native zero owner."""
import subprocess

def run_mutants(root,scratch,command,inputs):
    kernel='include/halo2/native_tu15_zero_shape_support_kernel.hpp';frame='src/physics/runtime/native_tu15_zero_shape_support_frame.hpp';result={}
    for name,path,relative,old,new in[
        ('nonzero_point_payload',kernel,'halo2/native_tu15_zero_shape_support_kernel.hpp','noexcept{return {};}','noexcept{return {0,0,0,1};}'),
        ('omit_escaped_XMM_zero',frame,'physics/runtime/native_tu15_zero_shape_support_frame.hpp','std::memcpy(&s_.xmm[0],point.data(),16);','/* Incorrectly retain incoming XMM0. */'),
        ('wrong_caller_argument',frame,'physics/runtime/native_tu15_zero_shape_support_frame.hpp','g[0]=word(g[5]+12u);','g[0]=word(g[5]+8u);'),
        ('capture_output_before_saved_BP_store',frame,'physics/runtime/native_tu15_zero_shape_support_frame.hpp','auto&g=s_.gp;saved_bp();g[5]=g[4];at(0x2e8d23u);g[0]=word(g[5]+12u);','auto&g=s_.gp;const auto stale=word(g[4]+8u);saved_bp();g[5]=g[4];at(0x2e8d23u);g[0]=stale;'),
    ]:
        text=(root/path).read_text()
        if text.count(old)!=1:raise ValueError('Zero-support mutation seam drift '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe');built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Zero-support mutant compile failed '+name+': '+built.stderr[-5000:])
        with inputs.open('rb')as inp:run=subprocess.run([str(overlay/'probe')],stdin=inp,capture_output=True,text=True)
        if not run.returncode:raise ValueError('Zero-support mutant accepted '+name)
        result[name]=dict(rejected=True,diagnostic=run.stderr[-5000:])
    return result
