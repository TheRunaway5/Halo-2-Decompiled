#!/usr/bin/env python3
"""Compile analysis-only original-emitter authored shadow vertex programs.

Xbox cache tokens and declarations identify each variant. The pinned original
Xemu decoder/helpers supply independent arithmetic. Native fetch, compact UBO
and output wrapper remain shared controls. These are isolated shader fixtures,
not matched gameplay frames or physical Xbox precision evidence. GPL original
shader text is retained only in the analysis output, never production source.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',type=Path,required=True)
    parser.add_argument('--catalog',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--family',choices=['shadow_buffer_generation','shadow_buffer_application_lightmap'],default='shadow_buffer_application_lightmap')
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    decoder_source=ROOT/'analysis/frontend/title-camera-matrix-audit/generate_world_vertex_straight_line.py'
    original=ROOT/'analysis/rendering-reference/vsh-prog.c'
    pinned=ROOT/'analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/pgraph/glsl/vsh-prog.c'
    assert original.read_bytes()==pinned.read_bytes()
    spec=importlib.util.spec_from_file_location('original_decoder',decoder_source)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    class LocalModelDecoder(module.Decoder):
        # Extend only the historical verification wrapper's explicit guards.
        # The retained original decode_opcode and operand formatter supply
        # ARL(A0,...) and c[A0+index]; paired ARL remains outside this fixture.
        def operand(self, token, letter):
            if self.field(token,'FLD_'+letter+'_MUX')==3 and self.field(token,'FLD_A0X'):
                index=self.field(token,'FLD_CONST')
                return ('-' if self.field(token,'FLD_'+letter+'_NEG') else '')+'c[A0+'+str(index)+']'+self.swizzle(token,letter)
            return super().operand(token,letter)

        def opcode(self,token,output_mux,mask,opcode,inputs):
            if opcode==self.mac[13]:
                assert output_mux==0 and not self.field(token,'FLD_ILU') and not self.field(token,'FLD_OUT_O_MASK')
                return '  ARL(A0'+inputs+');\n','',['_ARL']
            return super().opcode(token,output_mux,mask,opcode,inputs)

    decoder=LocalModelDecoder(original.read_text())
    helpers=ROOT/'analysis/frontend/title-camera-matrix-audit/world-vertex-straight-line-v2/source-original-helpers.glsl'
    catalog=json.loads(args.catalog.read_text());cache=Cache(args.map)
    tag=next(t for t in cache.tags.values()if t['group']=='vrtx' and t['name'].endswith('\\'+args.family))
    _,root=cache.tag(tag['datum'],'vrtx');rows=[]
    for classification,at in enumerate(cache.block(root+4,28)[:7]):
        semantics=[cache.u16(p)for p in cache.block(at+4,2)]
        code=bytes(cache.data[cache.u32(at+16)-cache.magic:cache.u32(at+16)-cache.magic+cache.u32(at+12)])
        entries=[r for r in catalog['entries']if r['class_index']==classification and r['semantics']==semantics
                 and (args.catalog.parent/r['program']).read_bytes()==code]
        assert len(entries)==1
        native=(args.catalog.parent/entries[0]['source']).read_text()
        prefix=native[:native.index('vec4 h2MenuMultiply(')]
        fetch=native[native.index('float sourceClampW('):native.index('void main() {')]
        epilogue=native[native.index('    vec4 screen=vpOut[0];'):]
        missing=''.join(f'vec4 v{i};\n'for i,s in enumerate(semantics)if s in (4,5,6))
        setup=''.join(f'    v{i}=sourcePackedNormal(v{i}_cmp);\n'for i,s in enumerate(semantics)if s in (4,5,6))
        body=''
        for slot,token in enumerate(struct.iter_unpack('<4I',code[4:])):
            text,_=decoder.token(token)
            body+=f'    /* original slot {slot} */\n'+text+'\n'
        copies=''.join(f'    vpOut[{i}]={name};\n'for i,name in [(0,'oPos'),(3,'oD0'),(4,'oD1'),(5,'oFog'),
            (6,'oPts'),(7,'oB0'),(8,'oB1'),(9,'oT0'),(10,'oT1'),(11,'oT2'),(12,'oT3')])
        (args.output/f'class-{classification}.tokens.bin').write_bytes(code)
        (args.output/f'class-{classification}.declaration.bin').write_bytes(struct.pack('<'+'H'*len(semantics),*semantics))
        (args.output/f'class-{classification}.semantic.glsl').write_text(body)
        source=args.output/f'class-{classification}.vert'
        source.write_text(prefix+helpers.read_text()+missing+fetch+'#define c source.c\nvoid main() {\n'+setup+body+copies+epilogue)
        spirv=source.with_suffix('.spv')
        result=subprocess.run(['glslangValidator','-V','--target-env','vulkan1.3',str(source),'-o',str(spirv)],capture_output=True,text=True)
        source.with_suffix('.log').write_text(result.stdout+result.stderr)
        assert result.returncode==0,result.stdout+result.stderr
        subprocess.run(['spirv-val','--target-env','vulkan1.3',str(spirv)],check=True)
        rows.append(dict(classification=classification,datum=tag['datum'],semantics=semantics,
                         program_sha256=sha(code),declaration_sha256=sha((args.output/f'class-{classification}.declaration.bin').read_bytes()),
                         source_sha256=sha(source.read_bytes()),spirv_sha256=sha(spirv.read_bytes())))
    cache.close()
    (args.output/'report.json').write_text(json.dumps(dict(scope=__doc__,cache_sha256=sha(args.map.read_bytes()),
        family=args.family,decoder_sha256=sha(original.read_bytes()),decoder_tool_sha256=sha(decoder_source.read_bytes()),
        helpers_sha256=sha(helpers.read_bytes()),tool_sha256=sha(Path(__file__).read_bytes()),programs=rows),indent=2)+'\n')
    print('PASS',len(rows),'compiled independent original-emitter authored shadow programs; GPU comparison pending')


if __name__=='__main__':main()
