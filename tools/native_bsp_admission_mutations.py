"""Compiled semantic controls for the actual BSP DATA/ABI and live admission."""
import json,os,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
DEFINITIONS=(
 ('ignore-cluster-auxiliary','include/halo2/native_tu15_bsp_admission_kernel.hpp','if(!view.cluster_allowed())continue;','if(false&&!view.cluster_allowed())continue;'),
 ('require-force-for-visible','include/halo2/native_tu15_bsp_admission_kernel.hpp','if(view.instance_visible()||view.force_instance())','if(view.instance_visible()&&view.force_instance())'),
 ('wide-instance-flags','src/rendering/native_tu15_bsp_admission.hpp','write(0x1372f5u,g[7]+g[2]*2u,g[6],2);','write(0x1372f5u,g[7]+g[2]*2u,g[6],4);'),
 ('discard-last-probed-cursor','src/rendering/native_tu15_bsp_admission.hpp','g[2]=read(0x137357u,g[1]+8u);','g[7]=0xffffffffu;g[2]=read(0x137357u,g[1]+8u);'),
 ('premature-rmw-flags','src/rendering/native_tu15_bsp_admission.hpp','io([&]{s_.memory.write(address,std::span(bytes).first(width));});s_.flags=flags;','s_.flags=flags;io([&]{s_.memory.write(address,std::span(bytes).first(width));});'),
 ('wrong-cluster-extended-radius','src/rendering/native_tu15_bsp_admission.hpp','faddp %%st,%%st(1);fsqrt','faddp %%st,%%st(1);fabs'),
 ('direct-page-offset','src/physics/runtime/native_tu15_raw_x87_scope.hpp','std::memcpy(output.data(),page->bytes+(address&4095u),output.size());return true;','std::memcpy(output.data(),page->bytes+((address+1u)&4095u),output.size());return true;'),
 ('direct-page-unfilled','src/physics/runtime/native_tu15_raw_x87_scope.hpp','std::memcpy(output.data(),page->bytes+(address&4095u),output.size());return true;','return true;'),
 ('view-read-short','src/physics/runtime/native_tu15_raw_x87_scope.hpp','std::memcpy(output.data(),view->data(),output.size());return true;','std::memcpy(output.data(),view->data(),output.size()-1u);return true;'),
 ('silent-hook-stale-cursor','src/physics/runtime/native_tu15_mopp_ray_caller_frame.hpp','        if(s_.instruction_cursor&&s_.instruction_cursor->position)\n            *s_.instruction_cursor->position=pc;','        if(!native_raw_x87_detail::skip_hook<Before>()&&s_.instruction_cursor&&s_.instruction_cursor->position)\n            *s_.instruction_cursor->position=pc;'),
 ('io-fast-path-keeps-provider-fp','src/physics/runtime/native_tu15_raw_x87_scope.hpp','if(std::memcmp(after.data(),area.data(),6)||std::memcmp(after.data()+24,area.data()+24,4)) {','if(false) {'),
 ('cleared-callback-admission','src/rendering/native_tu15_bsp_admission.hpp','else if(s.instruction_cursor||s.literal_service_sites||s.observe)return false;','else if(s.observe)return false;'),
)
def check(scratch,command,rows,metadata):
    outcomes=[]
    for name,path,old,new in DEFINITIONS:
        source=(ROOT/path).read_text()
        if source.count(old)!=1:raise ValueError('Unique BSP compiled mutant witness missing '+name)
        overlay=scratch/('mutant-'+name);target=overlay/Path(path).relative_to(Path(path).parts[0]);target.parent.mkdir(parents=True);target.write_text(source.replace(old,new,1))
        binary=overlay/'tests';mutated=list(command);mutated[mutated.index('-o')+1]=str(binary);mutated.insert(1,'-I'+str(overlay))
        if name!='cleared-callback-admission':mutated.insert(1,'-DHALO2_BSP_MUTATION_CORE_ONLY=1')
        compiled=subprocess.run(mutated,capture_output=True,text=True)
        if compiled.returncode:raise ValueError('BSP semantic mutant did not compile '+name+': '+compiled.stderr[-4000:])
        if name=='cleared-callback-admission':tests=['--guards']
        else:
            selected=[row for row in rows if row.get('provider_rmw_read')]if name=='premature-rmw-flags'else[row for row in rows if'fault'not in row]
            if not selected:raise ValueError('BSP semantic mutant has no genuine fixture '+name)
            tests=[]
            for index in range(0,len(selected),64):
                fixture=overlay/(str(index//64)+'.json');fixture.write_text(json.dumps(dict(metadata,cases=selected[index:index+64]),separators=(',',':')));tests.append(str(fixture))
        diagnostic='';rejected=False
        for test in tests:
            done=subprocess.run([str(binary),test],capture_output=True,text=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
            if done.returncode:diagnostic=(done.stdout+done.stderr)[-2500:];rejected=True;break
        if not rejected:raise ValueError('Compiled BSP semantic mutant survived '+name)
        outcomes.append(dict(name=name,compiled=True,rejected=True,diagnostic=diagnostic))
    return outcomes
