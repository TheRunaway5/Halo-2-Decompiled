#!/usr/bin/env python3
"""Lower complete unchanged TU1.5 lens reflection and primitive owners."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha
from generate_lens_query_tu15 import QueryCode

ENTRIES = (0x2e9c0,0x2e3d0,0x2f2b0,0x48090,0x47fc0,0x47e90,
    0x14540,0x14460,0x13a030,0x12360,0x12310,0x14370,0x13e960,0x13e6e0,
    0x1346c0,0x134a50,0x166a80,0x166ae0,0x1cf30,0x402400,0x4054c0,
    0x3ff5d0,0x400920,0x401020,0x400470,0x400270,0x48af0,0x40b080,
    0x405560,0x20d60,0x13e670,0x13de70,0x409b60,0x409ee0,0x40d4e0,
    0x40d440,0x406fe0,0x1301e0,0x359d0,0x139080,0x140e80,
    0x1ccd0,0x4044b0,0x4000d0,0x17f790,0x17f8a0,
    0x13660,0x1bdd0,0x1fd00,0x226f0,0x16aa0,0x1de00)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/source_lens_draw_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,required=True)
    a=p.parse_args()
    assert sha(NEW_DEFAULT)==NEW_PIN
    x=Xbe(NEW_DEFAULT);code=QueryCode(x)
    callback=code.at(0x2e16c)
    assert callback.mnemonic=='push' and callback.operands[0].imm==0x2e9c0
    calls={0x2ea29:0x2e3d0,0x2ea75:0x20d60,0x2ed47:0x2f2b0,
        0x2edba:0x13e960,0x2ecb6:0x166a80,0x166a96:0x166ae0,
        0x2f061:0x47fc0,0x2f08c:0x3ff5d0,0x2f1e4:0x134a50,
        0x2f227:0x48090,0x47fcd:0x14540,0x47fdd:0x14460,
        0x47fef:0x47e90,0x14578:0x13a030,0x14582:0x12360,
        0x145b4:0x14370,0x14379:0x12310,0x13e971:0x13e6e0,
        0x13e97a:0x1346c0,0x480ca:0x48af0,0x480d7:0x1cf30,
        0x1cf4a:0x402400,0x480e2:0x40b080,0x48121:0x4054c0,
        0x48487:0x405560,0x3ff63b:0x400920,0x3ff640:0x401020,
        0x3ff65e:0x400470,0x3ff663:0x400270}
    for pc,target in calls.items():
        i=code.at(pc);assert i.mnemonic=='call' and i.operands[0].imm==target
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
        assert all(width in (1,4)for _,width,_ in body.tables)
        tables=[dict(disc=pc,count=count)for pc,width,count in body.tables if width==4]
        translations=[dict(address=pc,width=width,count=count,raw_hex=x.read(pc,width*count).hex())
            for pc,width,count in body.tables if width==1]
        sites[f'{entry:08x}']=sorted(body.code)
        routines.append(dict(entry=entry,tables=tables,byte_tables=translations))
        for pc in sorted(body.code):
            i=code.at(pc);assert x.read(pc,i.size)==bytes(i.bytes)
            instructions[f'{pc:08x}']=pc;addresses[pc]=pc;addresses[pc+i.size]=pc+i.size
            for op in i.operands:
                if op.type==2:addresses[op.imm&0xffffffff]=op.imm&0xffffffff
                elif op.type==3:addresses[op.mem.disp&0xffffffff]=op.mem.disp&0xffffffff
        for pc,width,count in body.tables:
            addresses[pc]=pc
            if width==4:
                for n in range(count):
                    target=int.from_bytes(x.read(pc+n*4,4),'little');addresses[target]=target
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,
        entries={f'{p:08x}':p for p in ENTRIES},body_sites=sites,instruction_map=instructions,
        address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);bodies=a.directory/'bodies.json'
    bodies.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',
        correspondence=bodies,additional_entries=Path(__file__)),paired,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_lens_draw_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_lens_draw_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_lens_draw_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_lens_draw_tu15_owns_entry')
    a.output.write_text(source);report=json.loads((a.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(a.output),module_generator_sha256=sha(Path(__file__)),
        original_call_sites={f'{p:08x}':f'{q:08x}'for p,q in calls.items()},
        callback=dict(site='0002e16c',entry='0002e9c0'),
        boundary='Complete original reflection/draw bodies. Native resource admission, full parent shader/state publication and final GPU consumption retain separate owners; no original call result is fabricated.')
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print('Complete original lens draw owners',len(ENTRIES),'instructions',len(instructions))


if __name__=='__main__':
    main()
