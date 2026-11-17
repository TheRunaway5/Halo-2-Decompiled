"""Owned genuine313560 numerical/continuation/retirement mutation controls."""
import subprocess

def run_mutants(root,scratch,command,inputs):
    kernel='include/halo2/native_tu15_solver_hull_initialize_kernel.hpp'
    frame='src/physics/runtime/native_tu15_solver_hull_initialize_frame.hpp'
    cases=[
        ('omitted_initial_negation',kernel,'halo2/native_tu15_solver_hull_initialize_kernel.hpp','d.direction=view.xor_bits(d.translation,d.sign);','d.direction=d.translation;',False),
        ('inverted_difference_decision',kernel,'halo2/native_tu15_solver_hull_initialize_kernel.hpp','d.produced_plane=!view.difference_above_or_unordered(d);','d.produced_plane=view.difference_above_or_unordered(d);',False),
        ('stale_context_after_genuine_append',frame,'physics/runtime/native_tu15_solver_hull_initialize_frame.hpp','std::uint32_t differences_owner(const Data&){at(0x3135eeu);return s_.gp[6]=word(s_.gp[6]+0x50u);}','std::uint32_t differences_owner(const Data&d){at(0x3135eeu);return s_.gp[6]=word(d.context+0x50u);}',False),
        ('late_FSTP_commit_after_observer',frame,'physics/runtime/native_tu15_solver_hull_initialize_frame.hpp','Base::store(0x31366du,s_.gp[0]+0x20u,pending.bits,[&]{fp_.commit(pending);},true);','Base::store(0x31366du,s_.gp[0]+0x20u,pending.bits,[]{},true);fp_.commit(pending);',True),
    ]
    result={}
    for name,path,relative,old,new,prefixes in cases:
        text=(root/path).read_text()
        if text.count(old)!=1:raise ValueError('Initialization genuine defect seam drift '+name)
        overlay=scratch/name;target=overlay/relative;target.parent.mkdir(parents=True);target.write_text(text.replace(old,new,1))
        mutated=command.copy();mutated.insert(mutated.index('-I'+str(root/'include')),'-I'+str(overlay));mutated[-1]=str(overlay/'initialize')
        built=subprocess.run(mutated,capture_output=True,text=True)
        if built.returncode:raise ValueError('Initialization defect compile failed '+name+': '+built.stderr[-6000:])
        with inputs.open('rb')as inp:run=subprocess.run([str(overlay/'initialize')]+(['--prefixes']if prefixes else[]),stdin=inp,capture_output=True,text=True)
        if not run.returncode or 'Private hull initialize phased reference differs'not in run.stderr:raise ValueError('Initialization defect lacked its genuine expected mismatch '+name+': '+run.stderr[-6000:])
        result[name]=dict(rejected=True,prefix_matrix=prefixes,diagnostic=run.stderr[-6000:])
    return result
