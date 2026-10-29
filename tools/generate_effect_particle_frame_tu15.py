#!/usr/bin/env python3
"""Lower complete owned TU1.5 effect-particle frame functions to native C++."""
import argparse,json,re,struct
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

# Source13AA28 calls179DF0 with the frame delta. The effect walk tail-calls
#177830;1778D0 owns the source particle clock, emitter/child update and drain.
# Helpers and both actual particle metadata vtables retain their whole bodies.
ENTRIES=(0x3eb10,0x3f3e0,0x30bd0,0x11f9c0,0x13a6b0,0x145560,0x1458a0,0x1459b0,0x148de0,
    0x166a80,0x16e8a0,0x16ea60,0x177020,0x177830,0x1778d0,0x177fa0,
    0x178070,0x178110,0x178240,0x178290,0x178920,0x179df0,0x179ed0,
    0x17ba10,0x17ba50,0x17dce0,0x17dd50,0x17ddc0,0x17de10,
    0x22ac50,0x22ac60,0x22ac70,0x22ac80,0x22aca0,0x22acc0,0x22acd0,0x22ad20,0x22ad30,0x22ad40,0x22ad50,0x22ad60,0x22b700,0x22b710,0x22b780,0x22b790,
    # Literal44B56C/44B574 initialize/reset the per-frame audio state read
    # by13A9B0's complete18EE50 prelude. Keep the actual arena/checksum owner.
    0x22c440,0x22c480,
    0x250450,0x250ac0,0x251650,0x2516c0,0x2519f0,0x251a60,0x251f70,0x252060,0x2c2ad0,0x2c40d0,
    # Actual2C2B01 collision owner and its literal helper closure. Metadata
    # vtable slots0/4/38/3C are authenticated below, never supplied returns.
    0x2c2d90,0x176a40,0x24ffe0,0x2c4110,0x250310,0x250380,0x2502b0,
    0x252100,0x18bbc0,0x2501c0,0x250220,0x178e30,0x4efa0,0x16e940,
    0x22b6e0,0x22b6f0,0x22b750,0x22b760,
    # Actual CRT row46D978 constructs1024C0's static fallback records.
    0x380e70)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=Path('src/rendering/effects/source_effect_particle_frame_tu15_generated.cpp'));ap.add_argument('--directory',type=Path,required=True)
    ap.add_argument('--preserve-existing-bodies',action='store_true',help='Append newly authenticated entries without regenerating already qualified bodies or shared helpers')
    a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    existing=a.output.read_text() if a.preserve_existing_bodies else None
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    assert code.at(0x13aa28).operands[0].imm==0x179df0
    assert code.at(0x179ec8).operands[0].imm==0x177830
    assert code.at(0x2c2b01).mnemonic=='call' and code.at(0x2c2b01).operands[0].imm==0x2c2d90
    assert struct.unpack('<4I',image.read(0x44b56c,16))==(0x22c440,0x31d400,0x22c480,0x31d400)
    assert struct.unpack('<I',image.read(0x46d978,4))[0]==0x380e70
    tables={}
    for pointer in (0x486048,0x48603c):
        table=struct.unpack('<I',image.read(pointer,4))[0]
        targets={offset:struct.unpack('<I',image.read(table+offset,4))[0] for offset in (0,4,0x8,0xc,0x20,0x24,0x30,0x34,0x38,0x3c,0x44,0x48,0x4c,0x50)}
        # Actual251187 calls the selected metadata object's slot1C. Model
        #22B710 uses+7C; sprite22ACA0 uses+88. Authenticate both complete
        # bodies rather than treating the distinct parent layouts alike.
        targets[0x1c]=struct.unpack('<I',image.read(table+0x1c,4))[0]
        assert targets[0x1c]==(0x22aca0 if pointer==0x48603c else 0x22b710)
        assert set(targets.values())<=set(ENTRIES);tables[f'{pointer:08x}']=dict(table=table,targets=targets)
    sites,instructions,addresses,routines={},{},{},[]
    for entry in sorted(ENTRIES):
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4) for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=[dict(disc=p,count=n) for p,w,n in body.tables if w==4],byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex()) for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for operand in i.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generated_output=a.directory/'generated.cpp' if existing is not None else a.output
    generate(SimpleNamespace(output=generated_output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired)
    source=generated_output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_effect_particle_frame_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_effect_particle_frame_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_effect_particle_frame_tu15').replace('source_physics_live_support_tu15_owns_entry','source_effect_particle_frame_tu15_owns_entry')
    preserved=0
    if existing is not None:
        pattern=r'static void run_([0-9a-f]+)\(WorldState &s\) \{\n(.*?)\n\}'
        old_bodies=dict(re.findall(pattern,existing,re.S));new_bodies=dict(re.findall(pattern,source,re.S))
        assert set(old_bodies)<=set(new_bodies) and old_bodies
        source=existing;preserved=len(old_bodies)
        first=re.search(pattern,source,re.S).start()
        for entry in sorted(set(new_bodies)-set(old_bodies),key=lambda value:int(value,16)):
            body=f'static void run_{entry}(WorldState &s) {{\n{new_bodies[entry]}\n}}\n\n'
            source=source[:first]+body+source[first:];first+=len(body)
            source=source.replace('switch(entry) {','switch(entry) {\n'+f'case 0x{entry}u:run_{entry}(s);break;',1)
            owner='bool source_effect_particle_frame_tu15_owns_entry(std::uint32_t p) {switch(p){'
            assert owner in source
            source=source.replace(owner,owner+f'case 0x{entry}u:',1)
        # Virtual calls inside the module need the same complete owner as its
        # exported entry control. Keep the declaration ahead of local dispatch.
        for entry in ('22b710',):
            declaration=f'static void run_{entry}(WorldState &);\n'
            source=source.replace(declaration,'')
            position=source.index('static void run_')
            source=source[:position]+declaration+source[position:]
            selected='std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image){if(image==SourcePhysicsContactImage::MapPack11)return p;switch(p){'
            case=f'case 0x{entry}u:return 0x{entry}u;'
            if case not in source:
                assert selected in source
                source=source.replace(selected,selected+case,1)
            dispatch='static void dispatch(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail){'
            route=f'if(target==selected(0x{entry}u,s.image)){{run_{entry}(s);return;}}'
            if route not in source:
                assert dispatch in source
                source=source.replace(dispatch,dispatch+route,1)
        assert all(dict(re.findall(pattern,source,re.S))[entry]==body for entry,body in old_bodies.items())
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),particle_vtables=tables,preserved_existing_bodies=preserved)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print('Complete source particle-frame owners',len(ENTRIES),'instructions',len(instructions))
if __name__=='__main__':main()
