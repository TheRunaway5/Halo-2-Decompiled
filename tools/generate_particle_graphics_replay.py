#!/usr/bin/env python3
"""Wrap the production particle vertex/quad stages in the original capture ABI."""
from pathlib import Path
import argparse,subprocess,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
HEADER='''#version 450
#extension GL_GOOGLE_include_directive : require
layout(location=0)in vec4 corner;
layout(push_constant) uniform InlineValues{vec4 inlineValue[15];} originalInline;
layout(set=0,binding=0,std140) uniform VshUniforms{
 vec4 c[192];vec4 clipRange;vec2 fogParam;
 vec3 lightInfiniteDirection[8];vec3 lightInfiniteHalfVector[8];
 vec3 lightLocalAttenuation[8];vec3 lightLocalPosition[8];
 vec4 ltc1[20];vec4 ltctxa[26];vec4 ltctxb[52];
 float material_alpha;float pointParams[8];float specularPower;vec2 surfaceSize;
} original;
vec4 particleAttributes[16];
#define particleConstants original.c
#define particleCorner corner
#define particleSurfaceSize original.surfaceSize
#define particleFogParam original.fogParam
#define particleClipRange original.clipRange
'''
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--build',type=Path,default=ROOT/'build-frontend');p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/xemu-capture/particle-graphics-replay.json');a=p.parse_args()
 shader=(ROOT/'shaders/particle.vert').read_text();interface=shader[shader.index('layout(location=0) out'):shader.index('void main()')]
 source=HEADER+interface+'''void main(){
 particleAttributes[0]=vec4(0);
 for(int i=1;i<16;++i)particleAttributes[i]=originalInline.inlineValue[i-1];
 particleProgram();particleVertexOutput();
}\n'''
 path=a.output.parent/'particle-graphics-replay.vert';path.write_text(source);spv=path.with_suffix('.spv')
 subprocess.run(['glslangValidator','-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),'-I'+str(a.build/'shaders'),str(path),'-o',str(spv)],check=True)
 geometry=a.output.parent/'particle-graphics-replay.geom.spv'
 subprocess.run(['glslangValidator','-V','--target-env','vulkan1.3',str(ROOT/'shaders/particle.geom'),'-o',str(geometry)],check=True)
 report={'diagnostic_wrapper_only':True,'native_graphics_pipeline_integrated':False,
 'vertex_spirv':str(spv.relative_to(ROOT)),'geometry_spirv':str(geometry.relative_to(ROOT)),
 'shader_sources_sha256':{str(f.relative_to(ROOT)):hashlib.sha256(f.read_bytes()).hexdigest()for f in(path,ROOT/'shaders/particle.vert',ROOT/'shaders/particle.geom',ROOT/'shaders/particle_vertex_output.glsl',a.build/'shaders/particle_program.glsl')},
 'vertex_spirv_sha256':hashlib.sha256(spv.read_bytes()).hexdigest(),'geometry_spirv_sha256':hashlib.sha256(geometry.read_bytes()).hexdigest()}
 a.output.write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
