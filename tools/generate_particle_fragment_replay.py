#!/usr/bin/env python3
"""Generate bounded replay wrappers around the native particle fragment math.

The original textures, interpolation, geometry stage and framebuffer state
remain in the captured draw. Only fragment calculations are replaced. The
native material record supplies disc program words; no original shader's output
or frame pixels are copied into the implementation.
"""
import argparse,json,struct,hashlib,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HEADER='''#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_demote_to_helper_invocation : require
layout(set=0,binding=1,std140) uniform PshUniforms {
 int alphaRef;mat2 bumpMat[4];float bumpOffset[4];float bumpScale[4];
 vec4 clipRange;ivec4 clipRegion[8];uint colorKey[4];uint colorKeyMask[4];
 vec4 consts[18];float depthFactor;float depthOffset;vec4 fogColor;
 ivec2 surfaceScale;float texScale[4];
} original;
layout(location=0)in vec4 vtxD0;
layout(location=1)in vec4 vtxD1;
layout(location=9)flat in vec4 vtxPos0;
layout(location=10)flat in vec4 vtxPos1;
layout(location=11)flat in vec4 vtxPos2;
layout(location=12)flat in float triMZ;
layout(location=5)in vec4 vtxT0;
layout(location=6)in vec4 vtxT1;
layout(set=0,binding=2)uniform sampler2D texSamp0;
layout(set=0,binding=3)uniform sampler2D texSamp1;
layout(location=0)out vec4 fragColor;
'''
MAIN='''
#include "combiner.glsl"
#include "particle_depth.glsl"
void main(){
 bool contained=false;vec2 coord=gl_FragCoord.xy-vec2(0.5);
 for(int i=0;i<8;++i)if(!any(bvec4(lessThan(coord,original.clipRegion[i].xy),greaterThanEqual(coord,original.clipRegion[i].zw)))){contained=true;break;}
 if(!contained)demote;
 float depth=particleWDepth(gl_FragCoord.xy/vec2(original.surfaceScale),vtxPos0,vtxPos1,vtxPos2,triMZ,original.depthFactor,original.depthOffset);
 if(depth<original.clipRange.z || original.clipRange.w<depth)demote;
 vec4 sampled[4];sampled[0]=textureProj(texSamp0,vtxT0.xyw);
 sampled[1]=textureProj(texSamp1,vtxT1.xyw);
 sampled[2]=sampled[3]=vec4(0,0,0,1);
 fragColor=runCombiners(sampled,vtxD0,vtxD1);
 gl_FragDepth=particleD24Depth(depth);
}
'''
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--materials',type=Path,default=ROOT/'analysis/frontend/title-particle-material.native.bin')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/xemu-capture/particle-fragment-replay.json')
 a=p.parse_args();data=a.materials.read_bytes();pos=0;programs=[]
 for n in range(6):
  size=struct.unpack_from('<I',data,pos)[0];pos+=4;programs.append(data[pos:pos+240]);pos+=size
 assert pos==len(data) and programs[2]==programs[4]
 rows=[]
 for kind,index in (('tint',0),('plain',2)):
  words=struct.unpack('<60I',programs[index]);path=a.output.parent/f'particle-fragment-{kind}.frag'
  vectors=','.join('uvec4('+','.join(str(w)+'u'for w in words[n:n+4])+')'for n in range(0,60,4))
  code=HEADER+'struct NativeMaterial{uvec4 program[15];};\nconst NativeMaterial material=NativeMaterial(uvec4[15]('+vectors+'));\n'+MAIN
  path.write_text(code);spv=path.with_suffix('.spv')
  subprocess.run(['glslangValidator','-V','--target-env','vulkan1.3','-I'+str(ROOT/'shaders'),str(path),'-o',str(spv)],check=True)
  rows.append({'kind':kind,'source':str(path.relative_to(ROOT)),'spirv':str(spv.relative_to(ROOT)),
   'source_sha256':hashlib.sha256(code.encode()).hexdigest(),'spirv_sha256':hashlib.sha256(spv.read_bytes()).hexdigest(),
   'program_sha256':hashlib.sha256(programs[index]).hexdigest()})
 a.output.write_text(json.dumps({'diagnostic_only':True,'native_graphics_pipeline_integrated':False,'programs':rows},indent=2)+'\n')
if __name__=='__main__':main()
