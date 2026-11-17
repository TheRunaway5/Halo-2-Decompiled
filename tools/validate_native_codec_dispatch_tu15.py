#!/usr/bin/env python3
"""Qualify the private codec selectors to a genuine tail entry or actual RET."""
import argparse
import copy
import hashlib
import json
import os
import platform
import random
import re
import shlex
import subprocess
import sys
from pathlib import Path

from native_mopp_ray_dense_original import physical_original_dense
from native_sphere_frustum_mmx_original import physical_original_mmx
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

ROOT = Path(__file__).resolve().parents[1]
ENTRIES = (0x283570, 0x2840D0, 0x284140, 0x2841B0, 0x284220, 0x284290)
RETURNS = {entry: entry + (0x36 if entry == ENTRIES[0] else 0x6c) for entry in ENTRIES}
REGIONS = ((0x510000, 4096), (0x62000000, 4096))
STOP = 0x63000000
TEST = 'tests/native_tu15_codec_dispatch_tests.cpp'
FRAME = 'src/physics/runtime/native_tu15_codec_dispatch_frame.hpp'
KERNEL = 'include/halo2/native_tu15_codec_dispatch_kernel.hpp'
SOURCE_PINS = (
    ('src/physics/physics_live_support_tu15_generated.cpp', (
        (0x283570, '39feb7127c2d288a9d8e71ae1f48f3ba5d5300d024ac6f0ee0f6da831b7ab606'),
        (0x2840D0, '1920f49efbce373d198dde81aa65ed4b7e775cec15b2c8804bae78cb6205fff8'),
        (0x284140, '1f07b478ba972b2623e6ed20488c184e1d0a7d0594d60c78943472991ae5ce00'),
        (0x2841B0, '961953cac9c0acb706aba2d1d509d63d9762a562e2d4a90817a2675da6e7bb67'),
        (0x284220, 'e9e10a2a1395b679b0bfb42d19ddb42fb617982af780d8e1a9e7c877466252fe'),
        (0x284290, 'bb04dfa70038ede46852f3ff480638c0005b43399cb01ee28deeb2727f54b3ad'),
    )),
    ('src/campaign/ai/source_ai_frame_tu15_generated.cpp', (
        (0x283570, 'f82aecc6f885ae8261c8af3e57a7339fc0a6a4800ab79ea7c894deebfe0e074a'),
        (0x2840D0, '1e3f3053e7249ec1466b97d4b4142ad9de42a4ed7e1cfa955253fec2c88835a9'),
    )),
    ('src/gameplay/units/source_combat_action_tu15_generated.cpp', (
        (0x284290, 'f3f3c2f8b8223a7cb5dd307181b6814c23c11744c7ee4b8c785f54f00ac0fafb'),
    )),
)
DEFECTS = (
    ('wrong-format-target', FRAME, 'format_jumps[value],field_entries[value]', 'format_jumps[value],field_entries[(value+1u)%5u]'),
    ('wrong-wide-test', KERNEL, 'const bool wide=view.wide_nonzero();', 'const bool wide=!view.wide_nonzero();'),
    ('destroy-eax-upper', FRAME, 's_.gp[0]=(s_.gp[0]&0xffffff00u)|value;', 's_.gp[0]=value;'),
    ('wrong-low-test-width', FRAME, 's_.flags.logic(s_.gp[0]&0xffu,1);', 's_.flags.logic(s_.gp[0],4);'),
    ('skipped-real-ret', FRAME, '(void)Base::pop(instruction);return{instruction,0u};', 'return{instruction,0u};'),
)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def source_owners():
    source_parts = {}
    tails = {}
    for name, pins in SOURCE_PINS:
        source = (ROOT / name).read_text()
        resolver = re.findall(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}', source, re.S)
        if len(resolver) != 1:
            raise ValueError('Codec selector resolver cardinality changed')
        mapping = {int(a, 16): int(b, 16) for a, b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;', resolver[0])}
        for entry, pin in pins:
            matches = re.findall(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}', source, re.S)
            if len(matches) != 1 or hashlib.sha256(matches[0].encode()).hexdigest() != pin:
                raise ValueError(f'Complete codec selector body changed: {name} {entry:#x}')
            sites = [int(pc, 16) for pc in re.findall(r's.services.instruction\(0x([0-9a-f]+)u\)', matches[0])]
            transfers = {}
            for line in matches[0].splitlines()[1:]:
                route = re.search(r'(?:run_([0-9a-f]+)\(s\);return;|dispatch\(s,(?:selected\()?0x([0-9a-f]+)u.*?true\);return;)', line)
                if not route:
                    continue
                target = int(route[1] or route[2], 16)
                if 'dispatch(s,selected(' in line and mapping.get(target) != target:
                    raise ValueError('Codec selector target is no longer TU1.5 identity')
                pc = int(re.search(r's.services.instruction\(0x([0-9a-f]+)u\)', line)[1], 16)
                transfers[pc] = target
            if entry in tails and tails[entry] != transfers:
                raise ValueError('Codec selector source copy tail identity differs')
            tails[entry] = transfers
            source_parts[(name, entry)] = (pin, sites)
    return source_parts, tails


def cases(tails, smoke):
    rng = random.Random(0x283570)
    live = tuple((0x8000000000000000 | rng.getrandbits(60)).to_bytes(8, 'little').hex() + 'ff3f' for _ in range(8))
    rows = []

    def make(entry, value, kind=0, wide=0, byte=0, compose=None):
        number = len(rows)
        banks = {base: bytearray(rng.randbytes(size)) for base, size in REGIONS}
        alias = number % 8
        sp = (0x510f28, 0x510f2c, 0x510f54, 0x510f78)[alias] if alias < 4 else 0x62000100 + number % 16

        def put(address, value, width=4):
            for base, data in banks.items():
                if base <= address <= base + len(data) - width:
                    data[address-base:address-base+width] = value.to_bytes(width, 'little')
                    return
            raise ValueError('Codec selector fixture escaped backing')

        put(0x510f30, value)
        put(0x510f34, kind)
        put(0x510f5c, wide)
        put(0x510f80, byte, 1)
        # The observer publishes the actual caller return+two adjacent words.
        # Keep genuine selector values when those caller slots alias globals.
        offset = sp + 8
        stacked = {0x510f30: value, 0x510f34: kind, 0x510f5c: wide, 0x510f80: byte}
        matrix, source = stacked.get(sp+4, 0), stacked.get(offset, 0)
        put(sp, STOP)
        put(sp+4, matrix)
        put(sp+8, source)
        gp = [rng.getrandbits(32) for _ in range(8)]
        gp[4] = sp
        profile = number // 2
        cw = (0x7f,0x27f,0x37f)[profile%3] | ((profile%4)<<10)
        # Vary masked exception bits and condition codes without inventing an
        # active unmasked exception summary in this declared input domain.
        sw = ((profile%8)<<11) | ((profile<<7)&0x4700) | (profile&0x003f)
        mxcsr = 0x1f80 | ((number%4)<<13) | (number&0x3f)
        depth = (0,1,4,8)[profile%4]
        boundaries = sorted(set(tails[entry].values()) if compose is None else set(tails[compose].values()))
        row = dict(id=number, name=f'codec-dispatch-{entry:x}-{number}', entry=entry, gp=gp, matrix=matrix, source=source,
                   before_fp=[cw, sw, mxcsr], eflags=0x202 | (rng.getrandbits(12)&0xcd5),
                   xmm=[[rng.getrandbits(32) for _ in range(4)] for _ in range(8)], live_x87=live[:depth],
                   max_steps=100, banks=[dict(address=base, before=bank.hex()) for base, bank in banks.items()],
                   stop_before_entries=boundaries)
        if compose is not None:
            row['compose'] = compose
        rows.append(row)

    variants = ((0,0),(0,1),(0x80340000,0),(0x80340000,0x80))
    for repeat in range(1 if smoke else 6):
        for value in (0,1,2,3,4,5,0xffffffff):
            make(ENTRIES[0], value)
        for value, entry in enumerate(ENTRIES[1:]):
            for kind in (0,1,2,3,0xffffffff):
                for wide, byte in variants if kind < 2 else ((0,0),):
                    make(entry, value, kind, wide, byte)
                    make(ENTRIES[0], value, kind, wide, byte, compose=entry)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--xbe', type=Path, default=NEW_DEFAULT)
    for option in ('smoke','prefixes','mmx','sanitizers','semantic-mutants'):
        parser.add_argument('--'+option, action='store_true')
    args = parser.parse_args()
    if 'HALO2_TEST_SCRATCH' not in os.environ:
        parser.error('Use tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64','amd64'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    args.output.mkdir(parents=True, exist_ok=False)
    before = {path.resolve(): sha(path) for folder in ('tools','include','src','tests')
              for path in (ROOT / folder).rglob('*') if path.is_file() and path.suffix in ('.py','.hpp','.h','.cpp','.inc')}
    used = {ROOT/TEST, ROOT/FRAME, ROOT/KERNEL, Path(__file__).resolve(), ROOT/'tools/observe_native_campaign_ai_vector_math_tu15.cpp'}
    report = dict(passed=False, private_unbound=True, genuine_tail_entry_only=True, no_supplied_child_outcomes=True,
                  readonly_gp_xmm_before_provider_domain=True, owned_scratch=str(scratch), retained_large_artifacts=[], compilers=[])
    try:
        source, tails = source_owners()
        report['source_owner_pins']={f'{name}:{entry:x}': pin for (name,entry),(pin,_) in source.items()}
        if sha(args.xbe) != NEW_PIN:
            raise ValueError('Expected unchanged English TU1.5 XBE')
        image = Xbe(args.xbe)
        code = _Code(image, '.text')
        targets = {target for transfers in tails.values() for target in transfers.values()}
        starts = targets | set(ENTRIES)
        graphs = {entry: _trace(code, entry, starts) for entry in ENTRIES}
        if len(targets) != 50 or sum(len(graph.code) for graph in graphs.values()) != 182:
            raise ValueError('Original codec selector instruction set or tail count changed')
        for (_,entry),(_,sites) in source.items():
            if sorted(graphs[entry].code) != sites:
                raise ValueError('Codec selector original/source site set differs')
        for entry in ENTRIES:
            actual = {pc: int(code.at(pc).op_str,16) for pc in graphs[entry].code if code.at(pc).mnemonic=='jmp'}
            if actual != tails[entry] or code.at(RETURNS[entry]).mnemonic != 'ret':
                raise ValueError('Codec selector original/source tail or RET identity differs')
        import oracle_native_campaign_screen_lifecycle_tu15 as common
        base = common.metadata
        common.ENTRIES = ENTRIES
        common.complete_body = lambda _,entry: graphs[entry]

        def metadata(ref):
            result = base(ref)
            for site in result['sites']:
                if site['instruction'] in RETURNS.values():
                    site['operands'].append(dict(base=4,index=-1,scale=1,displacement=0,bytes=4,read=True,write=False))
            return result

        common.metadata = metadata
        io = [site['instruction'] for site in metadata(image)['sites'] if site['operands']]
        jump_sites = {pc: target for transfer in tails.values() for pc,target in transfer.items()}
        sites_by_target = {target: pc for pc,target in jump_sites.items()}
        trace = [dict(entry=pc,gp=True,xmm=True) for pc in sorted(set(io)|set(jump_sites))]
        rows = cases(tails,args.smoke)
        normal_count = len(rows)
        proofs = []

        def measure(values,label):
            where = scratch/label
            where.mkdir()
            opaque = 'active_mmx' in values[0]
            runner = physical_original_mmx if opaque else physical_original_dense
            refs, proof = runner(image,values,where,entries=ENTRIES,regions=REGIONS+(((0x64000000,4096),)if opaque else()),
                                 trace_calls=trace,trace_extended=True)
            proofs.append(proof)
            return refs['cases']

        observed = measure(rows,'normal')
        if args.mmx:
            rng = random.Random(0x2840D0)
            mmx = []
            for mask in range(16 if args.smoke else 256):
                row = copy.deepcopy(rows[mask%normal_count]);row.update(id=len(rows)+len(mmx),name=row['name']+f'-mmx-{mask}',live_x87=[])
                row['before_fp'][1] &= ~0x3800
                row['active_mmx'] = dict(known_mask=mask,physical=[
                    (rng.getrandbits(64).to_bytes(8,'little')+(0xffff,0x3fff,0,0x7fff)[(mask+n)%4].to_bytes(2,'little')).hex()for n in range(8)])
                mmx.append(row)
            rows += mmx
            observed += measure(mmx,'opaque')
        if args.prefixes:
            read_prefixes, jump_prefixes = [], []
            seen_reads, seen_jumps = set(), set()
            for row,result in zip(rows[:normal_count],observed[:normal_count],strict=True):
                for ordinal,event in enumerate(result['reads']):
                    pc = event['instruction']
                    if pc in seen_reads:
                        continue
                    seen_reads.add(pc)
                    prefix = copy.deepcopy(row)
                    prefix.update(id=len(rows)+len(read_prefixes),name=row['name']+f'-before-read-{pc:x}',fault=dict(phase='before_read',ordinal=ordinal))
                    read_prefixes.append(prefix)
                for pc in result['executed_sites']:
                    if pc not in jump_sites or pc in seen_jumps:
                        continue
                    seen_jumps.add(pc)
                    prefix = copy.deepcopy(row)
                    prefix.update(id=0,name=row['name']+f'-before-jmp-{pc:x}',jump_fault=pc,stop_before_entries=[pc])
                    if row.get('compose') is not None and pc in tails[ENTRIES[0]]:
                        prefix.pop('compose')
                    jump_prefixes.append(prefix)
            if seen_reads != set(io) or seen_jumps != set(jump_sites):
                raise ValueError('Missing actual codec selector read/jump prefix coverage')
            rows += read_prefixes
            observed += measure(read_prefixes,'read-prefixes')
            for n,row in enumerate(jump_prefixes,len(rows)):
                row['id']=n
            rows += jump_prefixes
            observed += measure(jump_prefixes,'jump-prefixes')
        site_coverage, tail_coverage = set(), set()
        for row,result in zip(rows,observed,strict=True):
            site_coverage.update(result['executed_sites'])
            if result['writes']:
                raise ValueError('Selector performed an unowned store')
            if 'fault' in row:
                if result['completed']:
                    raise ValueError('Codec selector read prefix completed unexpectedly')
                pc = result['boundary']['instruction']
            else:
                if not result['completed']:
                    raise ValueError('Original codec selector did not reach its declared boundary')
                pc = result['stopped_at']
                if 'jump_fault' in row:
                    if pc != row['jump_fault']:
                        raise ValueError('Original before-tail stop changed')
                else:
                    target = 0 if pc == STOP else pc
                    if target:
                        tail_coverage.add(target)
                    row.update(last_instruction=sites_by_target[target] if target else RETURNS[row.get('compose',row['entry'])],target=target)
            row['expected'] = dict(entry=pc,gp=result['boundary_gp'],eflags=result['boundary_eflags'],xmm=result['boundary_xmm'],
                                   fp=result['after_fp'],live_x87=result.get('boundary_live_x87',[]))
            if 'active_mmx' in result:
                row['expected']['active_mmx']=result['active_mmx']
            row.update(reads=result['reads'],calls=result['calls'])
            for call in row['calls']:
                if call.pop('arguments'):
                    raise ValueError('Unexpected selector trace argument')
            row['read_packets']=[call for call in row['calls'] if call['entry'] not in jump_sites]
            for bank, after in zip(row['banks'],result['banks'],strict=True):
                if bank['address'] != after['address']:
                    raise ValueError('Original selector bank identity changed')
                bank['after']=after['after']
        if tail_coverage != targets or site_coverage != {pc for graph in graphs.values() for pc in graph.code}:
            raise ValueError('Original codec selector site/tail coverage incomplete')
        fixture = scratch/'fixture.json'
        fixture.write_text(json.dumps(dict(cases=rows),separators=(',',':')))
        report.update(physical_paths=len(rows),normal_paths=normal_count,original_sites=len(site_coverage),
                      real_tail_targets=len(tail_coverage),readonly_original_source_copies=len(source),original=proofs,
                      composed_paths=sum('compose'in row for row in rows),fault_prefixes=sum('fault'in row or 'jump_fault'in row for row in rows))
        for compiler in args.compiler or ('g++','clang++'):
            binary = scratch/Path(compiler).name
            dep = binary.with_suffix('.d')
            command = [compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',
                       '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(ROOT/TEST),'-o',str(binary),'-MD','-MF',str(dep)]
            if args.sanitizers:
                command.extend(('-fsanitize=address,undefined','-fno-omit-frame-pointer'))
            built = subprocess.run(command,capture_output=True,text=True,timeout=480)
            if built.returncode:
                raise ValueError('Codec selector compile failed: '+built.stderr[-10000:])
            for value in shlex.split(dep.read_text().replace('\\\n',' ').split(':',1)[1]):
                path = Path(value).resolve()
                if path in before:
                    used.add(path)
            done = subprocess.run([str(binary),str(fixture)],capture_output=True,text=True,timeout=480)
            if done.returncode:
                raise ValueError('Original codec selector comparison failed: '+(done.stdout+done.stderr)[-10000:])
            row = dict(compiler=compiler,result=json.loads(done.stdout),compiled_defects=[])
            report['compilers'].append(row)
            if args.semantic_mutants:
                for name,relative,old,new in DEFECTS:
                    subject = (ROOT/relative).read_text()
                    if subject.count(old) != 1:
                        raise ValueError('Codec selector defect seam changed '+name)
                    overlay = scratch/(Path(compiler).name+'-'+name)
                    output = overlay/relative
                    output.parent.mkdir(parents=True)
                    output.write_text(subject.replace(old,new,1))
                    mutant = overlay/'probe'
                    changed = [command[0],'-I'+str(overlay/'include'),'-I'+str(overlay/'src'),*command[1:]]
                    changed[changed.index('-o')+1]=str(mutant)
                    changed[changed.index('-MF')+1]=str(overlay/'probe.d')
                    built = subprocess.run(changed,capture_output=True,text=True,timeout=480)
                    if built.returncode:
                        raise ValueError('Codec selector mutant failed to build '+name+' '+built.stderr[-8000:])
                    done = subprocess.run([str(mutant),str(fixture)],capture_output=True,text=True,timeout=480)
                    diagnostic=done.stdout+done.stderr
                    if not done.returncode or 'codec dispatch' not in diagnostic.lower():
                        raise ValueError('Codec selector compiled defect survived '+name+' '+diagnostic[-8000:])
                    row['compiled_defects'].append(dict(name=name,rejected=True,diagnostic=diagnostic[-7000:]))
        if source_owners() != (source,tails):
            raise ValueError('Exact source selector body or tail identity changed during proof')
        report['passed']=True
    except BaseException as error:
        report['error']=repr(error)
        raise
    finally:
        used.update(Path(module.__file__).resolve() for module in tuple(sys.modules.values())
                    if getattr(module,'__file__',None) and Path(module.__file__).resolve().is_relative_to(ROOT/'tools'))
        report['source_before']={str(path.relative_to(ROOT)):before[path] for path in sorted(used)}
        report['source_after']={name:sha(ROOT/name) for name in report['source_before']}
        if report['source_before'] != report['source_after']:
            report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed'] else 1


if __name__=='__main__':
    raise SystemExit(main())
