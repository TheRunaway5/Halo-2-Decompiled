"""Compile marker policy/ABI/FP defects against unchanged original packets."""
import json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def qualify(scratch,fixture,compiler,command):
    frame='src/gameplay/weapons/native_tu15_model_marker_lookup.hpp'
    cases=(
        ('unequal-name','include/halo2/native_tu15_id_search_kernel.hpp','if(value==view.key())','if(value!=view.key())'),
        ('wrong-row-stride',frame,'s_.flags.plus(g[1],12u,4)','s_.flags.plus(g[1],16u,4)'),
        ('unsigned-count',frame,'std::bit_cast<std::int32_t>(g[6])>0','g[6]!=0u'),
        ('unprotected-fp',frame,'scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();'),
        ('premature-push','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp',
         'store(pc,address,value,[&]{s_.gp[4]=address;});','s_.gp[4]=address;store(pc,address,value,[]{});'))
    results=[]
    for name,path,old,new in cases:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Marker mutation witness drift '+name)
        overlay=scratch/('mutant-'+Path(compiler).name+'-'+name);target=overlay/path
        target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1));binary=overlay/'marker'
        compiled=list(command);compiled[compiled.index('-o')+1]=str(binary)
        compiled[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')]
        done=subprocess.run(compiled,capture_output=True,text=True)
        if done.returncode:raise ValueError('Marker mutation did not compile '+name+': '+done.stderr[-4000:])
        tested=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        diagnostic=(tested.stdout+tested.stderr)[-3000:]
        expected=any(text in diagnostic for text in('final ABI differs','pre-I/O packet','ordered reads differ','Unbacked marker lookup address','Marker lookup crossed bank extent'))
        if tested.returncode==0 or not(expected or(name=='unprotected-fp'and tested.returncode==-8)):
            raise ValueError('Marker semantic mutant survived/unrecognized failure '+name+': '+diagnostic)
        results.append(dict(name=name,rejected=True,exit_code=tested.returncode,diagnostic=diagnostic))
    return results
