"""Compile bounded parent-mode DATA/ABI/FP/admission defects."""
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def qualify(scratch,fixture,compiler,command):
    raw='src/gameplay/weapons/native_tu15_object_parent_mode.hpp'
    cases=(
        ('wrong-predicate','include/halo2/native_tu15_object_parent_mode_kernel.hpp','(flags&2u)==0u','(flags&2u)!=0u',False),
        ('wrong-row-stride',raw,'s_.gp[1]*=3u','s_.gp[1]*=4u',False),
        ('eax-upper-lost',raw,'s_.set(0,0u,1,0);','s_.gp[0]=0u;',False),
        ('edx-upper-lost',raw,'s_.set(2,value,1,0);','s_.set(2,value,4,0);',False),
        ('unprotected-fp','src/gameplay/weapons/native_tu15_model_marker_lookup.hpp','scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();',False),
        ('missing-live-effect-protocol','src/gameplay/weapons/native_tu15_object_parent_mode_admission.hpp','if(reference()||!native_model_marker_lookup_detail::protocol(services,observe)||','if(reference()||false||',True),
    )
    # Actual-source baseline qualification is separate. These defects exercise
    # the shared numerical/raw/API kernel without recompiling unrelated Effect
    # owners for every mutation.
    base=[part for part in command if part!='-DHALO2_OBJECT_PARENT_SOURCE_COPIES=1'and not part.endswith(('/parent_mode_effect.cpp','/parent_mode_public.cpp','/parent_mode_public_reference.cpp'))]
    results=[]
    for name,path,old,new,guard in cases:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Parent-mode mutation witness drift '+name)
        overlay=scratch/('mutant-'+Path(compiler).name+'-'+name);target=overlay/path;target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'parent';compiled=list(base);compiled[compiled.index('-o')+1]=str(binary);compiled[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')]
        result=subprocess.run(compiled,capture_output=True,text=True)
        if result.returncode:raise ValueError('Parent-mode defect did not compile '+name+': '+result.stderr[-5000:])
        result=subprocess.run([str(binary),'--guards'if guard else str(fixture)],capture_output=True,text=True);diagnostic=(result.stdout+result.stderr)[-3500:]
        recognized=any(text in diagnostic for text in('final ABI differs','complete pre-I/O packets differ','Unbacked parent-mode read','Parent-mode bank extent','Parent-mode refusal','bad_function_call'))
        fp_fault=name=='unprotected-fp'and(result.returncode==-8 or(result.returncode==-6 and'terminate called without an active exception'in diagnostic))
        if result.returncode==0 or not(recognized or fp_fault):raise ValueError('Parent-mode defect survived/unrecognized '+name+': '+diagnostic)
        results.append(dict(name=name,rejected=True,exit_code=result.returncode,diagnostic=diagnostic,scope='Shared DATA/raw/API kernel; actual-source baseline separately qualified'))
    return results
