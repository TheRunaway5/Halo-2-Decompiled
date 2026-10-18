#!/usr/bin/env python3
"""Lower whole owned TU1.5 particle draw and transparent-command owners."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha
from generate_lens_query_tu15 import QueryCode
ENTRIES=(0xbcdf0,0x1e0f0,0x1e4c0,0x140c80,0x140cf0,0x135970,0x136ea0,0x1373f0,0x171070,0x174bf0,0x1770c0,0x1772c0,0x17de60,0x167ef0,0x184690,0x252ec0,0x22ace0,0x22b720,0x380fa0,
         0x176b30,0x177e20,0x22acf0,0x22adb0,0x247d70,0x25c80,0x4012b0,0x401410,
         0x1bd30,0x3ffdd0,0x2c390,
         0x1bbd0,0x16af0,0x3ffc20,0x400ef0,0x3ff5d0,
         0x400a60,0x407850,0x407860,0x407900,0x407ee0,0x407a50,0x13430,0x400270,0x400140,
         0x4021e0,0x402840,0x400920,0x407160,0x406fe0,0x15700,0x401020,
         0x1c2c0,0x1c570,0x400d40,0x400c50,
         0x1c310,0x1c690,0x1c6f0,0x35780,0x35840,0x401520,0x400dd0,0x400e30,
         0x4020d0,0x12d30,0x12f80,0x2bcb0,0x289c0,0x15c030,0x13610,
         0x132b0,0x15350,0x3ff670)
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=Path('src/source_effect_particle_draw_tu15_generated.cpp'));ap.add_argument('--directory',type=Path,required=True);a=ap.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=QueryCode(image);sites,instructions,addresses,routines={},{},{},[]
    assert int.from_bytes(image.read(0x46d990,4),'little')==0x380fa0
    # Authenticated TU1.5 direct callers, the queue producer and actual
    # renderer vtable. A trace beginning at a plausible address is not entry
    # authentication; in particular the old23E460 label is not included.
    calls={0x176b3f:0x177e20,0x177e4c:0x25c80,0x177ef6:0x4012b0,
           0x22ad0b:0x22adb0,0x22ae09:0x247d70,0x247ed5:0x401410,
           0x25d09:0x4012b0,0x25d20:0x4012b0,
           0x22ade9:0x1bd30,0x247e22:0x3ffdd0,0x22adcc:0x2c390,
           0x247eef:0x1bbd0,0x1bbe6:0x16af0,0x1bbec:0x3ffc20,
           0x12653:0x400ef0,0x1bbf3:0x3ff5d0,
           0x401193:0x400a60,0x407f5d:0x407850,0x407f68:0x407850,0x407f77:0x407850,
           0x406a3d:0x407860,0x406c31:0x407900,0x406e11:0x407900,
           0x406d87:0x407ee0,0x406e3a:0x407ee0,0x401a27:0x407a50,
           0x2bcea:0x13430,0x135c4:0x400270,0x40028e:0x400140,
           0x14b11:0x4021e0,0x4023c8:0x402840,0x4023cd:0x400920,
           0x400a4c:0x407160,0x4022f7:0x406fe0,0x13654:0x15700,0x3ff640:0x401020,
           0x247f42:0x1c570,0x1c581:0x1c2c0,0x1c5a7:0x400d40,0x400dc2:0x400c50,
           0x247f74:0x1c690,0x247f99:0x1c6f0,0x247fa3:0x1c6f0,
           0x1c70a:0x1c310,0x1c416:0x35780,0x1c428:0x35840,
           0x1c762:0x401520,0x1c76e:0x401520,0x1c79f:0x400dd0,0x400def:0x400e30,
           0x405b27:0x4020d0,0x4021ce:0x400920,0x4021d5:0x401020,
           0x13315:0x12f80,0x13075:0x12d30,0x12da8:0x400270,0x1329b:0x401410,
           0x13768:0x2bcb0,0x2bcef:0x289c0,0x2bcfc:0x15c030,0x2c307:0x13610,
           0x2bbfb:0x132b0,0x132e8:0x15350,0x1536e:0x3ff670}
    for site,target in calls.items():
        instruction=code.at(site)
        assert instruction.mnemonic=='call' and instruction.operands[0].imm==target
    # Follow complete callers from the already authenticated view parent.
    # Old canonical12D50/156F0/40770 labels are interior TU1.5 bytes and are
    # deliberately absent from this entry set.
    caller_chain=((0x2bcb0,0x2bcea,0x13430),(0x13430,0x13559,0x14aa0),
        (0x14aa0,0x14b11,0x4021e0),(0x4021e0,0x4023c8,0x402840),
        (0x4021e0,0x4023cd,0x400920),(0x400920,0x400a4c,0x407160),
        (0x4021e0,0x4022f7,0x406fe0),(0x2bcb0,0x2c307,0x13610),
        (0x13610,0x13654,0x15700),(0x247d70,0x247f42,0x1c570),
        (0x1c570,0x1c581,0x1c2c0),(0x1c570,0x1c5a7,0x400d40),
        (0x400d40,0x400dc2,0x400c50),(0x247d70,0x247f74,0x1c690),
        (0x247d70,0x247f99,0x1c6f0),(0x1c6f0,0x1c70a,0x1c310),
        (0x1c310,0x1c416,0x35780),(0x1c310,0x1c428,0x35840),
        (0x1c6f0,0x1c762,0x401520),(0x1c6f0,0x1c79f,0x400dd0),
        (0x400dd0,0x400def,0x400e30),(0x405cf0,0x406542,0x405b10),
        (0x405b10,0x405b27,0x4020d0),(0x4020d0,0x4021ce,0x400920),
        (0x4020d0,0x4021d5,0x401020),(0x132b0,0x13315,0x12f80),
        (0x12f80,0x13075,0x12d30),(0x12d30,0x12da8,0x400270),
        (0x12f80,0x1329b,0x401410),(0x13660,0x13768,0x2bcb0),(0x2bcb0,0x2bcef,0x289c0),
        (0x2bcb0,0x2bcfc,0x15c030),(0x2b9f0,0x2bbfb,0x132b0),
        (0x132b0,0x132e8,0x15350),(0x15350,0x1536e,0x3ff670))
    for owner,site,target in caller_chain:
        assert site in _trace(code,owner,{owner}).code
        instruction=code.at(site);assert instruction.mnemonic=='call' and instruction.operands[0].imm==target
    callback=code.at(0x17775e)
    assert callback.mnemonic=='push' and callback.operands[0].imm==0x176b30
    assert int.from_bytes(image.read(0x45f35c,4),'little')==0x22acf0
    for entry in sorted(ENTRIES):
        body=_trace(code,entry,set(ENTRIES))
        if entry==0x407a50:
            # The SDK depth-scale dispatch has no local bounds check. Its
            # admitted surface formats2A..31 select exactly these eight slots.
            # Do not mistake the tracer's empty table list for a direct jump.
            targets=[int.from_bytes(image.read(0x407a7c+4*n,4),'little')for n in range(8)]
            assert targets==[0x407a65,0x407a73,0x407a5e,0x407a6c]*2
            assert code.at(0x407a57).mnemonic=='jmp'
            for target in set(targets):
                first=code.at(target);second=code.at(target+first.size)
                assert first.mnemonic=='mov' and second.mnemonic=='ret'
                body.code.update({target:first.size,second.address:second.size})
            body.tables.append((0x407a7c,4,8))
        assert body.code and all(width in (1,4)for _,width,_ in body.tables)
        sites[f'{entry:08x}']=sorted(body.code);routines.append(dict(entry=entry,tables=[dict(disc=p,count=n)for p,w,n in body.tables if w==4],byte_tables=[dict(address=p,width=w,count=n,raw_hex=image.read(p,w*n).hex())for p,w,n in body.tables if w==1]))
        for pc in sorted(body.code):
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes);instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for o in i.operands:
                if o.type==2:addresses[o.imm&0xffffffff]=o.imm&0xffffffff
                elif o.type==3:addresses[o.mem.disp&0xffffffff]=o.mem.disp&0xffffffff
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json';correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,additional_entries=Path(__file__)),paired,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_effect_particle_draw_tu15.py').replace('halo2/physics_live_support_tu15.hpp','halo2/source_effect_particle_draw_tu15.hpp').replace('source_run_physics_live_support_tu15','source_run_effect_particle_draw_tu15').replace('source_physics_live_support_tu15_owns_entry','source_effect_particle_draw_tu15_owns_entry')
    data_regions=((0x40dac8,26*4),(0x40db30,64),(0x40dfe0,16),(0x4078d4,44),(0x4078ac,40),(0x407a7c,32),(0x46a0c8,0x24),(0x46a250,4),
                  (0x40db78,64),(0x40dff0,40),(0x40ddb0,560),(0x44a8a0,4),
                  (0x46a4b0,4),(0x46a388,4),(0x46a270,4),(0x46a11c,4),(0x46a9b8,4),
                  (0x15638,7*4))
    source+='\nnamespace halo2 {\nvoid mount_source_particle_graphics_layout_data_tu15(SourceRuntimeMemory &memory) {\n'
    for address,size in data_regions:
        raw=image.read(address,size)
        source+=' {constexpr std::array<std::byte,'+str(size)+'> bytes{'+','.join('std::byte{0x%02x}'%v for v in raw)+'};\n'
        source+=' memory.mount(0x%xu,bytes,SourceRuntimeMemory::Access::ReadOnly,"original TU1.5 graphics layout lookup data");}\n'%address
    source+='}\n'
    declaration_regions=((0x357f4,18*4),(0x35878,18*4),(0x44983a,21))
    assert 0x247f7f in sites['00247d70']
    assert code.at(0x247f7f).mnemonic=='mov' and code.at(0x247f7f).operands[1].imm==0x44983a
    source+='void mount_source_particle_declaration_data_tu15(SourceRuntimeMemory &memory) {\n'
    for address,size in declaration_regions:
        raw=image.read(address,size)
        source+=' {constexpr std::array<std::byte,'+str(size)+'> bytes{'+','.join('std::byte{0x%02x}'%v for v in raw)+'};\n'
        source+=' memory.mount(0x%xu,bytes,SourceRuntimeMemory::Access::ReadOnly,"original TU1.5 particle declaration lookup data");}\n'%address
    source+='}\n}\n';a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text());report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),original_call_sites={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},authenticated_view_caller_chain=[dict(owner=p,site=q,target=v)for p,q,v in caller_chain],rejected_interior_entries=[0x12d50,0x156f0,0x40770],layout_data_regions=[dict(address=p,size=n,sha256=__import__('hashlib').sha256(image.read(p,n)).hexdigest())for p,n in data_regions]);report.update(declaration_data_regions=[dict(address=p,size=n,sha256=__import__('hashlib').sha256(image.read(p,n)).hexdigest())for p,n in declaration_regions],packed_particle_declaration_call_site=0x247f7f,packed_particle_declaration_address=0x44983a);(a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n');print('Complete particle draw owners',len(ENTRIES),'instructions',len(instructions))
if __name__=='__main__':main()
