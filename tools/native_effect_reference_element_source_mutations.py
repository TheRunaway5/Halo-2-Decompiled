"""Compiled source/default/provider defects against genuine owned test routes."""
import hashlib,json,subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

def qualify(scratch,rows,io,compiler,command):
    project=scratch/('effect-source-mutants-'+Path(compiler).name);project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows[:10]),separators=(',',':')))
    source=(scratch/'effect_reference_source_bodies.inc').read_text()
    admission_path='src/rendering/effects/native_tu15_effect_reference_element_admission.hpp'
    admission=(ROOT/admission_path).read_text();test=(ROOT/'tests/native_tu15_effect_reference_element_source_tests.cpp').read_text()
    defects=(
        ('disabled_source_prefix','source','if(halo2::native_effect_reference_element_detail::bridge(',
         'if(false&&halo2::native_effect_reference_element_detail::bridge(',False,False),
        ('unchecked_provider','admission','!native_required_owner_policy_detail::preserves_instruction_policy(s.services)','false',True,False),
        ('ignored_reference','admission','if(reference()||','if(false||',False,True),
        ('bypassed_required_owner','source','dispatch(child,target,site,false);','::effect_reference_bypass(child,target,site);',False,False),
    )
    outcomes=[]
    for name,kind,needle,replacement,guards,reference in defects:
        overlay=project/name;overlay.mkdir();text=source if kind=='source'else admission
        if needle not in text or(kind=='admission'and text.count(needle)!=1):raise ValueError('Effect source defect seam drift '+name)
        changed=text.replace(needle,replacement)
        if kind=='source':
            (overlay/'effect_reference_source_bodies.inc').write_text(changed)
            (overlay/'effect_reference_source_includes.inc').write_text((scratch/'effect_reference_source_includes.inc').read_text())
        else:
            header=overlay/admission_path;header.parent.mkdir(parents=True);header.write_text(changed)
        candidate=test
        if name=='bypassed_required_owner':
            marker='#include "effect_reference_source_bodies.inc"'
            helper='''template<class State>void effect_reference_bypass(State&s,std::uint32_t target,std::uint32_t){
 auto before=[](auto&,auto){};auto forbidden=[](auto&,auto,auto){throw std::runtime_error("Unexpected source mutant child");};
 halo2::native_effect_reference_element_detail::run_private(s,target,before,forbidden);
}
'''
            if candidate.count(marker)!=1:raise ValueError('Source mutant test include seam drift')
            candidate=candidate.replace(marker,helper+marker,1)
        cpp=overlay/'source.cpp';cpp.write_text(candidate);binary=overlay/name;dep=overlay/'source.d'
        cmd=list(command);cmd[cmd.index(str(ROOT/'tests/native_tu15_effect_reference_element_source_tests.cpp'))]=str(cpp)
        cmd[cmd.index('-o')+1]=str(binary);cmd[cmd.index('-MF')+1]=str(dep)
        cmd[1:1]=['-I'+str(overlay),'-I'+str(overlay/'src'),'-I'+str(ROOT/'tests')]
        result=subprocess.run(cmd,capture_output=True,text=True);assert result.returncode==0,result.stderr[-7000:]
        import os
        env=dict(os.environ,**({'HALO2_REFERENCE_EFFECT_REFERENCE_ELEMENT':'1'}if reference else{}))
        if name=='disabled_source_prefix':env['HALO2_EFFECT_REFERENCE_FAIL_ON_RETAINED']='1'
        run=subprocess.run([str(binary),*(['--guards']if guards else[str(fixture)])],capture_output=True,text=True,env=env)
        messages=('Uncertified owner admitted','native source ran retained statements','source full packets differ',
            'positive reference did not execute','genuine required-owner entry count differs')
        if run.returncode!=1 or not any(message in run.stderr for message in messages):
            raise ValueError('Compiled effect source defect was not rejected '+name+' '+(run.stdout+run.stderr)[-5000:])
        outcomes.append(dict(name=name,rejected=True,diagnostic=run.stderr[-1000:],changed_source_sha256=hashlib.sha256(changed.encode()).hexdigest()))
    return dict(passed=True,compiler=compiler,controls=outcomes,original_instructions_or_outcomes_changed=False)
