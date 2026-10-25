#!/usr/bin/env python3
"""Lower authenticated complete TU1.5 shadow frame and owner bodies."""
import argparse
import hashlib
import json
from pathlib import Path
from types import SimpleNamespace
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

# Located by the complete earlier/TU1.5 CFG audit. Generation reads actual
# TU1.5 bytes; external renderer/cache/observer services remain required owners.
ENTRIES = (0x226b0,0x30c40,0x31f40,0x320a0,0x32340,0x333f0,0x335b0,0x335d0,
           0x44710,0x45a70,0x3d7b0,0x3d7e0,0x3dc80,0x3e9b0,0x48ed0,0x4a1d0,0x4a7b0,0x4ac20,
           0x4bae0,0x4b150,0xbbd10,0xc3dc0,0x168900)

# Complete ordinary main40E80 callees verified against the unchanged original
# on a published main-queue bank. 41630 is a tail target (40F2C/40F43), so the
# CALL-only trace collector cannot discover its required complete body.
ENTRIES += (0x1e2b0,0x40e00,0x41630,0x42090,0x42750,0xc0700,0x179a40,0x17cd10)

NATIVE_PREFIXES = {
    0x2e66b2: dict(resume=0x2e6714,code_sha256='2a88f4b134df82c047af7903b64d1f23f410ec54e0c532aacdf7a5f072eac29b',instructions=(
        (0x2e66b2,'0f284110'),
        (0x2e66b6,'0f2809'),
        (0x2e66b9,'0f2879e0'),
        (0x2e66bd,'0f59ca'),
        (0x2e66c0,'0f58c8'),
        (0x2e66c3,'0f28c1'),
        (0x2e66c6,'0f2849f0'),
        (0x2e66ca,'0f59fc'),
        (0x2e66cd,'0f59cb'),
        (0x2e66d0,'0f58f9'),
        (0x2e66d3,'0f28cf'),
        (0x2e66d6,'0f28b900feffff'),
        (0x2e66dd,'0f58c8'),
        (0x2e66e0,'0f288110feffff'),
        (0x2e66e7,'0f59fd'),
        (0x2e66ea,'0f282a'),
        (0x2e66ed,'0f596c2440'),
        (0x2e66f2,'0f58f8'),
        (0x2e66f5,'0f2881f0fdffff'),
        (0x2e66fc,'0f59c6'),
        (0x2e66ff,'0f58e8'),
        (0x2e6702,'0f28c5'),
        (0x2e6705,'0f58c7'),
        (0x2e6708,'0f28e8'),
        (0x2e670b,'0f5ce9'),
        (0x2e670e,'0f50c5'),
        (0x2e6711,'83f80f'),
    )),
    0x2e6810: dict(resume=0x2e6872,code_sha256='ecfad8bb49c01f07e6e3fa285e305a9a93d5b5cbcdb73efc3bf68adefea25e48',instructions=(
        (0x2e6810,'0f284110'),
        (0x2e6814,'0f2809'),
        (0x2e6817,'0f2879e0'),
        (0x2e681b,'0f59ca'),
        (0x2e681e,'0f58c8'),
        (0x2e6821,'0f28c1'),
        (0x2e6824,'0f2849f0'),
        (0x2e6828,'0f59fc'),
        (0x2e682b,'0f59cb'),
        (0x2e682e,'0f58f9'),
        (0x2e6831,'0f28cf'),
        (0x2e6834,'0f28b900feffff'),
        (0x2e683b,'0f58c8'),
        (0x2e683e,'0f288110feffff'),
        (0x2e6845,'0f59fd'),
        (0x2e6848,'0f282a'),
        (0x2e684b,'0f596c2420'),
        (0x2e6850,'0f58f8'),
        (0x2e6853,'0f2881f0fdffff'),
        (0x2e685a,'0f59c6'),
        (0x2e685d,'0f58e8'),
        (0x2e6860,'0f28c5'),
        (0x2e6863,'0f58c7'),
        (0x2e6866,'0f28e8'),
        (0x2e6869,'0f5ce9'),
        (0x2e686c,'0f50dd'),
        (0x2e686f,'83fb0f'),
    )),
    0x2e6a50: dict(resume=0x2e6ab2,code_sha256='ecb72c582d2a7bd0e6e0669da3dff5c92d7296fc56a42ce21be762a27810b251',instructions=(
        (0x2e6a50,'0f284010'),
        (0x2e6a54,'0f2808'),
        (0x2e6a57,'0f2878e0'),
        (0x2e6a5b,'0f59ca'),
        (0x2e6a5e,'0f58c8'),
        (0x2e6a61,'0f28c1'),
        (0x2e6a64,'0f2848f0'),
        (0x2e6a68,'0f59fc'),
        (0x2e6a6b,'0f59cb'),
        (0x2e6a6e,'0f58f9'),
        (0x2e6a71,'0f28cf'),
        (0x2e6a74,'0f28b800feffff'),
        (0x2e6a7b,'0f58c8'),
        (0x2e6a7e,'0f288010feffff'),
        (0x2e6a85,'0f59fd'),
        (0x2e6a88,'0f282a'),
        (0x2e6a8b,'0f596c2420'),
        (0x2e6a90,'0f58f8'),
        (0x2e6a93,'0f2880f0fdffff'),
        (0x2e6a9a,'0f59c6'),
        (0x2e6a9d,'0f58e8'),
        (0x2e6aa0,'0f28c5'),
        (0x2e6aa3,'0f58c7'),
        (0x2e6aa6,'0f28e8'),
        (0x2e6aa9,'0f5ce9'),
        (0x2e6aac,'0f50d5'),
        (0x2e6aaf,'83fa0f'),
    )),
}


def validate_native_prefixes(image,code):
    # Exact original PCs and bytes bind every operand, not only mnemonics.
    for entry,spec in NATIVE_PREFIXES.items():
        rows=spec['instructions'];assert len(rows)==27 and rows[0][0]==entry
        assert hashlib.sha256(b''.join(bytes.fromhex(raw) for _,raw in rows)).hexdigest()==spec['code_sha256']
        for (pc,raw),next_pc in zip(rows,(*[pc for pc,_ in rows[1:]],spec['resume'])):
            expected=bytes.fromhex(raw);instruction=code.at(pc)
            assert instruction.address==pc and instruction.size==len(expected) and pc+len(expected)==next_pc
            assert bytes(instruction.bytes)==expected and image.read(pc,len(expected))==expected


def native_prefix_guard(instruction):
    entry=instruction.address
    if entry in NATIVE_PREFIXES:
        return f'if(source_shadow_native_prefix_tu15_detail::run_{entry:x}(s))goto L{NATIVE_PREFIXES[entry]["resume"]:x};\n'
    if any(entry==spec['resume'] for spec in NATIVE_PREFIXES.values()):
        return f'L{entry:x}: '
    return ''



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--directory', type=Path, required=True)
    parser.add_argument('--output', type=Path,
                        default=Path('src/rendering/shadows/source_shadow_frame_tu15_generated.cpp'))
    parser.add_argument('--additional-trace',type=Path,action='append',default=[],
                        help='Authenticated original instruction trace; lower complete observed callees')
    parser.add_argument('--external-entry',type=lambda value:int(value,0),action='append',default=[],
                        help='Entry retained by an actual native platform owner, not lowered here')
    parser.add_argument('--additional-entry',type=lambda value:int(value,0),action='append',default=[],
                        help='Authenticated complete entry invoked directly by the native caller')
    args = parser.parse_args()
    assert sha(NEW_DEFAULT) == NEW_PIN
    image = Xbe(NEW_DEFAULT)
    code = _Code(image, '.text')
    # Ordinary44E80 marks selected subparts through this complete leaf.
    # Elongation reaches the call even though the earlier bounded bank did not.
    assert 0x4526f in _trace(code,0x44e80,set()).code
    assert code.at(0x4526f).mnemonic=='call' and code.at(0x4526f).operands[0].imm==0x45a70
    # The whole XBE is authenticated above. Bind the native seam to its exact
    # contiguous prefix; its branch and remaining loop stay lowered literally.
    validate_native_prefixes(image,code)
    entries=set(ENTRIES)|set(args.additional_entry)
    for trace_path in args.additional_trace:
        trace=json.loads(trace_path.read_text())
        for n,pc in enumerate(trace[:-1]):
            instruction=code.at(pc)
            if instruction.mnemonic=='call':
                operand=instruction.operands[0]
                if operand.type==3 and not operand.mem.base and not operand.mem.index:
                    imported=int.from_bytes(image.read(operand.mem.disp&0xffffffff,4),'little')
                    if 0x80000000<=imported<0x80001000:continue # Real external kernel boundary, absent from XBE trace.
                target=trace[n+1]
                if operand.type==2:
                    assert target==operand.imm&0xffffffff
                entries.add(target)
    entries=tuple(sorted(entries-set(args.external_entry)))
    sites, instructions, addresses, routines = {}, {}, {}, []
    for entry in entries:
        body = _trace(code, entry, set(entries))
        assert body.code and all(width in (1, 4) for _, width, _ in body.tables)
        sites[f'{entry:08x}'] = sorted(body.code)
        routines.append(dict(entry=entry,
            tables=[dict(disc=p, count=n) for p, width, n in body.tables if width == 4],
            byte_tables=[dict(address=p, width=width, count=n, raw_hex=image.read(p, width*n).hex())
                         for p, width, n in body.tables if width == 1]))
        for pc in sorted(body.code):
            instruction = code.at(pc)
            assert image.read(pc, instruction.size) == bytes(instruction.bytes)
            instructions[f'{pc:08x}'] = pc
            addresses[pc] = pc
            addresses[pc + instruction.size] = pc + instruction.size
            for operand in instruction.operands:
                if operand.type == 2:
                    addresses[operand.imm & 0xffffffff] = operand.imm & 0xffffffff
                elif operand.type == 3:
                    addresses[operand.mem.disp & 0xffffffff] = operand.mem.disp & 0xffffffff
    paired = dict(image='English TU1.5', original_sha256=NEW_PIN,
                  entries={f'{p:08x}': p for p in entries}, body_sites=sites,
                  instruction_map=instructions,
                  address_map={f'{p:08x}': q for p, q in addresses.items()}, routines=routines)
    args.directory.mkdir(parents=True, exist_ok=True)
    correspondence = args.directory / 'bodies.json'
    correspondence.write_text(json.dumps(paired, indent=2, sort_keys=True) + '\n')
    generate(SimpleNamespace(output=args.output, manifest=args.directory/'generation.json',
                             correspondence=correspondence, additional_entries=Path(__file__)), paired,
             instruction_guard=native_prefix_guard)
    source = args.output.read_text().replace('tools/generate_physics_live_support_tu15.py',
        'tools/generate_shadow_frame_tu15.py').replace('halo2/physics_live_support_tu15.hpp',
        'halo2/source_shadow_frame_tu15.hpp').replace('source_run_physics_live_support_tu15',
        'source_run_shadow_frame_tu15').replace('source_physics_live_support_tu15_owns_entry',
        'source_shadow_frame_tu15_owns_entry')
    source=source.replace('#include "physics/runtime/physics_world_runtime_internal.hpp"',
        '#include "physics/runtime/physics_world_runtime_internal.hpp"\n'
        '#include "rendering/shadows/source_shadow_native_prefix_tu15_internal.hpp"',1)
    prefix_owner_present=0x2e6290 in entries
    for entry,spec in NATIVE_PREFIXES.items():
        assert source.count(f'if(source_shadow_native_prefix_tu15_detail::run_{entry:x}(s))goto L{spec["resume"]:x};')==int(prefix_owner_present)
        assert source.count(f'L{spec["resume"]:x}: ')==int(prefix_owner_present)
    assert source.count('} // namespace halo2')==1
    source=source.replace('} // namespace halo2',
        'SourceShadowNativePrefixTu15Stats source_shadow_native_prefix_tu15_stats() noexcept {\n'
        '    return source_shadow_native_prefix_tu15_detail::stats;\n}\n'
        'void source_reset_shadow_native_prefix_tu15_stats() noexcept {\n'
        '    source_shadow_native_prefix_tu15_detail::stats={};\n}\n'
        '} // namespace halo2')
    args.output.write_text(source)
    report = json.loads((args.directory/'generation.json').read_text())
    report.update(generated_cpp_sha256=sha(args.output), module_generator_sha256=sha(Path(__file__)))
    if args.additional_trace:report['original_callee_trace_sha256']={str(p):sha(p)for p in args.additional_trace}
    report['actual_native_external_entries']=[f'{p:08x}'for p in args.external_entry]
    report['additional_direct_entries']=[f'{p:08x}'for p in args.additional_entry]
    report['native_read_prefixes']={f'{entry:08x}':dict(
        first=f'{entry:08x}',last=f"{spec['instructions'][-1][0]:08x}",resume=f"{spec['resume']:08x}",
        code_sha256=spec['code_sha256'],instruction_count=27,owner_present=prefix_owner_present,
        helper_sha256=sha(Path(__file__).resolve().parents[1]/
            'src/rendering/shadows/source_shadow_native_prefix_tu15_internal.hpp'),
        stats_header_sha256=sha(Path(__file__).resolve().parents[1]/
            'include/halo2/source_shadow_native_prefix_tu15.hpp'))for entry,spec in NATIVE_PREFIXES.items()}
    (args.directory/'generation.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print('Complete TU1.5 shadow frame owners', len(entries), 'instructions', len(instructions))


if __name__ == '__main__':
    main()
