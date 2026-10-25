#!/usr/bin/env python3
"""Compile the exact native observation layout before an installed-gameplay probe."""
import hashlib,json,os,shlex,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix='audio-observation-layout-',dir=owner) as temporary:
        scratch=Path(temporary);source=scratch/'layout.cpp';exe=scratch/'layout'
        source.write_text('''#include "halo2/sdl_sound_platform.hpp"
#include "halo2/simulation.hpp"
#include <cstddef>
#include <cstring>
#include <iostream>
int main(){using T=halo2::SdlSoundPlaybackPosition;using P=halo2::SdlSoundVoiceParameters;using R=halo2::SdlSoundRoute;
P parameters{};parameters.routes={{0,0,1.f},{1,1,.5f}};std::array<const R*,3> pointers{};
static_assert(sizeof(parameters.routes)==sizeof(pointers));std::memcpy(pointers.data(),&parameters.routes,sizeof(pointers));
if(pointers[0]!=parameters.routes.data() || pointers[1]!=parameters.routes.data()+parameters.routes.size() ||
 pointers[2]!=parameters.routes.data()+parameters.routes.capacity())return 1;
std::cout<<"{\\\"bytes\\\":"<<sizeof(T)<<",\\\"source_frames\\\":"<<offsetof(T,source_frames)
<<",\\\"output_frames\\\":"<<offsetof(T,output_frames)
<<",\\\"controls_crouch\\\":"<<offsetof(halo2::Controls,crouch)
<<",\\\"controls_secondary\\\":"<<offsetof(halo2::Controls,fire_secondary)
<<",\\\"parameters_volume\\\":"<<offsetof(P,volume_hundredths_db)<<",\\\"parameters_paused\\\":"<<offsetof(P,paused)
<<",\\\"parameters_routes\\\":"<<offsetof(P,routes)<<",\\\"route_stride\\\":"<<sizeof(R)
<<",\\\"route_gain\\\":"<<offsetof(R,linear_gain)<<",\\\"vector_begin\\\":0,\\\"vector_end\\\":"<<sizeof(void*)<<"}\\n";}
''')
        includes=shlex.split(subprocess.check_output(['pkg-config','--cflags','sdl2'],text=True))
        subprocess.run(['c++','-std=c++20',*includes,'-I'+str(ROOT/'include'),str(source),'-o',str(exe)],check=True)
        result=json.loads(subprocess.check_output([str(exe)],text=True))
        header=ROOT/'include/halo2/sdl_sound_platform.hpp'
        result['header_sha256']=hashlib.sha256(header.read_bytes()).hexdigest()
        result['simulation_header_sha256']=hashlib.sha256((ROOT/'include/halo2/simulation.hpp').read_bytes()).hexdigest()
        result['compiler']=subprocess.check_output(['c++','--version'],text=True).splitlines()[0]
        out=ROOT/'analysis/combat-presentation-audit-20261022/repair-installed-banshee/native-audio-observation-layout.json'
        out.parent.mkdir(parents=True,exist_ok=True);out.write_text(json.dumps(result,indent=2)+'\n')
        print(json.dumps(result),flush=True)
    assert not scratch.exists()
if __name__=='__main__':main()
