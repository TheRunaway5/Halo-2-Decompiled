#!/usr/bin/env python3
"""Compare the exact installed BCB70 family with original i386 and its retained source."""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys

import native_object_function_scalar_replacements as binding
import native_object_function_query_replacements as query
import native_object_type_query_replacements as type_query
from specialize_native_operands import specialize
from validate_native_tu15_prt_helpers import PIN, physical_original

ROOT = Path(__file__).resolve().parents[1]

ENTRY = 0xbcb70
RETURN = 0x63000000
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
BODY_PIN = '97f18741ab0d02659a55f24833a51870f8e5d1898b75e3622842b5479e382e52'
EDGES = {0xbcbbe: (0x10fb70, 0xbcbc3), 0xbcc0d: (0x10de40, 0xbcc12),
         0xbcc67: (0xbc8f0, 0xbcc6c), 0xbcc9f: (0xbcb70, 0xbcca4)}
ENTRIES = (ENTRY, 0x10fb70, 0x10de40, 0xbc8f0, 0xc9070)
UNRESOLVED = (0x13e670, 0xd3750, 0xd3950, 0x1046c0)
REGIONS = ((0x46a000, 4096), (0x474000, 4096), (0x4ec000, 4096), (0x4f0000, 8192),
           (0x51d000, 8192), (0x61000000, 65536), (0x62000000, 65536))


def digest(data):
    return hashlib.sha256(data.encode() if isinstance(data, str) else data).hexdigest()


def body(text, entry):
    return re.search(rf'^static void run_{entry:x}\(WorldState &s\) \{{.*?^\}}', text, re.M | re.S)[0]


def prepare(image):
    code = original._Code(image, '.text')
    listings = {entry: set(complete_body(code, entry).code) for entry in ENTRIES}
    sites = listings[ENTRY]
    assert len(sites) == 138
    assert {pc for pc in sites if code.at(pc).mnemonic == 'call'} == set(EDGES)
    for pc, (target, returned) in EDGES.items():
        call = code.at(pc)
        assert call.operands[0].imm == target and pc + call.size == returned
    authored = (ROOT/PRIMARY).read_text()
    source = binding.canonical_body(body(authored, ENTRY))
    assert digest(source) == BODY_PIN
    assert {int(pc, 16) for pc in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)', source)} == sites
    symbols = {pc: code.at(pc).operands[-1].imm & 0xffffffff for pc in (0xbcbd9, 0xbcbe5)}
    assert symbols == {0xbcbd9: 0x30005a6, 0xbcbe5: 0x40005a7}, symbols
    metadata = original.metadata

    def explicit_stack_reads(img):
        result = metadata(img)
        for instruction in result['sites']:
            if code.at(instruction['instruction']).mnemonic.startswith(('pop', 'ret')):
                instruction['operands'].append(dict(base=4, index=-1, scale=1, displacement=0,
                                                    bytes=4, read=True, write=False))
        return result

    original.metadata = explicit_stack_reads
    original.ENTRIES = ENTRIES
    return listings


def corpus(image):
    seeds = progress_corpus(image)
    profiles = (
        'zero', 'key-one', 'key-two', 'empty-rows', 'negative-rows', 'row-miss', 'invalid-datum',
        'cache-miss', 'cache-expired', 'cache-interpolation', 'type-c9070-hit',
        'row-real-child', 'real-recursion', 'invalid-inherited',
    )
    for number, (name, variant) in enumerate((name, variant) for name in profiles for variant in range(4)):
        seed = seeds[number % len(seeds)]
        row = {key: value for key, value in seed.items() if key != 'banks'}
        banks = {part['address']: bytearray.fromhex(part['before']) for part in seed['banks']}
        for address in (0x46a000, 0x474000):
            banks[address] = bytearray(image.read(address, 4096))

        def put(address, value, width=4):
            for base, data in banks.items():
                if base <= address <= base + len(data) - width:
                    at = address-base
                    data[at:at+width] = (value & ((1 << (width*8)) - 1)).to_bytes(width, 'little')
                    return
            raise ValueError(f'BCB70 fixture address escaped {address:x}/{width}')

        pool, slots = 0x61000100, 0x61000400
        target_body, inherited_body = 0x61000900, 0x61000e00
        desc, inherited_desc, method = 0x61001200, 0x61001400, 0x61001600
        tags, definition, records, record_type = 0x61001800, 0x61001a00, 0x61001c00, 0x61002000
        cache, timer, output = 0x61002200, 0x61002600, 0x61002e00
        sp = row['gp'][4]
        datum = 0xffffffff if name == 'invalid-datum' else 0
        if datum == 0xffffffff:
            slots -= 65535 * 12
        key = {'zero': 0, 'key-one': 0x30005a6, 'key-two': 0x40005a7, 'cache-miss': 0}.get(name, 0x6000087)
        for index, value in enumerate((RETURN, datum, key, output if variant != 3 else sp + 4)):
            put(sp+index*4, value)
        put(0x4ecbd0, pool)
        put(pool+0x44, slots)
        put(0x61000408, target_body)
        put(0x61000414, inherited_body)
        for address, kind in ((target_body, 0), (inherited_body, 1)):
            put(address, 2)
            put(address+4, 0)
            put(address+0x14, 0xffffffff)
            put(address+0xaa, kind, 1)
            put(address+0xc0, 0, 2)
            put(address+0x2c4, (0x3e800000, 0x3f400000, 0x3f000000, 0x3e000000)[variant])
        put(0x474b90, desc)
        put(0x474b94, inherited_desc)
        put(desc+0x84, method if name == 'type-c9070-hit' else 0)
        put(desc+0x88, 0)
        put(inherited_desc+0x84, method)
        put(inherited_desc+0x88, 0)
        put(method+0x4c, 0xc9070)
        put(0x4f0414, tags)
        put(tags+2*16+8, definition)
        put(definition+0x64, -1 if name == 'negative-rows' else 2 if name in ('row-miss', 'row-real-child') else 0)
        put(definition+0x68, records)
        for index in range(2):
            record = records+index*32
            put(record, 0)
            put(record+4, 0x7000abcd)
            put(record+8, key if name == 'row-real-child' and index == (variant & 1) else 0x51515151)
            put(record+0x18, record_type)
            put(record+0x1c, 0)
        put(record_type, 0, 1)
        put(0x51daf4, cache)
        put(0x51df54, timer)
        put(timer+8, 3 if name == 'cache-interpolation' else 8)
        for index in range(32):
            put(cache+0x280+index*4, 0xffffffff)
        if name.startswith('cache-') or name == 'row-real-child':
            put(target_body+0xc0, 0x200, 2)
            if name != 'cache-miss':
                index = (0, 3, 7, 31)[variant]
                put(cache+0x280+index*4, datum)
                entry = cache+index*20
                for offset, value in enumerate((0x7000abcd if name == 'row-real-child' else key,
                                                0x3e800000, 0x3f400000, 0, 6)):
                    put(entry+offset*4, value)
        if name in ('real-recursion','invalid-inherited'):
            put(target_body+4, 1<<26)
            put(target_body+0x14, 1 if name == 'real-recursion' else 0xffffffff)
        row.update(id=number, name=f'{name}-{variant}', entry=ENTRY, matrix=datum, source=key, max_steps=5000,
                   banks=[{'address': address, 'before': banks[address].hex()} for address, _ in REGIONS],
                   stop_before_entries=list(UNRESOLVED))
        if name == 'row-real-child' and variant == 3:
            # Keep the nested cache's index-store alias distinct from the caller's output argument.
            put(sp+12, output)
            row['banks'] = [{'address': address, 'before': banks[address].hex()} for address, _ in REGIONS]
        yield row

def extract(text, literal, scratch):
    label = 'literal' if literal else 'direct'
    template = 'true' if literal else 'false'
    helpers = re.findall(r'^std::uint32_t sign_extend[^\n]+\{.*?^\}', text, re.M|re.S)
    assert len(helpers)==1 and digest(helpers[0])=='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b'
    funcs = {
        'retained_bcb70': binding.ordered_shifts(binding.canonical_body(body(text, ENTRY)), literal).replace(
            'run_bcb70(', 'retained_bcb70(', 1),
        'run_10fb70': body(text, 0x10fb70),
        'run_bc8f0': body(text, 0xbc8f0),
        'run_10de40': type_query.canonical_body(body(text, 0x10de40)),
        'run_c9070': query.canonical_body(body(text, 0xc9070)),
    }
    scope = 'namespace '+label+'_source {\n' 
    scope += 'inline thread_local unsigned native{};\n'
    scope += '\n'.join(f'static void {name}(WorldState& state);' for name in (*funcs, 'run_bcb70'))+'\n'
    scope += '''void dispatch(WorldState& state,std::uint32_t target,std::uint32_t,bool tail) {
    if (tail || target!=0xc9070u) throw DescendantFrontier{};
    run_c9070(state);
}
void run_13e670(WorldState&) {throw DescendantFrontier{};}
void run_d3950(WorldState&) {throw DescendantFrontier{};}
void run_1046c0(WorldState&) {throw DescendantFrontier{};}
'''
    scope += helpers[0]+'\n'+'\n'.join(funcs.values())+'\n'
    scope += f'''static void run_bcb70(WorldState& state) {{
    if (!native) {{ retained_bcb70(state);return; }}
    const auto children = candidate::Children{{run_10fb70,run_10de40,run_bc8f0,run_bcb70}};
    if (native==2u) {{
        candidate::run_physical(state,children,[](auto& current,std::uint32_t pc) {{
            if(current.services.instruction)current.services.instruction(pc);
        }});
        return;
    }}
    candidate::run_source<{template},true>(state,children);
}}
}} // namespace {label}_source
'''
    (scratch/f'retained_{label}.inc').write_text(scope)
    return {name:digest(text) for name,text in funcs.items()}


def compact(row, actual):
    before = {part['address']: bytes.fromhex(part['before']) for part in row['banks']}
    after = {part['address']: bytes.fromhex(part['after']) for part in actual['banks']}
    changes = [[base+i,b] for base in sorted(before) for i,(a,b) in enumerate(zip(before[base],after[base],strict=True)) if a!=b]
    return dict(name=row['name'],abi=dict(gp=actual['boundary_gp'],eflags=actual['boundary_eflags'],
        xmm=actual['boundary_xmm'],fp=actual['after_fp'],live_x87=row['live_x87']),
        trace=[{field:packet[field] for field in ('entry','gp','eflags','xmm','fp','live_x87')} for packet in actual['calls']],
        writes=actual['writes'],reads=actual['reads'],changes=changes)

def fault_rows(normal):
    seeds=normal[:4]
    for mode in ('failed-first-push','failed-first-output'):
        for variant, seed in enumerate(seeds):
            row=dict(seed)
            row['gp']=list(seed['gp'])
            row['banks']=[dict(bank)for bank in seed['banks']]
            row['name']=mode+'-'+str(variant)
            row['id']=len(normal)+variant+(0 if mode=='failed-first-push' else 4)
            row['actual_write_fault']=True
            if mode=='failed-first-push':
                row['gp'][4]=0x62000000
                stack=next(bank for bank in row['banks'] if bank['address']==0x62000000)
                setup=b''.join(value.to_bytes(4,'little') for value in (RETURN,row['matrix'],row['source']))
                stack['before']=setup.hex()+stack['before'][24:]
            else:
                pointer=row['gp'][4]+12
                stack=next(bank for bank in row['banks'] if bank['address']==0x62000000)
                bank=bytearray.fromhex(stack['before']);at=pointer-stack['address']
                bank[at:at+4]=(0x64000000).to_bytes(4,'little');stack['before']=bank.hex()
            yield row


def compact_fault(row, actual):
    before={part['address']:bytes.fromhex(part['before'])for part in row['banks']}
    after={part['address']:bytes.fromhex(part['after'])for part in actual['banks']}
    changes=[[base+n,b] for base in sorted(before) for n,(a,b) in enumerate(zip(before[base],after[base],strict=True))if a!=b]
    boundary=dict(actual['boundary']); assert boundary.pop('phase')=='actual_write_fault'
    assert boundary['instruction']==(ENTRY if 'push' in row['name'] else 0xbcb8e)
    assert actual['boundary_eflags'] & 0x10000, 'Original Linux signal stop omitted RF'
    return {'name':row['name'],'abi':dict(gp=actual['boundary_gp'],eflags=actual['boundary_eflags'] & ~0x10000,
        xmm=actual['boundary_xmm'],fp=actual['after_fp'],live_x87=row['live_x87']),
        'reads':actual['reads'],'writes':actual['writes'],'changes':changes,'attempt':boundary}

def compare_rows(expected, actual, kind):
    for left, right in zip(expected, actual, strict=True):
        if left == right:
            continue
        fields = [key for key in left if left[key] != right[key]]
        detail = {}
        for field in fields:
            a, b = left[field], right[field]
            if isinstance(a,list) and isinstance(b,list):
                at = next((index for index,(x,y) in enumerate(zip(a,b)) if x!=y),min(len(a),len(b)))
                detail[field] = (at,a[at] if at<len(a) else None,b[at] if at<len(b) else None)
            else:
                detail[field] = (a,b)
        raise ValueError(f'BCB70 {kind} mismatch on {left["name"]}: {detail}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if (sys.platform!='linux' or platform.machine().lower() not in ('x86_64','amd64','i386','i686') or
        Path(args.compiler).name.lower() in ('cl','cl.exe','clang-cl','clang-cl.exe') or not args.xbe.is_file()):
        return 77
    global original, complete_body, progress_corpus
    import oracle_native_campaign_screen_lifecycle_tu15 as original
    from generate_physics_live_support_tu15 import complete_body
    from validate_native_node_function_progress_source_tu15 import corpus as progress_corpus
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed':False, 'owned_scratch':str(scratch), 'retained_large_artifacts':[]}
    try:
        if digest(args.xbe.read_bytes())!=PIN:
            raise ValueError('BCB70 requires the authenticated English TU1.5 executable')
        image=original.Xbe(args.xbe)
        listings=prepare(image)
        normal=list(corpus(image)); fault=list(fault_rows(normal)); rows=normal+fault
        trace_sites=sorted(listings[ENTRY]|{0x10fb70,0x10de40,0xbc8f0,0xc9070,0x10de97,0xbc957,0xbc95c})
        observed,authority=physical_original(image,rows,scratch,entries=ENTRIES,regions=REGIONS,
            trace_calls=[dict(entry=pc,arguments=0,gp=True,xmm=True)for pc in trace_sites],trace_extended=True,
            preserve_x87_prefix=True)
        complete, stopped = observed['cases'][:len(normal)], observed['cases'][len(normal):]
        assert all(row['completed'] and row['stopped_at']==RETURN and row['live_x87_unchanged'] for row in complete)
        assert all(not row['completed'] and row['prefix_x87_preservation_checked'] for row in stopped)
        visits=set().union(*(set(row['executed_sites'])for row in complete))
        assert len(listings[ENTRY]&visits)==138
        starts,returns=Counter(),Counter()
        for row in complete:
            for index,packet in enumerate(row['calls']):
                site=packet['entry']
                if site not in EDGES:continue
                target,returned=EDGES[site];callee=row['calls'][index+1]
                assert callee['entry']==target and callee['gp'][4]==packet['gp'][4]-4
                assert any(write['instruction']==site and write['address']==callee['gp'][4] and
                           int.from_bytes(bytes.fromhex(write['written']),'little')==returned for write in row['writes'])
                starts[hex(site)]+=1
                returns[hex(site)]+=any(resume['entry']==returned for resume in row['calls'][index+1:])
        assert starts==returns=={'0xbcbbe':16,'0xbcc0d':32,'0xbcc67':4,'0xbcc9f':4}
        original_packets=[compact(row,record)for row,record in zip(normal,complete,strict=True)]
        fault_packets=[compact_fault(row,record)for row,record in zip(fault,stopped,strict=True)]
        (scratch/'trace-sites.json').write_text(json.dumps(trace_sites))
        raw=(ROOT/PRIMARY).read_text(); prepared,receipt=specialize(raw)
        assert binding.installed_entries(raw)==binding.installed_entries(prepared)==['bcb70']
        assert receipt['native_object_function_scalar_entries']==['bcb70']
        report['source_spelling_pins']={'raw':extract(raw,False,scratch),'prepared':extract(prepared,True,scratch)}
        binary=scratch/'bcb70'; output=scratch/'result.json'
        flags=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if args.sanitizers else []
        command=[args.compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math',*flags,
            '-I'+str(scratch),*('-I'+str(ROOT/tree)for tree in ('src','include','tests')),
            str(ROOT/'tests/native_tu15_object_function_scalar_tests.cpp'),
            str(ROOT/'src/rendering/effects/source_effect_compiled_symbols_tu15.cpp'),'-o',str(binary)]
        subprocess.run(command,check=True,timeout=360)
        execution=subprocess.run([str(binary),str(scratch/'physical-original/inputs.json'),str(scratch/'trace-sites.json'),str(output)],
            check=True,capture_output=True,text=True,timeout=360)
        actual=json.loads(output.read_bytes())
        for name in ('physical','source'):
            compare_rows(original_packets,actual[name],name)
        compare_rows(fault_packets,[row['physical']for row in actual['faults']],'actual write fault')
        for expected,pair in zip(fault_packets,actual['faults'],strict=True):
            transport=pair['source']
            if 'push' in expected['name']:
                assert transport['abi']['gp'][4]==expected['abi']['gp'][4]-4
                transport['abi']['gp'][4]+=4
            compare_rows([expected],[transport],'retained failed-write publication')
        assert (ROOT/PRIMARY).read_text()==raw
        report.update(json.loads(execution.stdout), authority=authority, original_xbe_sha256=PIN,
            actual_parent_sites=138, original_complete_rows=len(complete), genuine_child_returns=returns,
            actual_fault_rows=len(stopped), observed_hardware_rf_mask=0x10000,
            original_compact_packet_sha256=digest(json.dumps(original_packets,sort_keys=True)),
            compiler=args.compiler, sanitizers=args.sanitizers)
    except subprocess.CalledProcessError as error:
        report['error']=str(error)
        if error.stderr:
            print(error.stderr[-8000:],file=sys.stderr)
        raise
    except Exception as error:
        report['error']=str(error)
        raise
    finally:
        args.report.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
