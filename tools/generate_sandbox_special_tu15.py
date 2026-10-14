#!/usr/bin/env python3
"""Lower complete unchanged TU1.5 sandbox special-action owners to native C++."""
import argparse
import json
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# Actual1ED997 reaches1F39D0 during the source-selected distant sword action.
# Its complete movement consumer calls1F3B80. Existing world owners retain
# their own direct callees; no target, physics result or service is supplied.
ENTRIES = (0x1f39d0, 0x1f3b80, 0x10f080, 0x30bd0,
           # Actual1F2553 records the source lunge contact. Preserve its
           # real recording gates, event identity, clock and ring consumers.
           0x19bb60, 0x19b8f0, 0x19de80, 0x19bb20,
           # Actual1F2565 continues the same lunge contact through retained
           # contact identity/history and its complete source consumers.
           0x1f2890, 0x1f1fc0, 0x1f20d0, 0x19b860, 0x19b2b0,
           # Actual1F2740 completes source lunge damage notifications through
           # the original game-engine/recording/attribution owner.
           0x15f8b0, 0x25bc90,
           # Literal Slayer slot78 reached by1600E4 after actual lunge damage.
           0x2c5dd0, 0x15e1e0, 0x161530, 0x248550, 0x19b7d0,
           # Actual1F2879 completes the original contact notification tail.
           0x1f2a50, 0x22c280, 0x165670,
           # Original TU1.5 animation event116BDF (legacy observer115CBF)
           # reaches the sound bit-bank consumer and its direct callees.
           0x213890, 0x21fe40, 0x140ea0,
           # Literal startup/map callback record44B524 owns the sound bit
           # bank. The XAPI wrapper retains its real native memory boundary.
           0x2131b0, 0x213260, 0x33b3ac,
           # Original116DDF (legacy observer115EBF) resolves the animation
           # event's authored sound metadata through the real ugh owner.
           0x116cd0, 0x213690, 0x21f670, 0x21f6b0,
           # The real contact collector at2F8B27 invokes literal41CA30+4.
           0x30b980,
           # ActualE4DAB finishes the source action's physical transition.
           0xe31c0, 0x1dd3d0, 0x1cc310,
           # Actual10380B reaches this complete authored barrel admission
           # owner when ordinary delivered primary input reaches the weapon.
           0x107610,
           # Actual1042D8 then consumes the same barrel's shot-rate/dual
           # ownership and original counter admission, including all stores.
           0x1071d0,
           # Actual104304 owns the prepared shot's ammunition, heat, shared
           # RNG, effects and notifications. Preserve the complete routine.
           0x108080,
           # Actual108338 publishes the original magazine-change notification.
           0xb1f00,
           # Actual10B0E4 owns unit action/hand notification after firing.
           0xf0be0,
           # Actual1087A9 retains the original item battery/age owner.
           0x106e80,
           # Actual1087F6 publishes the source firing stimulus and its
           # current original AI-admission gate; do not synthesize that gate.
           0xc1c60, 0x1d23e0,
           # Remaining complete direct consumers of the same prepared-shot
           # owner. External descendants keep their actual required ABI.
           0xc95e0, 0xd2920, 0xbcdf0, 0x17f8a0, 0x18a300, 0xcabc0,
           0x109100, 0x106be0, 0xbad60, 0x179800,
           # Actual10456E completes the original barrel-error postpass.
           0x102fb0, 0x106760, 0xd39d0,
           # Actual10295E executes the original prepared-projectile postpass,
           # including its complete source aim/spawn/contact consumer.
           0x102980, 0x109220,
           # Unbound direct owners in109220: original attribution, aim,
           # markers, shot randomness, placement and post-contact publication.
           0x15f6e0,0xca770,0x10eeb0,0xcd4a0,0xbcf70,0x16d620,
           0x10b4b0,0x10b3d0,0x1ad7f0,0x206090,0xb9960,0x148e20,
           0x10a680,0xb1a70,0xff100,0xbac30,0x145070,0xbc330,0xd91d0,0xb1f80,
           # Actual1AD81C and the complete original aim-query descendants.
           0xcdbb0,0x11fd70,0x1af670,0x1af920,0xbcfb0,
           # ActualE978A manual reload reads the original magazine phase.
           0x107aa0,
           # Actual107D00 reload transfer spends the genuine shared reserve.
           0x105c50,
           # Actual102F4E completes the magazine's original idle transition.
           0x107d30,
           # Actual102F64 completes original chamber state/effect ownership.
           0x107d90,
           # Literal13AB80 owns registered local first-person observation;
           # actual168D55 advances the original manager/pose clock.
           0x168ca1,0x169c46,
           # Unbound complete local-view lookup, feedback/input and packed
           # pose consumers reached by the actual registered FP update.
           0x150990,0xcaef0,0xd28f0,0x13f620,0x16961f,0x16a4dd,
           0x18a210,0x18a830,0x1d4510,
           # Actual1EEDFD consumes this player/view cache after source-local
           # binding. Its owning44B1C4 constructor/new-map reset are mandatory.
           0x1eecf0,0x1eed30,
           # Original1EFAA5 tail-dispatches actual local weapon input history.
           0x1eec30,
           # Actual2296DE consumes the per-view feedback bank. Preserve its
           # literal44B590 constructor/new-map reset, including slot gains.
           0x2293e0,0x229420,
           # Actual15792E consumes the local player effect/impulse bank.
           0x1563a0,0x1563e0,
           # Literal16A002 installs this real FP animation-event callback;
           # retain its complete authored sound/event consumer.
           0x169c21,0x169ba2,
           # Literal16A009 selects the same original callback for hand1.
           0x169c33,
           # Literal44B3BC director constructor/new-map reset owns the
           # first-person callback and camera gates read by158290/16B1BC.
           0x157e10,0x157eb0,
           # Complete direct consumers of the now-admitted16A4DD pose path:
           # ammunition, root matrix, packed node count/output and transition.
           0x105c10,0x1441c0,0x1d3fe0,0x1d4000,0x1d4030,0x1d4560,
           # Actual1D4000/1D4030 dispatch full original hierarchy builders.
           0x1e5470,0x1e5540,
           # Actual284EA7 calls the literal48C36C codec callback, copied
           # into510F6C by the original packed-animation codec selector.
           0x296220,
           # Actual1692AB publishes original FP muzzle/marker nodes once the
           # real camera gate admits its pose; keep complete map consumers.
           0x1e1360,0x1e11b0,0x1e1200,
           # Original3D35A submits the actual FP draw records to168DED.
           # Keep complete camera composition, graph remapping and marker IK.
           0x168ded,0x168d76,0x16b240,0x16b4e8,0x144cb0,0x1d3f90,0x1e5a50)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path,
                        default=Path('src/source_sandbox_special_tu15_generated.cpp'))
    parser.add_argument('--directory', type=Path,
                        default=Path('analysis/sandbox-tu15-goal-20261013/special-functions'))
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    assert code.at(0x1ed997).mnemonic == 'call'
    assert code.at(0x1ed997).operands[0].imm == 0x1f39d0
    assert code.at(0x1f2553).mnemonic == 'call'
    assert code.at(0x1f2553).operands[0].imm == 0x19bb60
    assert code.at(0x1f2565).mnemonic == 'call'
    assert code.at(0x1f2565).operands[0].imm == 0x1f2890
    assert code.at(0x1f2740).mnemonic == 'call'
    assert code.at(0x1f2740).operands[0].imm == 0x15f8b0
    # Original15FD63 calls the selected engine's literal slot7C. This is
    # the actual original two-instruction body, not a supplied host return.
    assert int.from_bytes(image.read(0x468be8+0x7c, 4), 'little') == 0x25bc90
    assert int.from_bytes(image.read(0x468be8+0x78, 4), 'little') == 0x2c5dd0
    assert code.at(0x1f2766).mnemonic == 'call'
    assert code.at(0x1f2766).operands[0].imm == 0x19b7d0
    assert code.at(0x1f2879).mnemonic == 'call'
    assert code.at(0x1f2879).operands[0].imm == 0x1f2a50
    assert code.at(0x116bdf).mnemonic == 'call'
    assert code.at(0x116bdf).operands[0].imm == 0x213890
    assert image.read(0x44b524, 12).hex() == 'b03121002032210060322100'
    assert int.from_bytes(image.read(0x41b6d4, 4), 'little') == 0x800000b8
    assert code.at(0x116ddf).mnemonic == 'call'
    assert code.at(0x116ddf).operands[0].imm == 0x116cd0
    assert code.at(0x2f8b27).bytes.hex() == 'ff5004'
    assert int.from_bytes(image.read(0x41ca34, 4), 'little') == 0x30b980
    assert code.at(0xe4dab).mnemonic == 'call'
    assert code.at(0xe4dab).operands[0].imm == 0xe31c0
    assert code.at(0x10380b).mnemonic == 'call'
    assert code.at(0x10380b).operands[0].imm == 0x107610
    assert code.at(0x1042d8).mnemonic == 'call'
    assert code.at(0x1042d8).operands[0].imm == 0x1071d0
    assert code.at(0x104304).mnemonic == 'call'
    assert code.at(0x104304).operands[0].imm == 0x108080
    assert code.at(0x108338).mnemonic == 'call'
    assert code.at(0x108338).operands[0].imm == 0xb1f00
    assert code.at(0x10b0e4).mnemonic == 'call'
    assert code.at(0x10b0e4).operands[0].imm == 0xf0be0
    assert code.at(0x1087a9).mnemonic == 'call'
    assert code.at(0x1087a9).operands[0].imm == 0x106e80
    assert code.at(0x1087f6).mnemonic == 'call'
    assert code.at(0x1087f6).operands[0].imm == 0xc1c60
    assert code.at(0xc1c9a).mnemonic == 'call'
    assert code.at(0xc1c9a).operands[0].imm == 0x1d23e0
    shot_calls={code.at(p).operands[0].imm for p in _trace(code,0x108080,{0x108080}).code
                if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0xc95e0,0xd2920,0xbcdf0,0x17f8a0,0x18a300,0xcabc0,
            0x109100,0x106be0,0xbad60,0x179800}<=shot_calls
    for site,target in ((0x10456e,0x102fb0),(0x103010,0x106760),(0x103039,0xd39d0)):
        assert code.at(site).mnemonic=='call' and code.at(site).operands[0].imm==target
    for site,target in ((0x10295e,0x102980),(0x1029d7,0x109220)):
        assert code.at(site).mnemonic=='call' and code.at(site).operands[0].imm==target
    projectile_calls={code.at(p).operands[0].imm for p in _trace(code,0x109220,{0x109220}).code
                      if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0x15f6e0,0xca770,0x10eeb0,0xcd4a0,0xbcf70,0x16d620,
            0x10b4b0,0x10b3d0,0x1ad7f0,0x206090,0xb9960,0x148e20,
            0x10a680,0xb1a70,0xff100,0xbac30,0x145070,0xbc330,0xd91d0,0xb1f80}<=projectile_calls
    aim_calls={code.at(p).operands[0].imm for p in _trace(code,0x1ad7f0,{0x1ad7f0}).code
               if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0xcdbb0,0x11fd70,0x1af670,0x1af920,0xbcfb0}<=aim_calls
    assert code.at(0xe978a).mnemonic=='call' and code.at(0xe978a).operands[0].imm==0x107aa0
    assert code.at(0x107d00).mnemonic=='call' and code.at(0x107d00).operands[0].imm==0x105c50
    assert code.at(0x102f4e).mnemonic=='call' and code.at(0x102f4e).operands[0].imm==0x107d30
    assert code.at(0x102f64).mnemonic=='call' and code.at(0x102f64).operands[0].imm==0x107d90
    for site,target in ((0x13ab80,0x168ca1),(0x168d55,0x169c46)):
        assert code.at(site).mnemonic=='call' and code.at(site).operands[0].imm==target
    assert code.at(0x168cba).mnemonic=='call' and code.at(0x168cba).operands[0].imm==0x150990
    fp_calls={code.at(p).operands[0].imm for p in _trace(code,0x169c46,{0x169c46}).code
              if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0xcaef0,0xd28f0,0x13f620,0x16961f,0x16a4dd,0x18a210,0x18a830,0x1d4510}<=fp_calls
    assert image.read(0x44b1c4,12).hex()=='f0ec1e0000d4310030ed1e00'
    assert code.at(0x1efaa5).mnemonic=='jmp' and code.at(0x1efaa5).operands[0].imm==0x1eec30
    assert image.read(0x44b590,12).hex()=='e093220000d4310020942200'
    assert image.read(0x44b5b4,12).hex()=='a063150000d43100e0631500'
    assert code.at(0x16a002).mnemonic=='mov' and code.at(0x16a002).operands[1].imm==0x169c21
    assert code.at(0x169c2b).mnemonic=='call' and code.at(0x169c2b).operands[0].imm==0x169ba2
    assert code.at(0x16a009).mnemonic=='mov' and code.at(0x16a009).operands[1].imm==0x169c33
    assert code.at(0x169c3e).mnemonic=='call' and code.at(0x169c3e).operands[0].imm==0x169ba2
    assert image.read(0x44b3bc,16).hex()=='107e150000d43100b07e1500c07f1500'
    pose_calls={code.at(p).operands[0].imm for p in _trace(code,0x16a4dd,{0x16a4dd}).code
                if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0x105c10,0x1441c0,0x1d3fe0,0x1d4000,0x1d4030,0x1d4560}<=pose_calls
    for parent,target in ((0x1d4000,0x1e5470),(0x1d4030,0x1e5540)):
        assert any(code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2 and
                   code.at(p).operands[0].imm==target for p in _trace(code,parent,{parent}).code)
    assert int.from_bytes(image.read(0x48c36c,4),'little')==0x296220
    assert image.read(0x284ea7,6).hex()=='ff156c0f5100'
    assert code.at(0x1692ab).mnemonic=='call' and code.at(0x1692ab).operands[0].imm==0x1e1360
    marker_calls={code.at(p).operands[0].imm for p in _trace(code,0x1e1360,{0x1e1360}).code
                  if code.at(p).mnemonic=='call' and code.at(p).operands[0].type==2}
    assert {0x1e11b0,0x1e1200}<=marker_calls
    for site,target in ((0x3d35a,0x168ded),(0x169103,0x16b4e8),(0x168d9f,0x16b240)):
        assert code.at(site).mnemonic=='call' and code.at(site).operands[0].imm==target
    assert image.read(0x3d351,2).hex()=='6a04'
    assert code.at(0x2ba58).op_str=='dword ptr [0x4c66f8], ecx'
    assert code.at(0x2baa6).op_str=='edi, 0x4c65c0'
    sites, instructions, addresses, routines = {}, {}, {}, []
    args.directory.mkdir(parents=True, exist_ok=True)
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n,
                raw_hex=image.read(p, width*n).hex())
                for p, width, n in body.tables if width == 1]))
        listing = []
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            listing.append(f'{pc:08x} {instruction.bytes.hex()} '
                           f'{instruction.mnemonic} {instruction.op_str}\n')
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc+instruction.size] = pc+instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
        (args.directory/f'{entry:08x}.asm').write_text(''.join(listing))
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
        entries={f'{p:08x}': p for p in ENTRIES}, body_sites=sites,
        instruction_map=instructions,
        address_map={f'{p:08x}': value for p, value in addresses.items()}, routines=routines)
    correspondence = args.directory/'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True)+'\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
        correspondence=correspondence, additional_entries=Path(__file__)), paired)
    source = args.output.read_text()
    source = source.replace('tools/generate_physics_live_support_tu15.py',
                            'tools/generate_sandbox_special_tu15.py')
    source = source.replace('halo2/physics_live_support_tu15.hpp',
                            'halo2/source_sandbox_special_tu15.hpp')
    source = source.replace('source_run_physics_live_support_tu15',
                            'source_run_sandbox_special_tu15')
    source = source.replace('source_physics_live_support_tu15_owns_entry',
                            'source_sandbox_special_tu15_owns_entry')
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report['generated_cpp_sha256'] = sha(args.output)
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')


if __name__ == '__main__':
    main()
