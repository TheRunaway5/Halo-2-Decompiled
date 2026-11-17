"""Compiled defects in the actual private313470 source transport owners."""
import subprocess

def run_mutants(root,scratch,command,path,env):
    cases=[
        ('raw_callback_admission','physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','if constexpr(!Literal)if(s.services.instruction)return false;','if constexpr(false)if(s.services.instruction)return false;','baseline','Hull-edge complete source route differs'),
        ('physical_PUSH_publication','physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','Publication::retained','Publication::physical','source-publication','Hull-edge complete source route differs'),
        ('omitted_policy_retirement','physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','if(!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};','if(false&&!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};','frontiers','Hull-edge complete source route differs'),
        ('stale_first_math_XMM','physics/runtime/native_tu15_solver_hull_edge_frame.hpp','d.position=s_.xmm[1];d.first=s_.xmm[3];d.first_transformed=s_.xmm[2];','/* incorrectly cached pre-provider operands */','feedback','Hull-edge complete source route differs'),
        ('omitted_empty_observer_entry_branch','physics/runtime/native_tu15_solver_hull_edge_frame.hpp','if(!s_.observe){fp_.io([&]{s_.memory.write(address,bytes);});return;}','/* incorrectly rechecked observer after an unobserved write */','observer-objects','Hull-edge complete source route differs'),
        ('omitted_true_child_retirement','physics/runtime/native_tu15_solver_hull_edge_frame.hpp','call_(s_,0x313390u,pc);retired(pc);','call_(s_,0x313390u,pc);','frontiers','Hull-edge complete source route differs'),
        ('omitted_unaligned_source_handoff','physics/runtime/native_tu15_solver_hull_edge_source_owner.hpp','return !(address&15u);','return true;','feedback','AlignedLoadFault'),
    ]
    result={}
    for name,relative,old,new,mode,diagnostic in cases:
        header=(root/'src'/relative).read_text()
        if header.count(old)!=1:raise ValueError('Hull-edge source defect seam changed '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(old,new,1))
        mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'routes')
        built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Source edge defect compile failed '+name+': '+built.stderr[-8000:])
        with path.open('rb')as inp:run=subprocess.run([str(overlay/'routes'),mode],stdin=inp,capture_output=True,text=True,env=env)
        if not run.returncode or diagnostic not in run.stderr:raise ValueError('Source edge defect lacked its genuine expected mismatch '+name+': '+run.stderr[-8000:])
        result[name]=dict(rejected=True,mode=mode,exit_code=run.returncode,diagnostic=run.stderr[-8000:])
    return result
