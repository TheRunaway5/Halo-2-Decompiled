#!/usr/bin/env python3
"""Unchanged TU1.5 activation/variant caller cases with explicit retained banks.

No external result, callback success, GPU operation or readiness is supplied.
The fixture stops at the exact external-owner entry, including dynamic original
registration callbacks. Declared banks are CPU inputs, not a live Xbox session.
"""
import argparse,hashlib,json,struct
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

GAME,SCENARIO,RANDOM,OPTIONS,BSP,ARENA,STACK,STOP=(
    0x2100000,0x2102000,0x2103000,0x2104000,0x2106000,0x2110000,0x21fff00,0x3000000)
REGS=[getattr(R,'UC_X86_REG_'+name) for name in ('EAX','ECX','EDX','EBX','ESP','EBP','ESI','EDI')]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--recovery',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    if args.output.exists():raise ValueError('Preserve existing original evidence')
    args.output.mkdir(parents=True)
    image=Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest()!=NEW_PIN:raise ValueError('Expected original English TU1.5')
    code=_Code(image,'.text')
    recovery=json.loads((args.recovery/'generation.json').read_text())
    entries={int(p,16) for p in recovery['complete_native_entries']}
    external={int(row['target'],16) for row in recovery['external_owners'].values() if row['target'].startswith('0x')}
    for row in range(68):
        for column in (0x44aed8,0x44aee0,0x44aee4,0x44aee8):
            target=int.from_bytes(image.read(column+row*0x24,4),'little')
            if target and target not in entries:external.add(target)
    guest=Uc(UC_ARCH_X86,UC_MODE_32)
    guest.mem_map(0x10000,0x600000);guest.mem_map(0x2100000,0x100000);guest.mem_map(STOP,4096)
    for section in image.sections:guest.mem_write(section.va,image.section_bytes(section))
    literal={}
    for entry in entries:
        body=_trace(code,entry,entries)
        for pc in body.code:
            for operand in code.at(pc).operands:
                if operand.type==3 and not operand.mem.base and not operand.mem.index:
                    at=operand.mem.disp&0xffffffff
                    for n,b in enumerate(guest.mem_read(at,operand.size)):literal[at+n]=b
        for at,width,count in body.tables:
            for n,b in enumerate(image.read(at,width*count)):literal[at+n]=b
    for n,b in enumerate(image.read(0x44aed0,0x9a0)):literal[0x44aed0+n]=b
    # Actual219FD2 bounds the random vector ordinal to0..0x401. This is
    # indexed source data, not a jump table or a replacement random sequence.
    instruction=code.at(0x219fd2)
    if instruction.mnemonic!='imul' or instruction.operands[2].imm!=0x402:
        raise ValueError('Original reset vector table bound differs')
    for n,b in enumerate(image.read(0x44b8e8,0x402*12)):literal[0x44b8e8+n]=b
    for row in range(13):
        at=0x474b90+row*4;pointer=int.from_bytes(image.read(at,4),'little')
        for n,b in enumerate(image.read(at,4)):literal[at+n]=b
        for n,b in enumerate(image.read(pointer,16)):literal[pointer+n]=b
        for n,b in enumerate(image.read(0x44a834+row*4,4)):literal[0x44a834+row*4+n]=b
    rows,stores,boundary,visited=[],[],[],set()
    def instruction(machine,pc,size,unused):
        if pc in external:boundary.append(pc);machine.emu_stop()
        else:visited.add(pc)
    def write(machine,access,address,size,value,unused):
        stores.append(dict(pc=machine.reg_read(R.UC_X86_REG_EIP),address=address,
            before=bytes(machine.mem_read(address,size)).hex(),
            written=(value&((1<<(size*8))-1)).to_bytes(size,'little').hex()))
    guest.hook_add(UC_HOOK_CODE,instruction);guest.hook_add(UC_HOOK_MEM_WRITE,write)
    def case(name,entry,*,selected=0,game_bsp=-1,requested=0,kind=1,variant=0,membership_gate=0,block=None):
        data=dict(literal)
        def put(at,value):
            for n,b in enumerate(value):data[at+n]=b
        def word(at,value):put(at,struct.pack('<I',value&0xffffffff))
        put(GAME,bytes((i*11+7)&255 for i in range(0x1200)))
        put(GAME+2,struct.pack('<h',game_bsp));word(GAME+8,kind)
        put(SCENARIO,bytes(0x240));word(SCENARIO+0x210,2)
        put(OPTIONS,bytes(0x1118));word(OPTIONS,kind);word(OPTIONS+0x10,0x12345678)
        word(OPTIONS+0x134+0x44,variant)
        put(BSP,bytes(0x200));put(ARENA,bytes((i*5+1)&255 for i in range(0x1300)))
        caller_arg=OPTIONS if entry in (0x13a8b0,0x13a780) else ARENA if entry==0x86c30 else ARENA+0x528 if entry==0x2260e0 else requested&0xffffffff
        put(0x21fe000,bytes(0x2000));put(STACK,struct.pack('<3I',STOP,caller_arg,1))
        for at,value in ((0x4f3218,GAME),(0x4ecc20,SCENARIO),(0x4ecc18,BSP),
                         (0x4f3cd8,RANDOM),(0x4f2964,ARENA),(0x4ecc1c,0),(0x52dfd0,0)):
            word(at,value)
        word(RANDOM,0x98765432);put(0x474c24,struct.pack('<h',selected));put(0x4f9c6c,bytes([membership_gate]))
        # Explicit CPU caller destinations, not native constructor results.
        # External owners still stop before executing or supplying a result.
        for callback,binding in ((0x1888f0,0x4f9b54),(0x1f38d0,0x52be68),
                (0x219300,0x52be68),(0x2282b0,0x50ebf0),(0xd7360,0x4ecbf4),
                (0x22c9c0,0x52d688)):
            if entry==callback:word(binding,ARENA+0x300)
        if entry==0x1f38d0:word(0x474d04,ARENA+0x400)
        if entry in (0x11e550,0x219f90):put(0x505e90,bytes([0x3c])*0xd0c)
        if entry==0x19a430:put(0x51dfb0,bytes([0x3c])*0xde78)
        if entry in (0x22570,0x1281a0):word(0x4ecc1c,BSP)
        if entry==0x22570:
            put(0x4c2ac0,b'\0');word(0x515ec0,ARENA+0x300)
            word(0x515e2c,ARENA+0x700);word(0x515ec4,ARENA+0x800)
            put(0x52dfd8,bytes([0x3c])*0x40c0)
        if entry==0x1f290:
            put(0x4c2ac0,bytes([block!='inactive']))
            put(0x4c2ad0,b''.join(struct.pack('<II',
                0xcafe0000|(i%64) if block=='live' and i%5 else 0xffffffff,0x12340000+i)
                for i in range(512)))
            word(0x55c404,ARENA);word(ARENA+0x44,ARENA+0x200)
        if entry==0x1281a0:
            word(BSP+0xc0,1);word(BSP+0xc4,ARENA+0x300);word(ARENA+0x320,0)
            word(0x4f0414,ARENA+0x500);word(ARENA+0x508,0xabcdef01);word(0x4f2c50,ARENA+0x700)
        if entry==0x128900:
            word(0x4ecc1c,BSP);word(BSP+0xc0,1);word(BSP+0xc4,ARENA+0x100)
            word(ARENA+0x114,0xffffffff if block=='absent' else 0xabcd0000)
            word(0x4f0414,ARENA+0x300);word(ARENA+0x308,ARENA+0x500)
            word(0x4f2c50,ARENA+0x700)
        if entry in (0x128900,0x2260e0):
            word(0x52d674,ARENA+0x4000);put(ARENA+0x4000,bytes([0x3c])*0x2ab8)
            word(ARENA+0x6ab4,0 if block=='missing' else 0xf0000123)
        if entry==0x197230:
            word(0x51df90,ARENA+0x100);word(0x52d674,ARENA+0x4000)
            put(ARENA+0x4000,bytes([0x3c])*0x2ab4)
            word(ARENA+0x6ab0,0 if block=='missing' else 0xf0005678)
        if entry==0x18b660:word(0x4f9b5c,0 if block=='absent' else ARENA+0x300)
        if entry==0x14a38b:word(0x55c434,0)
        if entry==0x1378c0:word(0x4f3010,0 if block=='absent' else ARENA+0x300)
        if entry==0x86c30:
            word(ARENA+0xc,0);word(GAME+0x270,5);put(GAME+0x2d4,b'\0')
            put(0x4dc3e8,bytes([membership_gate]))
        if entry==0x191260:
            put(ARENA+0x3c,bytes([1 if block=='flagged' else 0]));put(ARENA+0x3f,b'\0')
            word(ARENA+0x28,ARENA+0x100);word(ARENA+0x34,ARENA+0x200 if block=='move' else 0)
            word(ARENA+0x200,0x20);word(ARENA+0x204,ARENA+0x1f0);word(ARENA+0x208,0)
        # A direct variant call consumes the same admitted variant region as
        # the options-copy owner; all fields start as explicitly declared0.
        gp=[(i+1)*0x11111111 for i in range(8)];gp[0]=OPTIONS;gp[3]=OPTIONS+0x134;gp[4]=STACK;gp[6]=OPTIONS
        if entry==0x191260:gp[7]=ARENA
        if entry==0x86bd0:gp[6]=ARENA
        if entry==0x887b0:gp[2]=ARENA+0x300
        if entry==0x2260e0:gp[1]=ARENA+0x520;gp[2]=ARENA+0x518
        banks={};start=None;values=bytearray()
        for at,value in sorted(data.items()):
            if start is None:start=at
            if at!=start+len(values):banks[start]=bytes(values);start=at;values.clear()
            values.append(value)
        if start is not None:banks[start]=bytes(values)
        for at,value in banks.items():guest.mem_write(at,value)
        for register,value in zip(REGS,gp):guest.reg_write(register,value)
        xmm=[[(n+1)*0x10203+i for i in range(4)] for n in range(8)]
        for n,words in enumerate(xmm):guest.reg_write(getattr(R,'UC_X86_REG_XMM'+str(n)),int.from_bytes(struct.pack('<4I',*words),'little'))
        for register,value in ((R.UC_X86_REG_EFLAGS,2),(R.UC_X86_REG_FPCW,0x27f),(R.UC_X86_REG_MXCSR,0x1f80),
                               (R.UC_X86_REG_FPSW,0),(R.UC_X86_REG_FPTAG,0xffff)):guest.reg_write(register,value)
        stores.clear();boundary.clear();guest.emu_start(entry,STOP,count=200000)
        terminal=guest.reg_read(R.UC_X86_REG_EIP)
        if terminal!=STOP and boundary!=[terminal]:raise ValueError('Original activation did not return or reach a declared external owner')
        result=dict(name=name,entry=entry,gp=gp,xmm=xmm,before={f'{at:08x}':value.hex() for at,value in banks.items()},
            after={f'{at:08x}':bytes(guest.mem_read(at,len(value))).hex() for at,value in banks.items()},
            gp_after=[guest.reg_read(r)for r in REGS],
            xmm_after=[list(struct.unpack('<4I',guest.reg_read(getattr(R,'UC_X86_REG_XMM'+str(n))).to_bytes(16,'little')))for n in range(8)],
            eflags_after=guest.reg_read(R.UC_X86_REG_EFLAGS),external_boundary=boundary[:],stores=stores[:])
        # Stream the large ordered-store cases: building dumps()'s complete
        # chunk list alongside the retained bytes exceeds the fixed guard.
        filename=f'{len(rows):03d}-{name}.json'
        with (args.output/filename).open('w') as output:
            json.dump(result,output,indent=2);output.write('\n')
        rows.append(dict(name=name,entry=entry,file=filename,external_boundary=boundary[:]))
    case('bsp-already-selected',0x11e8e0,game_bsp=0)
    case('bsp-invalid-negative',0x11e8e0,requested=-2)
    case('bsp-out-of-range',0x11e8e0,requested=2)
    case('bsp-none-already-none',0x11e8e0,selected=-1,game_bsp=-1,requested=-1)
    case('bsp-switch-prepare',0x11e8e0,game_bsp=0,requested=1)
    case('bsp-first-bind',0x11e8e0)
    case('bsp-callbacks-current',0x13a820)
    case('bsp-callbacks-none',0x13a820,selected=-1)
    case('bsp-unload-callbacks',0x13a880)
    case('options-copy-campaign',0x13a8b0)
    case('options-copy-multiplayer',0x13a8b0,kind=2,variant=2)
    case('options-copy-campaign-variant',0x13a8b0,variant=2)
    case('membership-gated',0x13a920,membership_gate=1)
    case('membership-owner',0x13a920)
    case('map-start-owner',0x191df0)
    case('map-reset-owner',0x13a780)
    case('shader-cache-reset',0x3f2a0)
    case('hash-storage-absent',0x1e1a0)
    case('bsp-effect-reset',0x36aa0)
    case('palette-stage-owner',0x15b50)
    case('original-first-new-map-flag',0x11f5d0)
    case('original-bsp-placement-owner',0xd7b80)
    case('map-physical-arena-owner',0x126730)
    for entry in (0x1888f0,0x692d0,0x11e550,0x1f38d0,0x22570,0x171f40,
                  0x219300,0x2282b0,0x1281a0,0x14a38b,0x1410b6,0xd7360,0x22c9c0):
        case('new-map-'+f'{entry:x}',entry)
    case('new-map-pool-absent',0x18b660,block='absent')
    case('new-map-pool-present',0x18b660,block='present')
    case('new-map-storage-absent',0x1378c0,block='absent')
    case('new-map-storage-present',0x1378c0,block='present')
    case('simulation-player-reset',0x86bd0)
    case('simulation-block-reset',0x887b0)
    case('simulation-options-local',0x86c30)
    case('simulation-options-other-kind',0x86c30,kind=3)
    case('simulation-options-session-owner',0x86c30,membership_gate=1)
    case('simulation-session-statistics-owner',0x19a430)
    case('new-map-vector-table-reset',0x219f90)
    case('render-reset-inactive',0x1f290,block='inactive')
    case('render-reset-invalid-handles',0x1f290,block='invalid')
    case('render-reset-live-handles',0x1f290,block='live')
    case('environment-reset-absent',0x128900,block='absent')
    case('environment-reset-owner',0x128900,block='present')
    case('effect-data-reset-missing-device',0x197230,block='missing')
    case('effect-data-reset-declared-device',0x197230,block='declared')
    case('environment-parameters-missing-plugin',0x2260e0,block='missing')
    case('environment-parameters-declared-plugin',0x2260e0,block='declared')
    case('block-empty',0x191260,block='empty')
    case('block-flagged',0x191260,block='flagged')
    case('block-move-owner',0x191260,block='move')
    for value in range(11):case('variant-type-'+str(value),0x1a7170,variant=value)
    report=dict(scope=__doc__,suite='activation',xbe_sha256=NEW_PIN,instruction_patches=[],supplied_external_results=[],
        cases=rows,covered_instructions=sorted(visited),complete_activation_verified=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',len(rows),'unchanged activation caller cases;',len(visited),'executed instruction addresses; no external result supplied')


if __name__=='__main__':main()
