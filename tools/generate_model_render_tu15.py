#!/usr/bin/env python3
"""Compile authenticated TU1.5 model resource/render callbacks to native C++."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from generate_combat_action_tu15 import CombatCode
# Original model resource admission, render-cache lifecycle and current
# lighting/node services. All bodies are compiled from the pinned TU1.5 image.
ENTRIES=(
    0x41c10,0x41c70,0x41970,0xd7230,
    # Original PRT frame cache, section allocation, SH sampling/rotation and
    # PCA projection. Empty/invalid banks retain the original caller state.
    0x22730,0x22830,0x22c10,0x22cd0,0x23520,0x23580,0x235e0,0x23670,0x23c80,
    0x1456f0,0x145d50,0x145e60,0x146100,0x148b20,
    0x12360,0x15cb0,0x16780,0x1cf90,0x1d2d0,0x1d490,0x241a0,0x24470,
    0x24570,0x2b480,0x2b520,0x2c390,0x358c0,0x3b890,0x3d260,0x3daf0,
    0x3ddc0,0x3dfd0,0x3e4d0,0x3e590,0x3ea20,0xbc200,0xbcdf0,0xbcfb0,
    0xbf640,0xd3b20,0xd3b70,0xd3bc0,0xd3d90,0xd3f70,0xd42b0,0xd4350,
    # Actual4B96E render-collector skin/compound palette and its inverse-node
    # transform helper. Preserve BF640's completed render overlays.
    0x13a1a0,0x13a2e0,
    0xd4f90,0xd5130,0xd5300,0xd54d0,0xd5610,0xd5700,0xd58e0,0xd5ae0,
    0xd5b70,0xd5d30,0xd65c0,0xe7bc0,0x110240,0x110c10,0x1112e0,0x1114b0,0x115940,0x1159d0,
    # BF640's actual equipped render-pose overlays and their shared sampler.
    0x2130c0,0x2cec60,
    0x11a360,0x1211c0,0x130d20,0x1346c0,0x134960,0x1349e0,0x138360,0x138510,
    0x1385d0,0x1386e0,0x138780,0x138c20,0x138d90,0x138f70,0x138fc0,0x1404d0,
    0x140510,0x14d410,0x14d790,0x14da70,0x171010,0x1710f0,0x22a790,0x22a880,
    0x22a8e0,0x22aa50,0x22aa90,0x243f20,0x405770,0x407aa0,0x407ca0,
)
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/rendering/models/source_model_render_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,default=Path('analysis/shader-parity-20261013/model-render-functions-v1'))
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=CombatCode(image)
    # Literal TU1.5 codec descriptors: name, flags and seven callback slots.
    # Equipped render overlays can select formats absent from collision poses.
    codec_names=(
        '_no_compression_codec','_uncompressed_static_data_codec',
        '_uncompressed_animated_data_codec','_8byte_quantized_rotation_only_codec',
        'byte_keyframe_lightly_quantized','word_keyframe_lightly_quantized',
        'reverse_byte_keyframe_lightly_quantized','reverse_word_keyframe_lightly_quantized',
        '_blend_screen_codec')
    codec_rows=[];codec_entries=set()
    for index,expected in enumerate(codec_names):
        address=0x48c330+index*40;raw=image.read(address,40)
        values=struct.unpack('<10I',raw)
        assert image.read(values[0],80).split(b'\0')[0].decode()==expected
        callbacks=values[3:]
        assert all(not callback or code.inside(callback) for callback in callbacks)
        codec_entries.update(callback for callback in callbacks if callback)
        codec_rows.append(dict(address=address,name=expected,raw_hex=raw.hex(),callbacks=callbacks))
    entries=tuple(sorted(set(ENTRIES)|codec_entries))
    assert code.at(0x13ffa7).mnemonic=='call'
    assert code.at(0x130d92).operands[0].imm==0x22a790
    sites,instructions,addresses,routines={},{},{},[]
    for entry in entries:
        body=_trace(code,entry,set(entries));assert body.code
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],
            byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in entries},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},
        routines=routines,codec_descriptors=codec_rows)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_model_render_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_model_render_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_model_render_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_model_render_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
