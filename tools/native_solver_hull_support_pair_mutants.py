"""Owned semantic/ABI negative controls; only native candidates are mutated."""
import subprocess

def run_mutants(root,scratch,command,inputs):
    kernel='include/halo2/native_tu15_solver_hull_support_pair_kernel.hpp';frame='src/physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp';result={}
    for name,path,relative,old,new in [
        ('stale_first_child_direction',frame,'physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','fp_.child([&]{call_(s_,target,0x3133abu);retired(0x3133abu);});adopt(d);','const auto stale=d.direction;fp_.child([&]{call_(s_,target,0x3133abu);retired(0x3133abu);});adopt(d);d.direction=stale;'),
        ('reversed_dual_nan_MUL_operands',kernel,'halo2/native_tu15_solver_hull_support_pair_kernel.hpp','d.y_term=view.multiply(d.transposed_y,d.negative_y);','d.y_term=view.multiply(d.negative_y,d.transposed_y);'),
        ('omitted_negative_direction',kernel,'halo2/native_tu15_solver_hull_support_pair_kernel.hpp','d.negative=view.xor_bits(d.input,d.sign);','d.negative=d.input;'),
        ('eager_Y_before_Z_product',kernel,'halo2/native_tu15_solver_hull_support_pair_kernel.hpp','d.final_z_term=view.multiply(d.final_z,d.local_z);','d.final_y=view.vector(P::second_y,d,d.context+0x10u);d.final_z_term=view.multiply(d.final_z,d.local_z);'),
        ('eager_second_table',kernel,'halo2/native_tu15_solver_hull_support_pair_kernel.hpp','view.prepare_second(d);d.second_table=view.second_table(d);','d.second_table=view.second_table(d);view.prepare_second(d);'),
        ('duplicate_memCALL_Before',frame,'physics/runtime/native_tu15_solver_hull_support_pair_frame.hpp','Base::store(pc,address,value,[&]{s_.gp[4]=address;},true);','Base::store(pc,address,value,[&]{s_.gp[4]=address;},false);'),
    ]:
        text=(root/path).read_text()
        if text.count(old)!=1:raise ValueError('Support-pair semantic mutation seam drift '+name)
        if name=='eager_Y_before_Z_product':
            later='    d.final_y=view.vector(P::second_y,d,d.context+0x10u);'
            if text.count(later)!=1:raise ValueError('Support-pair Y acquire seam drift')
            text=text.replace(later,'    /* Y was acquired prematurely. */',1)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'probe')
        built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Support-pair mutant compile failed '+name+': '+built.stderr[-5000:])
        with inputs.open('rb')as inp:run=subprocess.run([str(overlay/'probe')],stdin=inp,capture_output=True,text=True)
        if run.returncode==0:raise ValueError('Support-pair semantic mutant accepted '+name)
        result[name]=dict(rejected=True,diagnostic=run.stderr[-7000:])
    return result
