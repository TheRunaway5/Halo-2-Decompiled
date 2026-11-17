#!/usr/bin/env python3
"""Compare the real C9070 World routes, including separately counted inherited work."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from validate_native_node_function_progress_source_tu15 import PIN, Xbe, corpus as progress_corpus

COAGULATION = '1de37727dbb9c51aa08e03bfea86c0641ad67250dba28c5dcb3ba3dadaaf93fa'
REFERENCE = 'HALO2_REFERENCE_OBJECT_FUNCTION_QUERY'
PRIMARY = 'src/physics/physics_live_support_tu15_generated.cpp'
INHERITED_ORIGINAL_PACKET = '9a675c8f010fb1394378a1154e430e1296eb719a760a870b4f696ad21e1d3db1'
ENTRY = 0xc9070
RETURN = 0x63000000
CALLS = {0xc9101: (0xd3750, 0xc9106), 0xc9108: (0xd3950, 0xc910d),
         0xc9186: (0xd3950, 0xc918b), 0xc92a8: (0x1046c0, 0xc92ad)}
UNRESOLVED = (0x126200, 0xfc510, 0x107140, 0x165590, 0x1655f0)
REGIONS = ((0x46a000, 4096), (0x4ec000, 4096), (0x4f0000, 8192), (0x51d000, 8192),
           (0x61000000, 65536), (0x62000000, 65536))


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def corpus(image):
    original_rows=progress_corpus(image)
    cases=[]
    profiles=(
        ('elapsed-ratio',0x5000581,4),('direct-progress',0x90006aa,4),('complement-progress',0xb0006ab,4),
        ('field-2c4',0x6000087,6),('field-208',0xe00059f,4),('byte-243',0xd00059d,5),
        ('field-1b8',0xf0006bf,5),('field-2b0',0x11000088,5),('field-25c',0x1100059b,5),
        ('field-260',0x1100059c,5),('flag-10a-134',0x90005a1,4),('sentinel-248',0x140005a2,4),
        ('sentinel-24c',0x140005a3,3),('field-264',0x160005a0,5),('byte-1f8',0x1800059e,5),
        ('unknown-low',0x00aa0101,2),('unknown-high',0x7fff0001,2),('unknown-signed',0xffffffff,2),
        ('linked-byte',0xb0005a9,5),('linked-records',0x130005a8,6),
    )
    scalars=(0x3e800000,0xbf800000,0x40400000,0x7fc01234,0,0x80000000)
    for family,key,count in profiles:
        for variant in range(count):
            number=len(cases)
            seed=variant%2 if 'progress' in family else number%16
            row={k:v for k,v in original_rows[seed].items() if k!='banks'}
            banks={bank['address']:bytearray.fromhex(bank['before']) for bank in original_rows[seed]['banks']}
            banks[0x46a000]=bytearray(image.read(0x46a000,4096))
            def location(address,width):
                for base,bank in banks.items():
                    if base<=address<=base+len(bank)-width:return bank,address-base
                raise ValueError(f'Private C9070 fixture address escaped {address:x}/{width}')
            def get(address,width=4):
                bank,offset=location(address,width)
                return int.from_bytes(bank[offset:offset+width],'little')
            def put(address,value,width=4):
                bank,offset=location(address,width)
                bank[offset:offset+width]=(value&((1<<(8*width))-1)).to_bytes(width,'little')
            datum=(0,1,17,65535)[seed%4]
            pool=get(0x4ecbd0);slots=get(pool+0x44)
            body=get((slots+datum*12+8)&0xffffffff)
            output,enabled=0x61002e00,0x61002e20
            stack=row['gp'][4]
            if variant==4 and family.startswith(('field','byte')):output=enabled
            if variant==4 and family=='linked-byte':output=stack+12
            for n,value in enumerate((RETURN,datum,key,output,enabled)):put(stack+n*4,value)
            for field in (0x2c4,0x208,0x1b8,0x2b0,0x25c,0x260,0x264):put(body+field,scalars[variant%len(scalars)])
            for field in (0x243,0x1f8,0x175):put(body+field,(0,1,127,255,17,253)[variant%6],1)
            put(body+0x10a,4 if variant==1 else 0,1)
            put(body+0x134,0x40000 if variant==2 else 2 if variant==3 else 0)
            put(body+0x248,0 if variant==1 else 0xffffffff)
            put(body+0x24c,0xffffffff if variant==0 else 0)
            if family=='elapsed-ratio':
                put(body,2);put(body+0x334,(0x3f800000,0,0x40000000,0x3f000000)[variant]);
                put(body+0x338,(0,0,0x3f800000,0x7fc0a55a)[variant]);
                definition=0x61003000
                put(get(0x4f0414)+2*16+8,definition)
                put(definition+0x1d4,0x40000000);put(definition+0x1dc,0x40800000)
            if family.startswith('linked-'):
                linked_index=-1 if variant==0 else -3 if variant==3 else 0
                put(body+0x212,linked_index,1)
                if linked_index!=-1:put(body+0x218+linked_index*4,0xffffffff if variant==1 else datum)
                put(body,2)
                definition,records=0x61003000,0x61003200
                put(get(0x4f0414)+2*16+8,definition)
                put(definition+0x2d0,0);put(definition+0x2c8,1 if variant==5 else 0)
                put(definition+0x2cc,records);put(records+0x1c,0x3f800000)
            row.update(id=number,name=f'{family}-{variant}',entry=ENTRY,matrix=datum,source=key,max_steps=4000,
                       stop_before_entries=list(UNRESOLVED),banks=[dict(address=base,before=banks[base].hex()) for base,_ in REGIONS])
            cases.append(row)
    return cases

def verify_calls(image):
    for site, (target, returned) in CALLS.items():
        opcode = image.read(site, 5)
        if opcode[0] != 0xe8 or returned + int.from_bytes(opcode[1:], 'little', signed=True) != target:
            raise ValueError(f'Wrong original C9070 CALL at {site:x}')


def accounting(path, reference, complete, stopped, inherited, native_ratio=False):
    receipt = json.loads(path.read_bytes())
    if receipt['native_closure_proved'] or any(receipt[name] for name in (
        'omitted_records', 'active_scopes', 'registration_failures', 'orphan_literal_events'
    )):
        raise ValueError('Invalid C9070 execution accounting report')
    owners = {}
    for values in receipt['rows']:
        row = dict(zip(receipt['row_fields'], values, strict=True))
        if (receipt['units'][row['unit']][0] == PRIMARY and
            receipt['image_names'][row['image']] == 'tu15' and row['symbol'] in ('c9070', '1046c0', '107140')):
            if row['symbol'] in owners:
                raise ValueError('Ambiguous C9070 accounting owner ' + row['symbol'])
            owners[row['symbol']] = row
    parent = owners['c9070']
    total = complete + stopped
    if (parent['literal_sites'], parent['entries'], parent['returns'], parent['unwinds']) != (144, total, complete, stopped):
        raise ValueError(f'C9070 parent entry/return accounting differs: {parent}')
    if reference:
        if parent['body_entries'] != total or parent['literal_statements'] < total:
            raise ValueError(f'C9070 own-reference body did not execute: {parent}')
    elif any(parent[key] for key in ('body_entries', 'literal_statements')) or (
        parent['returns_without_body'], parent['unwinds_without_body']) != (complete, stopped):
        raise ValueError(f'C9070 default executed translated parent statements: {parent}')
    child = owners['1046c0']
    expected = 1 if inherited else 6 + stopped
    if (child['literal_sites'], child['entries'], child['returns'], child['unwinds']) != (
        406, expected, expected - stopped, stopped):
        raise ValueError(f'C9070 lost its separate 1046C0 owner accounting: {child}')
    if native_ratio:
        if child['body_entries'] or child['literal_statements'] or (
            child['returns_without_body'], child['unwinds_without_body']) != (expected - stopped, stopped):
            raise ValueError(f'C9070 executed the retained 1046C0 body by default: {child}')
    elif child['body_entries'] != expected or child['literal_statements'] < expected:
        raise ValueError(f'C9070 lost its separately retained 1046C0 accounting: {child}')
    descendant = owners.get('107140')
    if inherited and (not descendant or descendant['entries'] != 1 or descendant['returns'] != 1):
        raise ValueError(f'C9070 inherited path lost the genuine 107140 return: {descendant}')
    return {name: {key: row[key] for key in ('literal_sites', 'entries', 'body_entries', 'literal_statements', 'returns', 'unwinds')}
            for name, row in owners.items()}


def verify_observed(rows):
    calls = {site: 0 for site in CALLS}
    writes = 0
    for row in rows:
        for site, address, before, written in row['stores']:
            writes += 1
            if site in CALLS:
                if int.from_bytes(bytes.fromhex(written), 'little') != CALLS[site][1]:
                    raise ValueError(f'C9070 pushed the wrong return at {site:x}')
                calls[site] += 1
    if list(calls.values()) != [4, 4, 4, 7] or rows[-1]['stores'][-1][0] != 0x104815:
        raise ValueError(f'C9070 direct/inherited call sequence differs: {calls}')
    return {f'{site:x}': count for site, count in calls.items()}, writes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--xbe', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--accounting', action='store_true')
    parser.add_argument('--native-1046c0', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Use tools/run_test.py')
    if not args.xbe.is_file() or not args.map.is_file():
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    report = {'passed': False, 'owned_scratch': str(scratch), 'retained_large_artifacts': []}
    try:
        if digest(args.xbe) != PIN or digest(args.map) != COAGULATION:
            raise ValueError('C9070 requires the authenticated TU1.5 XBE and Coagulation map')
        report.update(xbe_sha256=PIN, map_sha256=COAGULATION, binary_sha256=digest(args.binary), routes={})
        image = Xbe(args.xbe)
        verify_calls(image)
        fixture = scratch / 'c9070-inputs.json'
        fixture.write_text(json.dumps(corpus(image), separators=(',', ':')))
        report['fixture_sha256'] = digest(fixture)
        base = {key: value for key, value in os.environ.items() if not key.startswith('HALO2_REFERENCE_')}
        base.pop('HALO2_NATIVE_EXECUTION_REPORT', None)
        outputs = {}
        for mode in ('observed', 'quiet', 'inherited'):
            complete = 1 if mode == 'inherited' else 84
            stopped = int(mode == 'observed')
            for reference in (False, True):
                label = mode + ('-reference' if reference else '-default')
                env = dict(base)
                if reference:
                    env[REFERENCE] = '1'
                counter = scratch / (label + '.json')
                if args.accounting:
                    env['HALO2_NATIVE_EXECUTION_REPORT'] = str(counter)
                command = [str(path.resolve()) for path in (args.binary, args.map, args.xbe, scratch / label, fixture)] + [mode]
                run = subprocess.run(command, env=env, capture_output=True, text=True, timeout=100)
                if run.returncode:
                    raise RuntimeError(label + ' failed: ' + run.stderr[-5000:])
                reply = json.loads(run.stdout)
                rows = reply.pop('rows')
                progress = 0 if mode == 'inherited' else 8
                if (reply['complete'], reply['stopped'], len(rows), reply['direct_refusals'], reply['older_image_refusals'],
                    reply['query_native_entries'], reply['progress_native_entries'], reply['descriptor_progress_native_entries']) != (
                        complete, stopped, complete + stopped, 2, 2, 0 if reference else len(rows), progress, progress):
                    raise ValueError(label + ' did not take the required C9070 source and child routes: ' + str(reply))
                if args.accounting:
                    reply['separate_owner_counters'] = accounting(counter, reference, complete, stopped, mode == 'inherited', args.native_1046c0)
                report['routes'][label] = reply
                if mode in outputs and outputs[mode] != rows:
                    raise ValueError(label + ' differs from the genuine World default')
                outputs[mode] = rows
        calls, writes = verify_observed(outputs['observed'])
        for observed, quiet in zip(outputs['observed'][:-1], outputs['quiet'], strict=True):
            if quiet['stores'] or any(quiet[key] != observed[key] for key in ('name', 'abi', 'changes')):
                raise ValueError('C9070 observer altered escaped World state for ' + quiet['name'])
        inherited, = outputs['inherited']
        inherited_stores = [site for site, *_ in inherited['stores']]
        if inherited_stores.count(0x104815) != 1 or inherited_stores.count(0xc92a8) != 1:
            raise ValueError('C9070 inherited path did not call both genuine owners')
        inherited_sha = hashlib.sha256(json.dumps(inherited, sort_keys=True).encode()).hexdigest()
        if inherited_sha != INHERITED_ORIGINAL_PACKET:
            raise ValueError('C9070 inherited escaped packet differs from original: ' + inherited_sha)
        report.update(passed=True, calls=calls, writes_per_observed_route=writes, inherited_complete=1,
                      inherited_result_sha256=inherited_sha,
                      retained_1046c0=not args.native_1046c0, whole_native_dependency_closure=False)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        args.report.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
