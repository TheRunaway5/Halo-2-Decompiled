"""Compiled effect-reference DATA/ABI defects against untouched CPU outcomes."""
import json,subprocess,sys
from pathlib import Path
from oracle_smg_firing_1_1 import ROOT
from validate_native_tu15_prt_helpers import PIN

def qualify(scratch,rows,io,compiler,command):
    project=scratch/('effect-reference-mutants-'+Path(compiler).name);project.mkdir()
    fixture=project/'fixture.json';fixture.write_text(json.dumps(dict(image_sha256=PIN,io_sites=io,cases=rows[:40]),separators=(',',':')))
    path='src/rendering/effects/native_tu15_effect_reference_element_frame.hpp';source=(ROOT/path).read_text()
    defects=(
        ('unsigned_group_index','g[1]=std::uint32_t(std::int32_t(std::int16_t(g[0]&65535u)));','g[1]=g[0]&65535u;'),
        ('wrong_definition_stride','return g[0]=word(g[0]+g[2]+8u);','return g[0]=word(g[0]/2u+g[2]+8u);'),
        ('wide_returned_index','g[1]=word(g[6]+14u,2);at(0x1780fdu);','g[1]=word(g[6]+14u,4);at(0x1780fdu);'),
        ('conflated_particle_layout','word(s_.gp[1]+(model_?0x7cu:0x88u))','word(s_.gp[1]+0x88u)'),
        ('omit_bound_handle','Base::store(0x22b79au,s_.gp[1]+4u,handle,[]{});',''),
    )
    outcomes=[]
    for name,needle,replacement in defects:
        if source.count(needle)!=1:raise ValueError('Exact effect-reference defect seam drift '+name)
        overlay=project/name;header=overlay/path;header.parent.mkdir(parents=True);header.write_text(source.replace(needle,replacement,1));binary=overlay/name
        cmd=list(command);cmd[cmd.index('-o')+1]=str(binary);cmd[1:1]=['-I'+str(overlay/'src')]
        compiled=subprocess.run(cmd,capture_output=True,text=True);assert compiled.returncode==0,compiled.stderr[-6000:]
        run=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        if run.returncode!=1 or not any(message in run.stderr for message in('full packet differs','ordered reads differ','ordered writes differ','store observer differs','Ray-frame bank extent','Missing ray-frame bank')):
            raise ValueError('Effect-reference real defect was not rejected: '+name+' '+(run.stdout+run.stderr)[-6000:])
        outcomes.append(dict(name=name,rejected=True,diagnostic=run.stderr[-1200:],source_sha256=__import__('hashlib').sha256(header.read_bytes()).hexdigest()))
    return dict(passed=True,compiler=compiler,controls=outcomes,original_text_or_outcomes_modified=False)
