"""Compiled defects in actual private313560 guards and retired transport."""
import subprocess
def run_mutants(root,scratch,command,path,env):
    cases=[
      ('raw_callback_admission','owner','if constexpr(!Literal)if(s.services.instruction)return false;','if constexpr(false)if(s.services.instruction)return false;','baseline'),
      ('unknown_provider_admission','owner','if(!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','if(false&&!native_required_owner_policy_detail::preserves_instruction_policy(s.services))return false;','provider-controls'),
      ('omitted_policy_retirement','owner','if(!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};','if(false&&!continuation_admitted<Literal>(v,next))throw Continuation{next,&token};','frontiers'),
      ('omitted_actual_child_retirement','frame','call_(s_,target,pc);retired(pc);','call_(s_,target,pc);','child-return'),
      ('stale_direction_after_store','frame','d.direction=s_.xmm[0];','/* incorrectly retained old direction */','feedback'),
      ('wrong_source_C1_spill','frame','const auto bits=spill_(s_);','const auto bits=LegacySingleSpill{}(s_);','feedback'),
      ('physical_PUSH_publication','frame','at(pc);s_.push(pc,value);retired(pc);','at(pc);const auto address=s_.gp[4]-4u;physics_step_detail::w(s_.memory,s_.observe,pc,address,4u,value);s_.gp[4]=address;retired(pc);','source-publication'),
      ('swallowed_foreign_continuation','owner','if(resume.owner!=&token)throw;','if(false&&resume.owner!=&token)throw;','foreign-exceptions'),
    ];result={}
    for name,kind,old,new,mode in cases:
        relative='physics/runtime/native_tu15_solver_hull_initialize_source_'+kind+'.hpp';header=(root/'src'/relative).read_text()
        if header.count(old)!=1:raise ValueError('Initialization source defect seam drift '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(header.replace(old,new,1))
        mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'routes')
        built=subprocess.run(mutated,capture_output=True,text=True,timeout=600)
        if built.returncode:raise ValueError('Initialization source defect failed compilation '+name+': '+built.stderr[-5000:])
        with path.open('rb')as stream:done=subprocess.run([str(overlay/'routes'),mode],stdin=stream,capture_output=True,text=True,env=env,timeout=1200)
        expected=('Uncertified provider did not take whole-entry fallback'if name=='unknown_provider_admission'else
                  'Unsupported native hull-initialize continuation'if name=='swallowed_foreign_continuation'else
                  'Hull-initialize complete source route differs')
        if not done.returncode or expected not in done.stderr:raise ValueError('Initialization source defect lacked authentic comparison/guard failure '+name+': '+done.stderr[-5000:])
        result[name]=dict(rejected=True,mode=mode,expected_failure=expected,diagnostic=done.stderr[-5000:])
    return result
