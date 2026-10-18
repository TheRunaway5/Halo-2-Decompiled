#!/usr/bin/env python3
"""Prepare analysis-only original-emitter projected-light vertex fixtures.

Literal cache programs/declarations and retained Xemu decoder/helper arithmetic
are compared through the same measured raw-fetch/UBO/output wrapper. GPL shader
text is confined to analysis outputs. This is not an Xbox framebuffer or a live
owner/admission proof. It runs only when explicitly maintaining local fixtures.
"""
import argparse,hashlib,importlib.util,json,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def sha(data):return hashlib.sha256(data).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--catalog',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 source=ROOT/'analysis/frontend/title-camera-matrix-audit/generate_world_vertex_straight_line.py';original=ROOT/'analysis/rendering-reference/vsh-prog.c';pinned=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/glsl/vsh-prog.c';assert original.read_bytes()==pinned.read_bytes()
 spec=importlib.util.spec_from_file_location('retained_decoder',source);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
 class Decoder(m.Decoder):
  def operand(self,token,letter):
   if self.field(token,'FLD_'+letter+'_MUX')==3 and self.field(token,'FLD_A0X'):
    return ('-'if self.field(token,'FLD_'+letter+'_NEG')else'')+'c[A0+'+str(self.field(token,'FLD_CONST'))+']'+self.swizzle(token,letter)
   return super().operand(token,letter)
  def opcode(self,token,output_mux,mask,opcode,inputs):
   if opcode==self.mac[13]:
    assert output_mux==0 and (not self.field(token,'FLD_OUT_O_MASK') or self.field(token,'FLD_OUT_MUX')==1)
    if self.field(token,'FLD_ILU'):return '  ARL(_temp_addr'+inputs+');\n','  A0 = _temp_addr;\n',['_ARL']
    return '  ARL(A0'+inputs+');\n','',['_ARL']
   return super().opcode(token,output_mux,mask,opcode,inputs)
 d=Decoder(original.read_text());helpers=ROOT/'analysis/frontend/title-camera-matrix-audit/world-vertex-straight-line-v2/source-original-helpers.glsl';manifest=json.loads(a.catalog.read_text());rows=[]
 for e in manifest['entries']:
  if not any('\u005clightaccum_frustum' in alias['name']for alias in e['aliases']):continue
  code=(a.catalog.parent/e['program']).read_bytes();native=(a.catalog.parent/e['source']).read_text();semantics=e['semantics'];prefix=native[:native.index('vec4 h2MenuMultiply(')];fetch=native[native.index('float sourceClampW('):native.index('void main() {')];epilogue=native[native.index('    vec4 screen=vpOut[0];'):]
  packed=[i for i,s in enumerate(semantics)if s in(4,5,6)];external=[i for i in e['consumed_inputs']if i>=len(semantics)];declarations=''.join(f'vec4 v{i};\n'for i in packed+external);setup=''.join(f'    v{i}=sourcePackedNormal(v{i}_cmp);\n'for i in packed)+''.join(f'    v{i}=source.attributes[{i}];\n'for i in external)
  body='';pairs=[]
  for slot,token in enumerate(struct.iter_unpack('<4I',code[4:])):
   text,_=d.token(token);body+=f'    /* original slot {slot} */\n'+text+'\n'
   mac,ilu=(token[1]>>21)&15,(token[1]>>25)&7
   if mac and ilu:pairs.append([mac,ilu])
  copies=''.join(f'    vpOut[{i}]={name};\n'for i,name in[(0,'oPos'),(3,'oD0'),(4,'oD1'),(5,'oFog'),(6,'oPts'),(7,'oB0'),(8,'oB1'),(9,'oT0'),(10,'oT1'),(11,'oT2'),(12,'oT3')])
  identity=sha(code);stem='class'+str(e['class_index'])+'-'+identity[:16];path=a.output/(stem+'.vert');path.write_text(prefix+helpers.read_text()+declarations+fetch+'#define c source.c\nvoid main() {\n'+setup+body+copies+epilogue);spv=path.with_suffix('.spv');q=subprocess.run(['glslangValidator','-V','--target-env','vulkan1.3',path,'-o',spv],capture_output=True,text=True);path.with_suffix('.log').write_text(q.stdout+q.stderr);assert q.returncode==0,q.stdout+q.stderr;subprocess.run(['spirv-val','--target-env','vulkan1.3',spv],check=True)
  rows.append(dict(class_index=e['class_index'],program_sha256=identity,semantics=semantics,spirv=str(spv.resolve()),source_sha256=sha(path.read_bytes()),spirv_sha256=sha(spv.read_bytes()),pairs=pairs))
 (a.output/'report.json').write_text(json.dumps(dict(scope=__doc__,decoder_sha256=sha(original.read_bytes()),helper_sha256=sha(helpers.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),catalog_sha256=sha(a.catalog.read_bytes()),GPU_verified=False,programs=rows),indent=2)+'\n');print('PASS',len(rows),'compiled original-emitter projected programs; GPU comparison pending')
if __name__=='__main__':main()
