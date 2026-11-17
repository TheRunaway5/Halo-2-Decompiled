"""Compiled inherited-marker DATA, ABI, FP and real-provider defects."""
import subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def qualify(scratch,fixture,compiler,command):
    frame='src/gameplay/weapons/native_tu15_marker_owner.hpp'
    admission='src/gameplay/weapons/native_tu15_marker_owner_admission.hpp'
    cases=(
        ('wrong-inheritance','include/halo2/native_tu15_marker_owner_kernel.hpp','parent!=0xffffffffu&&view.inherits_marker()','parent==0xffffffffu&&view.inherits_marker()',False,False),
        ('wrong-kind-mask',frame,'s_.flags.logic(s_.gp[0]&0x1cu,1)','s_.flags.logic(s_.gp[0]&0x18u,1)',False,False),
        ('swapped-inheritance-pops',frame,'g[7]=pop(0xbad15u);g[6]=pop(0xbad16u);','g[6]=pop(0xbad15u);g[7]=pop(0xbad16u);',False,False),
        ('missing-return-argument-pop',frame,'g[4]+=4u;return g[0];','g[4]+=0u;return g[0];',False,False),
        ('unprotected-host-fp','src/gameplay/weapons/native_tu15_model_marker_lookup.hpp','scope->io(std::forward<Callback>(callback));','std::forward<Callback>(callback)();',False,False),
        ('bypassed-required-owner',admission,'state.services.required_owner(state.memory,{site,target,false},abi,state.observe);','native_weapon_effect_dispatch_tu15_run(state.memory,target,abi,state.services,state.observe);',True,False),
        ('copied-effect-caller-abi',admission,'services.required_owner(memory,{site,target,false},abi,observe);','auto copied=abi;services.required_owner(memory,{site,target,false},copied,observe);abi=copied;',True,False),
        ('wrapped-effect-child-observer',admission,'services.required_owner(memory,{site,target,false},abi,observe);','services.required_owner(memory,{site,target,false},abi,state.observe);',True,False),
        ('missing-live-effect-protocol',admission,'!native_model_marker_lookup_detail::protocol(services,observe)||!abi.floating_point','false||!abi.floating_point',True,True),
        ('missing-required-owner-policy',admission,'!native_required_owner_policy_detail::preserves_instruction_policy(services)','false',True,True),
        ('unchecked-returned-provider-policy',admission,'!native_required_owner_policy_detail::preserves_instruction_policy(s.services)','false',True,'policy'),
        ('missing-generic-provider-fp-fence',admission,'native_required_abi_fp_detail::invoke(state,abi,[&]{\n        state.services.required_owner(state.memory,{site,target,false},abi,state.observe);\n    });','try{state.services.required_owner(state.memory,{site,target,false},abi,state.observe);}\n    catch(...){state.load(abi);throw;}\n    state.load(abi);',True,'fp'),
        ('missing-effect-provider-fp-fence',admission,'native_required_abi_fp_detail::invoke(current,abi,[&]{\n            services.required_owner(memory,{site,target,false},abi,observe);\n        });','try{services.required_owner(memory,{site,target,false},abi,observe);}\n        catch(...){current.load(abi);throw;}\n        current.load(abi);',True,'fp'),
    )
    results=[]
    for name,path,old,new,source,guard in cases:
        original=(ROOT/path).read_text()
        if original.count(old)!=1:raise ValueError('Missing inherited-marker defect witness '+name)
        overlay=scratch/('mutant-'+Path(compiler).name+'-'+name);target=overlay/path;target.parent.mkdir(parents=True);target.write_text(original.replace(old,new,1))
        binary=overlay/'marker';compiled=list(command);compiled[compiled.index('-o')+1]=str(binary)
        if not source:
            compiled=[part for part in compiled if not part.endswith('/marker_owner_effect.cpp')]
            compiled=[str(ROOT/'tests/native_tu15_marker_owner_tests.cpp')if part.endswith('/native_tu15_marker_owner_source_tests.cpp')else part for part in compiled]
        compiled[1:1]=['-I'+str(overlay/'include'),'-I'+str(overlay/'src')]
        done=subprocess.run(compiled,capture_output=True,text=True)
        if done.returncode:raise ValueError('Inherited-marker defect did not compile '+name+': '+done.stderr[-6000:])
        arguments=['--provider-policy-mutations',str(fixture)]if guard=='policy'else['--hostile-required',str(fixture)]if guard=='fp'else['--guards']if guard else[str(fixture)]
        done=subprocess.run([str(binary),*arguments],capture_output=True,text=True)
        diagnostic=(done.stdout+done.stderr)[-7000:]
        known=any(text in diagnostic for text in('Marker-owner final ABI differs','Marker-owner ordered reads differ','Marker-owner ordered stores differ','Marker-owner complete pre-I/O packets differ','Marker-owner typed value differs','Inherited-marker source final ABI differs','Inherited-marker source access protocol differs','Inherited-marker source bypassed/repeated actual required owner','FC510 bypassed its actual required-owner boundary','Effect child lost caller ABI reference identity','Inherited-marker required-owner observer identity differs','Uncertified provider did not retain whole source parent','bad_function_call','Unbacked parent-mode read'))
        fp=name=='unprotected-host-fp'and(done.returncode==-8 or(done.returncode==-6 and'terminate called without an active exception'in diagnostic))
        if guard=='fp':fp=done.returncode==-8 or('FPE'in diagnostic and'State::load'in diagnostic)
        if done.returncode==0 or not(known or fp):raise ValueError('Inherited-marker defect survived/unrecognized '+name+': '+diagnostic)
        results.append(dict(name=name,rejected=True,exit_code=done.returncode,diagnostic=diagnostic,actual_source_transport=source))
    return results
