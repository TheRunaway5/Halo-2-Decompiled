"""Genuine compiled edge-policy and raw ABI defects; only native copies mutate."""
import subprocess
def run_mutants(root,scratch,command,inputs):
    kernel='include/halo2/native_tu15_solver_hull_edge_kernel.hpp';frame='src/physics/runtime/native_tu15_solver_hull_edge_frame.hpp';result={}
    for name,path,relative,old,new in [
        ('eager_first_sum_before_current_point',kernel,'halo2/native_tu15_solver_hull_edge_kernel.hpp','d.second_transformed=view.vector(P::second_transformed,d);\n    d.first_sum=view.add_scalar(d.first_z,d.first_xy);','d.first_sum=view.add_scalar(d.first_z,d.first_xy);\n    d.second_transformed=view.vector(P::second_transformed,d);'),
        ('wrong_upper_lanes_at_first_xy',kernel,'halo2/native_tu15_solver_hull_edge_kernel.hpp','d.first_xy=view.add_scalar(d.first_y,d.first_product);','d.first_xy=view.add_scalar(d.first_product,d.first_y);'),
        ('omitted_fresh_second_score_read',frame,'physics/runtime/native_tu15_solver_hull_edge_frame.hpp','bool select_first(const Data&){\n        at(0x313525u);const auto bits=word(s_.gp[4]+0x1cu);','bool select_first(const Data&d){\n        at(0x313525u);const auto bits=low(d.second_sum);'),
        ('late_second_exit_EDI_pop',frame,'physics/runtime/native_tu15_solver_hull_edge_frame.hpp','s_.gp[7]=pop(0x313554u);retired(0x313554u);store(0x313555u,s_.gp[6]+0x30u,s_.gp[0]);','store(0x313555u,s_.gp[6]+0x30u,s_.gp[0]);s_.gp[7]=pop(0x313554u);retired(0x313554u);'),
        ('restored_entry_EAX_after_actual_child',frame,'physics/runtime/native_tu15_solver_hull_edge_frame.hpp','fp_.child([&]{call_(s_,0x313390u,pc);retired(pc);});','const auto stale=s_.gp[0];fp_.child([&]{call_(s_,0x313390u,pc);retired(pc);});s_.gp[0]=stale;'),
        ('FNSTSW_overwrites_upper_EAX',frame,'physics/runtime/native_tu15_solver_hull_edge_frame.hpp','s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;','s_.gp[0]=status;'),
    ]:
        text=(root/path).read_text()
        if text.count(old)!=1:raise ValueError('Hull-edge compiled defect seam drift '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1));mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'edge')
        built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Hull-edge defect compile failed '+name+': '+built.stderr[-6000:])
        with inputs.open('rb')as inp:run=subprocess.run([str(overlay/'edge')],stdin=inp,capture_output=True,text=True)
        if not run.returncode:raise ValueError('Hull-edge compiled defect survived '+name)
        result[name]=dict(rejected=True,diagnostic=run.stderr[-6000:])
    return result
