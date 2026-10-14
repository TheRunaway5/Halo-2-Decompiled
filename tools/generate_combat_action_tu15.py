#!/usr/bin/env python3
"""Lower complete authenticated TU1.5 combat/action functions to native C++."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import generate,sha

# Actual EA7F1 reaches F0550; its sole original callee is D23B0.
ENTRIES=(0xf0550,0xd23b0,0xbdec0,0xb2e40,
         # Normal carried-item admission reached by actual dual pickup.
         0xb8180,
         # Actual Elite biped construction reaches this complete physics owner.
         0x1e32a0,0x285370,0x28fba0,0x2883c0,0x28b870,0x28bf00,
         # Original local-view action-cache construction/reset, required by1EEDB0.
         0x1eec30,0x1eecf0,0x1eed30,0x148de0,0x1a9d90,0x11f440,0x1a9f60,
         # Complete texture-cache lock/load/scale frame owner and its callees.
         0x12edf0,0x12ef10,0x12ef60,0x12f040,0x12f0c0,0x12f110,0x140430,
         0x12fd10,0x4057f0,0x404e30,0x33b590,
         # Actual local source feedback/formatting owners reached after view binding.
         0x1a7c10,0x142630,0x150990,0x15b1d0,0x15b210,0x15bb50,
         0x164660,0x1648e0,0x165fc0,0x1a7ce0,0x1a8670,0x1a86f0,
         0x1a9910,0x233cf1,0x32bf34,0x32bff9,0x32da9e,0x32dac3,
         0x32dae8,0x32db25,0x330389,0x330491,
         0x32bdc2,0x32a970,0x32a8ed,
         0x25aaf1,0x25abde,0x1425a0,0x16ea60,
         0x1563a0,0x1563e0,0x156dd0,0x2293e0,0x229420,
         0x244460,0x2446e0,0x2447f0,0x244d00,0x244bd0,0x244350,
         # Actual1816F1 handles instanced-geometry impact projection.
         0x181c00,0x1453b0,0x1454d0,
         # Complete prepared-shot effect selection/refresh/creation.
         0x108a70,0x106f90,0x17dff0,0x179470,
         # Actual103AEB reaches the original energy-weapon ready presentation.
         0x107880,0x108b60,0x10b330,
         # Actual Needler FP constraint and Sentinel weapon-switch continuations.
         0x296db0,0xc62b0,0x108db0,0x100e70,0x16ea20,0x107e30,
         # Complete remaining direct trigger/charge continuations and their
         # reached helper closure; keep the existing1030F0 owner intact.
         0xfc8d0,0x102c70,0x102d50,0x107080,0x1074a0,0x107ee0,
         0x107fe0,0x108040,0x108b30,0x108be0,0x108c40,0x108ca0,
         0x108d10,0x10a810,0x10a8d0,0x10a910,0x10af40,
         0x165590,0x1655f0,0x24b430,
         # Complete13A9CC cached-decal fade/retirement prelude and callbacks.
         0x180520,0x43980,0x13fd10,0x17faf0,0x17fb60,0x140310,
         0x43810,0x43840,0x1806a0,0x32cc0c,
         # Original Midship authored scenery physical callback reached by PVS.
         0x2f1cc0,
         0x2f1a50,
         # Complete original command melee target and weapon aim query.
         0xc6920,0xcdcc0,0xce5a0,0xe79c0,0xe8110,0xe82a0,0xe8900,
         0x10c920,0x10c9a0,0x11b560,0x153ba0,0x153e10,0x153f20,
         0x1540d0,0x1541b0,0x154270,0x16cea0,0x1864e0,
         0xbd460,0xe1c10,0x106160,0x1062a0,0x14d200,0x1ad460,
         0x1adc90,0x1adf60,0x1ae090,0x1ae2b0,0x1ae500,0x1ae5e0,
         0x1ae6f0,0x1ae7f0,0x1aea30,0x1aece0,0x1aeed0,0x1aef30,
         0x1af1a0,0x1af2e0,0x1af3e0,0x32b4f0,0x32b560,
         0x1582f0,0x244f80,0x245560,0x245d10,0x37d0b4,0x37d0d1,
         # Reached original Midship spatial/collision owners.
         0x16cf00,0x16d0e0,0x16d2a0,0x1e6050,0x1f84f0,0x24dfb0,
         0x24e0f0,0x24e580,0x24e710,0x24e850,0x24eb60,0x24edb0,
         0x1eeb00,0x24f4b0,0x24f670,0x24f9c0,0x24fcc0,
         0x1f39d0,0x1f3b80,
         # Original live melee contact stimulus at186636.
         0x2f7130,0x2f6eb0,0x30b980,
         # Actual melee victim recovery/physics continuation E4DAB.
         0xe31c0,0x1dd3d0,
         # EA8D0's original prepare/release and their unbound direct owners.
         0xe9c80,0xe9e50,0xb33f0,0xff820,0x100d40,0x102560,0xb2610,
         0x1eabe0,0x207620,0x207200,0x2072d0,0x2673a0,0x275fe0,
         # Actual projectile type5 constructor reached at10DB94.
         0xfd200,0x1028b0,0x3ddc0,0x3dd00,0x2c390,
         # Original44B350 cached render-state construction/map activation.
         0x2b480,0x3d260,0x2b520,0x3b890,0x16780,
         # Original17E577 source effect environment classification.
         0xd4f90,0x14da70,0xc19d0,0x14d790,0x14d410,0x171010,0x1211c0,
         # Complete source lighting/color consumers reached by D4F90.
         # D3D imports remain explicit native resource boundaries.
         0xd4350,0x12360,0x15cb0,0x1cf90,0x1d2d0,0x1d490,0x241a0,
         0x24470,0x358c0,0xd3b20,0xd3b70,0xd3bc0,0xd42b0,0xd5130,
         0xd5300,0xd54d0,0xd5610,0xd5700,0xd58e0,0xd5ae0,0xd5b70,
         0x134960,0x138360,0x138510,0x1385d0,0x1386e0,0x138780,
         0x138c20,0x138d90,0x138f70,0x138fc0,0x1404d0,0x140510,
         0x1710f0,0x1458a0,0x22ac90,0x22a880,0x22a8e0,0x22aa50,0x22aa90,0x243f20,
         # Original projectile attachment update at283EE4.
         0x120740,0x284290,0x292280,0x2928e0,0x292c90,0x293290,
         0x2935c0,0x293cb0,0x2940f0,0x2947b0,0x2843c0,
         # Type5 activation callback selected by original10DD5B.
         0xfdde0,
         # Complete FDEB0 projectile update, detonation and direct owners.
         0x24530,0x48e60,0x4efa0,0xb2710,0xb27f0,0xb29d0,0xb3110,
         0xb87d0,0xbe200,0xfd6f0,0xfd7f0,0xfdeb0,0xffa30,0xffa60,
         0x100fd0,0x101030,0x101330,0x101bc0,0x101ea0,0x102410,
         0x102460,0x1024c0,0x102740,0x1027d0,0x10eab0,0x13a6b0,
         0x145140,0x149150,0x158290,0x176a40,0x176ab0,0x176e70,
         0x177f10,0x178020,0x178070,0x1782d0,0x17b980,0x17b9c0,
         0x17ea60,0x17ed80,0x17eed0,0x17ef70,0x17f710,0x186ed0,
         0x187270,0x1872b0,0x18c5b0,0x1ce210,0x1d2940,0x1d2ca0,
         0x208500,0x21f740,0x24ffe0,0x2512e0,0x251700,0x251930,
         0x251fd0,0x2520a0,0x252100,0x252db0,0x116720,0x1136a0,
         0x186cb0,0x186d30,
         # Original178E30 explosion allocation/visibility/location callees.
         0xd50f0,0x10f160,0x14dc50,0x169151,0x169376,0x1693ab,
         0x16945e,0x172200,0x178de0,0x1790a0,0x17b0d0,0x17b880,
         0x17ba50,0x17dd50,0x17ddc0,0x17e460,
         # Original D9259 radial-damage object query.
         0xbd080,0xbaa40,0xd9a30,
         # Type27's complete melee selection, posed contact and presentation.
         # Original network/kernel imports stay explicit required owners.
         0xb2f20,0xb3490,0xcd440,0xd13d0,0xd18c0,0xd2180,0xe0f20,
         0xe13b0,0xebd00,0xebf20,0xebfa0,0x10c420,0x10c4e0,
         0x156840,0x1578a0,0x157920,0x157d70,0x1974b0,0x223cd0,
         0x223d70,0x226130,0x229340,0x2296c0,0x22c7f0,
         0x1800b0,0x180170,0x43880,0xd9830,0xd9910,0xd94e0,
         0xbd020,0xd9a00,0x186780,0x315be0,0x31faa0,
         0x15f630,0x165370,0x179cd0,0x2e4ea0,0xfcec0,
         0x1870c0,0x1ce080,0x1ce0b0,0xbd8b0,0x10dfa0,0xd1220,0xdeaa0,0xff9f0,
         0xc36a0,0xc5290,0xd6fa0,0xc0e90,0x1cc390,0x1d74d0,0x1d7ca0,
         # Remaining original fatal-physics cleanup owners admitted by main.
         0x1d3e00,0x1d8cd0,0x1e4a40,0x324f80,
         # BA407 invokes every original474BC4 object-deletion callback.
         0x155820,0x160b70,0x1611e0,0x1693ed,0x17e250,0x17e320,
         0x18a850,0x1d0640,0x1d21e0,0x1e70b0,0x1e7130,0x1e9a80,
         0x1ea400,0x1ec970,0x20d5e0,0x2118a0,0x2125b0,0x212760,
         0x217b80,0x2182f0,0x2186a0,0x219910,0x264720,0x274800,
         0x2748e0,0x274970,0x274b40,0x278080,0x297f10,0x2980a0,
         0x299a10,0x299aa0,0x29a8a0,0x2a0040,0x2a0080,0x2a00c0,
         # Complete D3D metadata/address helpers; no GPU/device execution.
         0x405770,0x407aa0,0x407ca0)

class CombatCode:
    def __init__(self,image):
        self.parts=(_Code(image,'.text'),_Code(image,'D3D'))
        self.never=set()
    def owner(self,p):
        return next((part for part in self.parts if part.inside(p)),None)
    def inside(self,p):return self.owner(p) is not None
    def at(self,p):return self.owner(p).at(p) if self.owner(p) else None
    def dword(self,p):return self.owner(p).dword(p) if self.owner(p) else None
    def byte(self,p):return self.owner(p).byte(p) if self.owner(p) else None
    def boundary(self,p):return self.owner(p).boundary(p) if self.owner(p) else False
    def prologue(self,p):return self.owner(p).prologue(p) if self.owner(p) else False

def combat_instructions(*,get,put,addr,**unused):
    def emit(i):
        if i.address==0xd1fbf:
            assert i.mnemonic=='call' and i.operands[0].imm==0xd13d0
            # Observe the actual contact at this full ABI boundary, then run
            # D13D0 unchanged. The host supplies no collision/material return.
            return 's.push(0xd1fbfu,0xd1fc4u);s.required(0xd1fbfu,0xd13d0u,false);'
        if i.mnemonic!='cmpltss':return None
        left,right=i.operands
        assert left.type==1 and i.reg_name(left.reg).startswith('xmm')
        # Scalar SSE comparison preserves the destination's upper lanes;
        # unordered inputs produce a false low-lane mask, as on the Xbox.
        value=f'_mm_set_ss(f({get(i,right)}))' if right.type==3 else get(i,right)
        return put(i,left,f'_mm_cmplt_ss({get(i,left)},{value})')
    return emit

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=Path('src/source_combat_action_tu15_generated.cpp'))
    p.add_argument('--directory',type=Path,default=Path('analysis/weapon-effects-20261013/live-tu15/combat-functions'))
    a=p.parse_args();assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=CombatCode(image)
    assert code.at(0xea7f1).bytes.hex()=='e85a5d0000'
    assert code.at(0xea843).bytes.hex()=='e87836fdff'
    assert code.at(0xea849).bytes.hex()=='e8f285fcff'
    assert code.at(0xea95a).mnemonic=='call' and code.at(0xea95a).operands[0].imm==0xe9c80
    assert image.read(0x44b350,12).hex()=='80b40200a0b5020020b50200'
    assert image.read(0x44b404,12).hex()=='b00018003001180070011800'
    assert int.from_bytes(image.read(0x473d28+27*4,4),'little')==0x473b74
    assert image.read(0x473b74,16).hex()=='50c40e0020c60e000000000050c70e00'
    import struct
    assert struct.unpack('<8I',image.read(0x474bc4,32))==(0xbd8b0,0xc36a0,
        0x1d21e0,0x155820,0x1693ed,0x1611e0,0x2118a0,0x17e250)
    assert code.at(0xd1fbf).mnemonic=='call' and code.at(0xd1fbf).operands[0].imm==0xd13d0
    sites,instructions,addresses,routines={},{},{},[]
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES));assert body.code
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
    paired=dict(image='English TU1.5',original_sha256=NEW_PIN,entries={f'{p:08x}':p for p in ENTRIES},
        body_sites=sites,instruction_map=instructions,address_map={f'{p:08x}':q for p,q in addresses.items()},routines=routines)
    a.directory.mkdir(parents=True,exist_ok=True);correspondence=a.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(output=a.output,manifest=a.directory/'generation.json',correspondence=correspondence,
        additional_entries=Path(__file__)),paired,extra_instruction_factory=combat_instructions,code_override=code)
    source=a.output.read_text().replace('tools/generate_physics_live_support_tu15.py','tools/generate_combat_action_tu15.py')
    source=source.replace('halo2/physics_live_support_tu15.hpp','halo2/source_combat_action_tu15.hpp')
    source=source.replace('source_run_physics_live_support_tu15','source_run_combat_action_tu15')
    source=source.replace('source_physics_live_support_tu15_owns_entry','source_combat_action_tu15_owns_entry')
    a.output.write_text(source)
    report=json.loads((a.directory/'generation.json').read_text());report['generated_cpp_sha256']=sha(a.output)
    (a.directory/'generation.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
