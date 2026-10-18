#!/usr/bin/env python3
"""Run unchanged TU1.5 audio code against declared offline caller states.

These synthetic inputs exercise the original CPU writer; they do not claim
that native startup produced an active game or that the Xbox SDK completed.
Original director, stack-probe and declared clock-data readers execute. Other owners stop
before execution, with no supplied return value or instruction patch.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

GAME, USERS, POOL, SYSTEM, PLAYERS = 0x2100000, 0x2103000, 0x2104000, 0x2105000, 0x2107000
VECTORS, BSP, CLUSTERS, ENVIRONMENTS = 0x210b000, 0x210c000, 0x210d000, 0x210e000
STACK_BASE, STACK, STOP = 0x21fd000, 0x21fff00, 0x3000000
REGS = [getattr(R, 'UC_X86_REG_' + n) for n in ('EAX', 'ECX', 'EDX', 'EBX', 'ESP', 'EBP', 'ESI', 'EDI')]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--recovery', type=Path, required=True)
    p.add_argument('--connected-recovery', type=Path)
    a = p.parse_args()
    if a.output.exists():
        raise ValueError('Preserve previous original audio execution evidence')
    image = Xbe(NEW_DEFAULT)
    if hashlib.sha256(image.data).hexdigest() != NEW_PIN:
        raise ValueError('Expected original English TU1.5 executable')
    from oracle_smg_firing_1_1 import _Code
    code=_Code(image,'.text')
    bodies=json.loads((a.recovery/'bodies.json').read_text())
    # Preserve the original two callers' deliberate callee boundaries even
    # when the supplied recovery now includes those callees as complete code.
    external=set()
    for entry in (0x128650,0x129f10):
        for pc in bodies['body_sites'][f'{entry:08x}']:
            i=code.at(pc);assert image.read(pc,i.size)==bytes(i.bytes)
            if i.mnemonic=='call' and i.operands[0].type==2:
                target=i.operands[0].imm
                if target not in (0x128650,0x129f10,0x158290,0x32a660):external.add(target)
    active_external=set(external)
    connected_external=set()
    literal={}
    if a.connected_recovery:
        connected_manifest=json.loads((a.connected_recovery/'generation.json').read_text())
        connected_external={int(r['target'],16) for r in connected_manifest['external_owners'].values()
                            if r['target'].startswith('0x')} - {0x158290,0x32a660}
        for pc in connected_manifest['lowered_instructions']:
            for op in code.at(int(pc,16)).operands:
                if op.type==3 and not op.mem.base and not op.mem.index:
                    at=op.mem.disp&0xffffffff
                    for i,b in enumerate(image.read(at,op.size)):literal[at+i]=b
    guest = Uc(UC_ARCH_X86, UC_MODE_32)
    guest.mem_map(0x10000, 0x600000)
    guest.mem_map(0x2100000, 0x100000)
    guest.mem_map(STOP, 4096)
    for section in image.sections:
        guest.mem_write(section.va, image.section_bytes(section))
    stores, boundary, visited, rows = [], [], set(), []

    def instruction(m, pc, size, unused):
        if pc in active_external:
            boundary.append(pc)
            m.emu_stop()
        else:
            visited.add(pc)

    def write(m, access, address, size, value, unused):
        stores.append(dict(pc=m.reg_read(R.UC_X86_REG_EIP), address=address,
                           before=bytes(m.mem_read(address, size)).hex(),
                           written=(value & ((1 << (size * 8)) - 1)).to_bytes(size, 'little').hex()))

    guest.hook_add(UC_HOOK_CODE, instruction)
    guest.hook_add(UC_HOOK_MEM_WRITE, write)
    a.output.mkdir(parents=True)

    def case(name, *, entry=0x129f10, game=True, active=1, users=(), install=0,
             mode=0x16f6a0, unit=False, variant=0, environment=-1, plane=0,
             frame_readiness=(0, 0, 0), frame_counter=3, connected=False,
             extra_banks=None, gp_overrides=None, xmm_overrides=None, arguments=None,
             execute_clock=False):
        banks = {}

        def bank(address, data):
            banks[address] = bytearray(data)

        def word(address, value):
            bank(address, struct.pack('<I', value & 0xffffffff))

        bank(GAME, bytes(0x1200)); banks[GAME][0x1120] = active
        word(0x4f3218, GAME if game else 0)
        bank(USERS, bytes(0x50))
        struct.pack_into('<4I', banks[USERS], 0xc,
                         *(0xcafe0000 + i if i in users else 0xffffffff for i in range(4)))
        word(0x4f54f0, USERS)
        bank(POOL, bytes(0x50)); struct.pack_into('<I', banks[POOL], 0x44, PLAYERS)
        word(0x4f54f4, POOL)
        bank(PLAYERS, bytes(4 * 0x21c))
        for i in range(4):
            struct.pack_into('<I', banks[PLAYERS], i * 0x21c + 0x2c, 0xaced0000 if unit else 0xffffffff)
        bank(SYSTEM, bytes([0xa5]) * 0x20c)
        banks[SYSTEM][0x78:0x7b] = bytes(frame_readiness)
        for offset in (0x1b0, 0x1cc):
            struct.pack_into('<I', banks[SYSTEM], offset, 0xffffffff)
        word(0x4f2c50, SYSTEM)
        bank(0x2112000, bytes([0x3c]) * 0x68)
        struct.pack_into('<I', banks[0x2112000], 0x38, frame_counter & 0xffffffff)
        word(0x50ebe4, 0x2112000)
        bank(0x4f5514, bytes(4 * 0x140))
        bank(0x4f64a4, bytes([0x69]) * (4 * 0x358))
        for i in range(4):
            struct.pack_into('<IfI', banks[0x4f5514], i * 0x140, 0x10 + i, float(variant), mode)
            # Callback is at +C; +8 above is retained as an independent word.
            struct.pack_into('<I', banks[0x4f5514], i * 0x140 + 0xc, mode)
            row = i * 0x358
            position = (17.5 + i + variant, -3.25 + i, 4.75 - variant)
            forward, up = ((1., 0., 0.), (0., 0., 1.)) if variant == 0 else ((0., -1., 0.), (1., 0., 0.))
            struct.pack_into('<3fIhh3f3f3f', banks[0x4f64a4], row + 0xb8,
                             *position, 0x12345670 + i, -1 if environment < 0 else 0, 0x1234,
                             0.25 + i, -0.5, 2.0, *forward, *up)
        bank(0x474c24, struct.pack('<h', 0))
        for address in (0x46a0d0, 0x46a0e8, 0x46a11c):
            bank(address, image.read(address, 4))
        bank(0x52d6a0, bytes([variant & 1]))
        # Declared retained SDK input vectors. The original install call stops.
        bank(VECTORS, struct.pack('<12f', *(float(i - 5) / 4 for i in range(12))))
        for address, offset in ((0x474d08, 0), (0x474d10, 12), (0x474ce8, 24), (0x474d04, 36)):
            word(address, VECTORS + offset)
        bank(BSP, bytes(0xc0)); word(0x4ecc18, BSP)
        struct.pack_into('<I', banks[BSP], 0xa0, CLUSTERS)
        struct.pack_into('<I', banks[BSP], 0x68, ENVIRONMENTS)
        bank(CLUSTERS, bytes(4 * 0xb0))
        for i in range(4):
            banks[CLUSTERS][i * 0xb0 + 0x70] = 0xff if environment < 0 else environment
        bank(ENVIRONMENTS, bytes(128 * 24))
        for i in range(128):
            struct.pack_into('<hh4fI', banks[ENVIRONMENTS], i * 24,
                             7, -1 if plane == -1 else 3, 1., 0., 0., float(plane), 0)
        # Object path reaches a real nested owner; no unit-derived return supplied.
        bank(0x210f000, bytes(0x50)); word(0x4ecbd0, 0x210f000)
        struct.pack_into('<I', banks[0x210f000], 0x44, 0x2110000)
        bank(0x2110000, struct.pack('<3I', 0, 0, 0x2111000))
        bank(0x2111000, bytes(0x200))
        struct.pack_into('<I', banks[0x2111000], 0x14, 0xffffffff)
        bank(STACK_BASE, bytes([0x5a]) * 0x3000)
        struct.pack_into('<2I', banks[STACK_BASE], STACK - STACK_BASE, STOP, install)
        if arguments is not None:
            struct.pack_into('<'+'I'*(1+len(arguments)),banks[STACK_BASE],STACK-STACK_BASE,STOP,*arguments)
        if connected:
            for at,data in (extra_banks or {}).items():bank(at,data)
            active_external.clear();active_external.update(connected_external)
            if execute_clock:active_external.discard(0x33b590)
        else:
            active_external.clear();active_external.update(external)
        # Coalesce neighboring declared banks so native reads may span fields.
        data=dict(literal) if connected else {}
        for address,values in banks.items():
            for i,value in enumerate(values):data[address+i]=value
        merged = {}
        for address, value in sorted(data.items()):
            if merged and end == address:
                merged[start].append(value)
            else:
                start = address; merged[start] = bytearray([value])
            end = address + 1
        banks = merged
        gp = [(i + 1) * 0x11111111 for i in range(8)]; gp[4] = STACK
        for index,value in (gp_overrides or {}).items():gp[index]=value&0xffffffff
        xmm = [[(n + 1) * 0x10203 + i for i in range(4)] for n in range(8)]
        for index,words in (xmm_overrides or {}).items():xmm[index]=words
        for address, data in banks.items():
            guest.mem_write(address, bytes(data))
        for register, value in zip(REGS, gp):
            guest.reg_write(register, value)
        for i, words in enumerate(xmm):
            guest.reg_write(getattr(R, 'UC_X86_REG_XMM' + str(i)), int.from_bytes(struct.pack('<4I', *words), 'little'))
        for register, value in ((R.UC_X86_REG_EFLAGS, 2), (R.UC_X86_REG_FPCW, 0x27f),
                                (R.UC_X86_REG_MXCSR, 0x1f80), (R.UC_X86_REG_FPSW, 0), (R.UC_X86_REG_FPTAG, 0xffff)):
            guest.reg_write(register, value)
        stores.clear(); boundary.clear()
        try:
            guest.emu_start(entry, STOP, count=200000)
        except Exception as error:
            raise RuntimeError(f'{name}: original execution failed at {guest.reg_read(R.UC_X86_REG_EIP):08x}: {error}') from error
        terminal = guest.reg_read(R.UC_X86_REG_EIP)
        if terminal != STOP and boundary != [terminal]:
            raise ValueError('Original audio did not return or reach its external owner')
        row = dict(name=name, entry=entry, gp=gp, xmm=xmm,
                   before={f'{p:08x}': b.hex() for p, b in banks.items()},
                   after={f'{p:08x}': bytes(guest.mem_read(p, len(b))).hex() for p, b in banks.items()},
                   gp_after=[guest.reg_read(r) for r in REGS],
                   xmm_after=[list(struct.unpack('<4I', guest.reg_read(getattr(R, 'UC_X86_REG_XMM' + str(i))).to_bytes(16, 'little'))) for i in range(8)],
                   eflags_after=guest.reg_read(R.UC_X86_REG_EFLAGS), external_boundary=boundary[:], stores=stores[:])
        row['external_targets']=sorted(active_external)
        row['clock_owner_executed']=execute_clock
        filename = f'{len(rows):03d}-{name}.json'
        (a.output / filename).write_text(json.dumps(row, indent=2) + '\n')
        rows.append(dict(name=name, entry=entry, file=filename, external_boundary=boundary[:],
                         connected=connected,clock_owner_executed=execute_clock))

    case('no-game', game=False)
    case('inactive-game', active=0, users=(0,), install=1)
    case('empty-users')
    for mask in range(1, 16):
        users = tuple(i for i in range(4) if mask & (1 << i))
        for variant in (0, 1):
            case(f'users-{mask}-basis-{variant}', users=users, variant=variant)
    for active in (2, 255):
        case(f'nonzero-active-{active}', active=active, users=(0, 2))
    for mode in (0x245400, 0x245ea0, 0x246380, 0, 0x16f6a0):
        for variant in (0, 1):
            case(f'actual-director-{mode:x}-{variant}', users=(1, 3), mode=mode, variant=variant)
    for environment, plane in ((0, -1), (0, 0), (0x80, -1), (0x80, 0), (0x80, 40), (0x83, 8)):
        case(f'environment-{environment}-{plane}', users=(0, 2), environment=environment, plane=plane)
    for users in ((), (0,), (0, 1, 2, 3)):
        case('sdk-boundary-' + str(len(users)), users=users, install=1)
    for mode in (0x245400, 0x16f6a0):
        case(f'unit-retained-path-{mode:x}', users=(0,), unit=True, mode=mode)
    for flags in ((0, 0, 0), (0, 1, 1), (1, 0, 1), (1, 1, 0), (1, 1, 1), (255, 2, 3)):
        for counter in (0, 0x7ffffffe, 0x7fffffff, 0xffffffff):
            case(f'frame-readiness-{flags[0]}-{flags[1]}-{flags[2]}-counter-{counter:x}',
                 entry=0x128650, active=0, frame_readiness=flags, frame_counter=counter)
    if a.connected_recovery:
        # Explicit offline driver/pool/output storage, never a constructed
        # native SDK resource or supplied method result.
        driver,output,classes,pool,voices,velocity=0x2121000,0x2125000,0x2120000,0x2126000,0x2127000,0x2128000
        def word(value):return struct.pack('<I',value&0xffffffff)
        base={0x52d674:word(driver),driver:bytes(0x2b00),output:bytes([0x93])*0x400,0x47c778:bytes([0]),
              0x50ebf0:word(classes),classes:bytes(0x360),pool:bytes(0x50),
              0x4f2c4c:word(pool),0x52d668:word(pool),0x4f2c48:word(voices),
              voices:bytes([0xff])*0x900,velocity:bytes(0x100)}
        # Original227410/227415/227417 establish eighteen DWORDs at455CF8.
        base[0x455cf8]=image.read(0x455cf8,18*4)
        for i in range(54):
            b=bytearray(base[classes]);struct.pack_into('<3fI',b,i*16,0.25+i/100,0.75-i/100,
                (0.,0.01,0.5,2.)[i%4],0x98760000+i);base[classes]=bytes(b)
        for step in (-1.,0.,0.005,0.01,1/30,0.5,2.,4.):
            bits=struct.unpack('<I',struct.pack('<f',step))[0]
            case('connected-class-fades-'+str(step),entry=0x228310,connected=True,
                 extra_banks=base,xmm_overrides={1:[bits,0,0,0]})
        for count in (0,1,4,32,64):
            bank=bytearray(base[driver]);struct.pack_into('<hh',bank,4,count,64)
            for i in range(count):
                bank[0xc+i*0x34]=(255,0,31,32,63)[i%5]
                bank[0xd+i*0x34]=i%4
            for i in range(6):struct.pack_into('<h',bank,0x1a0c+i*2,-1 if i%2 else i)
            case('connected-voice-bitmap-'+str(count),entry=0x225d70,connected=True,
                 extra_banks={**base,driver:bytes(bank)},arguments=(output,))
        for entry in (0x12ab30,0x12ade0,0x2206d0,0x2207a0,0x221af0):
            case(f'connected-pool-owner-boundary-{entry:x}',entry=entry,connected=True,
                 extra_banks=base,arguments=(0,))
        for entry in (0x225610,0x225890,0x225fb0):
            case(f'connected-sdk-owner-boundary-{entry:x}',entry=entry,connected=True,extra_banks=base)
        for entry in (0x12bbe0,0x12bc10,0x12be90,0x12bf00,0x12cbc0,0x228480,0x129500):
            s=bytearray(0x20c)
            struct.pack_into('<h',s,0x208,0)
            for i in range(4):struct.pack_into('<Ih',s,0x88+i*0x48,0xffffffff,-1)
            case(f'connected-voice-caller-{entry:x}',entry=entry,connected=True,
                 extra_banks={**base,SYSTEM:bytes(s)},gp_overrides={0:0,3:output,7:output},
                 arguments=(0,output,output+0x100))
        for mode in (0,1):
            bank=bytearray(base[driver]);bank[0]=mode
            parameters=bytearray(0x38)
            struct.pack_into('<9f',parameters,0,1.,2.,3.,1.,0.,0.,0.,0.,1.)
            struct.pack_into('<2I',parameters,0x30,0,velocity)
            bank[0x2918:0x2918+36]=parameters[:36]
            case('connected-listener-sdk-'+str(mode),entry=0x2271a0,connected=True,
                 extra_banks={**base,driver:bytes(bank),output:parameters},arguments=(output,))
        for flags in ((0,1,1),(1,0,1),(1,1,0),(1,1,1)):
            case('connected-full-frame-'+''.join(map(str,flags)),entry=0x128650,connected=True,
                 frame_readiness=flags,extra_banks=base)
        for users in ((),(0,),(0,1,2,3)):
            case('connected-full-listener-'+str(len(users)),entry=0x129f10,connected=True,
                 users=users,install=1,extra_banks=base)
        for mask in range(16):
            case('connected-local-user-search-'+str(mask),entry=0x150900,connected=True,
                 users=tuple(i for i in range(4) if mask&(1<<i)),extra_banks=base)
        for entry in (0x120060,0x120310):
            for fraction in (0.,0.25,0.5,1.):
                bits=struct.unpack('<I',struct.pack('<f',fraction))[0]
                case(f'connected-listener-basis-{entry:x}-{fraction}',entry=entry,connected=True,
                     extra_banks={**base,velocity:struct.pack('<8f',1.,0.,0.,0.,0.,1.,0.,0.)},
                     gp_overrides={0:output,1:velocity,2:velocity+16},xmm_overrides={0:[bits,0,0,0]})
        gates=(0x4f9c6d,0x556e03,0x556dfe,0x556dff,0x556e02,0x556e00,0x556e01,0x556e06,0x4f2d40,0x4f2d70)
        gate_banks={at:bytes(4 if at in (0x4f2d40,0x4f2d70) else 1) for at in gates}
        def clock_case(name,last,now,*,paused=0,gain=0.25,delay=-1,gate=None,kind=1,active=1,game=True):
            g=bytearray(0x1200);g[0x1120]=active;g[0x1121]=paused;struct.pack_into('<I',g,8,kind)
            s=bytearray(0x20c);struct.pack_into('<II',s,0x80,last,0x1020)
            struct.pack_into('<fifff',s,0x1e4,gain,delay,0.05,0.1,0.2)
            struct.pack_into('<f',s,0x1fc,0.5);struct.pack_into('<f',s,0x204,0.75)
            clocks=bytearray(base[velocity]);struct.pack_into('<I',clocks,0x80,now)
            state={**base,**gate_banks,GAME:g,SYSTEM:s,0x41b774:word(velocity+0x80),velocity:clocks}
            if gate is not None:state[gate]=word(1) if gate in (0x4f2d40,0x4f2d70) else bytes([1])
            case('connected-clock-'+name,entry=0x129500,connected=True,execute_clock=True,
                 extra_banks=state,active=active,game=game)
        for last,now in ((1000,1000),(1000,1001),(1000,1033),(1000,1501),(0xfffffff0,0x10),(200,100)):
            for paused in (0,1):
                for gain in (0.,0.25,1.):
                    for delay in (-1,0,2):
                        clock_case(f'{last:x}-{now:x}-pause{paused}-gain{gain}-delay{delay}',last,now,
                                   paused=paused,gain=gain,delay=delay)
        for gate in gates:clock_case('gate-'+hex(gate),1000,1033,gate=gate)
        for name,kwargs in (('no-game',dict(game=False)),('inactive',dict(active=0)),('other-kind',dict(kind=2))):
            clock_case(name,1000,1033,**kwargs)
    report = dict(scope=__doc__, suite='audio', xbe_sha256=NEW_PIN,
                  instruction_patches=[], supplied_external_results=[], external_targets=sorted(external),
                  executed_original_callees=[0x158290,0x32a660]+([0x33b590] if any(c['clock_owner_executed'] for c in rows) else []),
                  connected_cases=sum(c['connected'] for c in rows),clock_cases=sum(c['clock_owner_executed'] for c in rows),
                  covered_instructions=sorted(visited), cases=rows,
                  complete_engine_verified=False, native_startup_readiness_verified=False,
                  final_volume_or_sample_timing_verified=False)
    (a.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS', len(rows), 'unchanged audio caller cases;', len(visited), 'executed instructions; no supplied external results')


if __name__ == '__main__':
    main()
