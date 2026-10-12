#!/usr/bin/env python3
"""Compile original-emitter sphere VS fixtures for isolated GPU comparisons.

The independently retained Xemu arithmetic and token text replace only the
native arithmetic body. The already measured raw-input fetch/compact UBO and
screen epilogue are shared comparison controls. This is a shader test fixture,
not an original gameplay framebuffer, residency or complete draw-owner proof.
Original GPL shader text stays in analysis artifacts, outside production.
"""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def sha(raw):return hashlib.sha256(raw).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bindings',type=Path,required=True);p.add_argument('--catalog',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 report=json.loads((a.bindings/'report.json').read_text());catalog=json.loads(a.catalog.read_text());helpers=ROOT/'analysis/frontend/title-camera-matrix-audit/world-vertex-straight-line-v2/source-original-helpers.glsl';helper_raw=helpers.read_bytes()
 assert sha((ROOT/'analysis/rendering-reference/vsh-prog.c').read_bytes())==report['original_decoder_sha256']
 compiled=[]
 for key,program in report['programs'].items():
  code=(a.bindings/(key+'.tokens.bin')).read_bytes();decl=(a.bindings/(key+'.declaration.bin')).read_bytes();semantic=(a.bindings/(key+'.semantic.glsl')).read_bytes()
  for name,raw in [(key+'.tokens.bin',code),(key+'.declaration.bin',decl),(key+'.semantic.glsl',semantic)]:assert sha(raw)==report['files'][name]['sha256']
  semantics=list(struct.unpack('<'+'H'*(len(decl)//2),decl));entries=[r for r in catalog['entries']if r['class_index']==program['classification'] and r['semantics']==semantics and (a.catalog.parent/r['program']).read_bytes()==code];assert len(entries)==1
  native=(a.catalog.parent/entries[0]['source']).read_text();prefix=native[:native.index('vec4 h2MenuMultiply(')];fetch=native[native.index('float sourceClampW('):native.index('void main() {')];epilogue=native[native.index('    vec4 screen=vpOut[0];'):]
  missing=''.join(f'vec4 v{i};\n'for i,s in enumerate(semantics)if s in (4,5,6));setup=''.join(f'    v{i}=sourcePackedNormal(v{i}_cmp);\n'for i,s in enumerate(semantics)if s in (4,5,6))
  copies=''.join(f'    vpOut[{i}]={name};\n'for i,name in [(0,'oPos'),(3,'oD0'),(4,'oD1'),(5,'oFog'),(6,'oPts'),(7,'oB0'),(8,'oB1'),(9,'oT0'),(10,'oT1'),(11,'oT2'),(12,'oT3')])
  text=prefix+'// Analysis-only original-emitter arithmetic comparison.\n'+helper_raw.decode()+missing+fetch+'#define c source.c\nvoid main() {\n'+setup+semantic.decode()+copies+epilogue
  source=a.output/(key+'.vert');source.write_text(text);output=a.output/(key+'.spv');command=['glslangValidator','-V','--target-env','vulkan1.3','-S','vert',str(source),'-o',str(output)]
  run=subprocess.run(command,capture_output=True,text=True);(a.output/(key+'.compile.log')).write_text(run.stdout+run.stderr);assert run.returncode==0,run.stdout+run.stderr
  validate=subprocess.run(['spirv-val','--target-env','vulkan1.3',str(output)],capture_output=True,text=True);assert validate.returncode==0,validate.stdout+validate.stderr
  compiled.append(dict(key=key,program_sha256=sha(code),source_sha256=sha(source.read_bytes()),spirv_sha256=sha(output.read_bytes()),native_wrapper_sha256=sha(native.encode()),command=command))
 result=dict(boundary=__doc__,tool_sha256=sha(Path(__file__).read_bytes()),bindings_report_sha256=sha((a.bindings/'report.json').read_bytes()),original_helper_sha256=sha(helper_raw),catalog_sha256=sha(a.catalog.read_bytes()),compiled=compiled,GPU_verified=False)
 (a.output/'report.json').write_text(json.dumps(result,indent=2)+'\n');(a.output/'tool.py').write_bytes(Path(__file__).read_bytes());print('PASS',len(compiled),'compiled original-emitter sphere fixtures; GPU comparison pending')
if __name__=='__main__':main()
