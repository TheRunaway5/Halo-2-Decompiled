#!/usr/bin/env python3
"""Execute original whole unit/base callbacks at folded StringID comparisons.

Explicit component object/debug fields are inputs. No runtime admission,
callee, instruction, voice or rendering result is replaced.
"""
import hashlib,json,os,re,struct
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe
from oracle_weapon_heat_shader_tu15 import Original
from oracle_weapon_barrel_timing_tu15 import WEAPON,DEF,TIME,STACK,STOP,OUT,WI,pack
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from unicorn import x86_const as R
ROOT=Path(__file__).resolve().parents[1]
def main():
    Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(image.data).hexdigest()==NEW_PIN
    original=Original('TU1.5',image);g=original.g
    debug=0x1014000;g.mem_write(0x4eabc4,pack([debug]))
    values=json.loads((ROOT/'tests/fixtures/weapon_firing_material_tu15.json').read_text())['raw_values']
    choices=[(0xc9070,0x1100059b,0x110005a7,'driver_seat_power',0x25c),
             (0xc9070,0x1100059c,0x110005a8,'gunner_seat_power',0x260)]
    choices += [(0xbec50,0x1a00071a+n,0x1a000729+n,'scripted_object_function_'+chr(97+n),0x70+n*4) for n in range(4)]
    names={name:((len(name)<<24)|int(old,16),int(new,16)) for old,new,name in re.findall(
        r'\{0x([0-9a-f]+)u,0x([0-9a-f]+)u,"([^"]*)"\}',(ROOT/'src/ui/mcc_frontend_string_ids.inc').read_text())}
    choices += [(0x10be20,*names[name],name,offset) for name,offset in (
        ('locked',0x134),('loop',0x140),('power',0x134),('delay',0x148),
        ('position',0x140),('change_in_power',0x138),('change_in_position',0x144))]
    rows=[]
    for entry,selector,mcc,name,offset in choices:
        for value in values:
            unit=entry==0xc9070
            device=entry==0x10be20
            g.mem_write(0x51df54,pack([TIME]));g.mem_write(TIME+4,pack([0x3d088889]))
            g.mem_write(WEAPON+0xaa,b'\x00')
            g.mem_write(WEAPON+0x12c,pack([0]))
            g.mem_write(WEAPON+0x134,pack([0x3e800000,0x3e000000,0x3f000000,0x3e000000,1]))
            g.mem_write(DEF+0xc0,pack([0x3f000000]));g.mem_write(DEF+0xc8,pack([0x3e800000]))
            g.mem_write(DEF+0x10c,pack([0x3f800000]))
            g.mem_write(WEAPON+0x25c,pack([0x3e000000,0x3e800000]))
            g.mem_write(debug+0x70,pack([0x3e000000,0x3e800000,0x3f000000,0x3f400000]))
            g.mem_write((WEAPON if unit or device else debug)+offset,pack([value]))
            g.mem_write(OUT,bytes.fromhex('efbeaddeefbeadde'))
            g.mem_write(STACK,pack([STOP,WI,selector,OUT,OUT+4]))
            g.reg_write(R.UC_X86_REG_ESP,STACK);g.reg_write(R.UC_X86_REG_FPCW,0x27f)
            g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
            g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
            g.emu_start(entry,STOP,count=10000)
            assert g.reg_read(R.UC_X86_REG_EIP)==STOP and g.reg_read(R.UC_X86_REG_ESP)==STACK+20
            rows.append(dict(entry=entry,selector=selector,mcc_selector=mcc,name=name,offset=offset,
                unit=unit,device=device,input_bits=value,value_bits=struct.unpack('<I',g.mem_read(OUT,4))[0],
                active=g.mem_read(OUT+4,1)[0],recognized=g.reg_read(R.UC_X86_REG_EAX)&255))
    result=dict(scope=__doc__,original_xbe_sha256=NEW_PIN,cases=rows,
        executed_instructions=original.instructions,source_instruction_patches=[],supplied_callee_results=[])
    # Retain a small required original reference, never the emulator bank.
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols'
    fixture=ROOT/'tests/fixtures/object_function_folded_symbols_tu15.json'
    fixture.write_text(json.dumps({k:v for k,v in result.items() if k!='executed_instructions'},indent=2)+'\n')
    result['fixture_sha256']=hashlib.sha256(fixture.read_bytes()).hexdigest()
    (out/'folded-original-execution.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(dict(cases=len(rows),fixture_bytes=fixture.stat().st_size,fixture_sha256=result['fixture_sha256'])),flush=True)
if __name__=='__main__':main()
