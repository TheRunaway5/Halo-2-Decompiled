#!/usr/bin/env python3
"""Compare production hand skinning with actual original GPU varyings.

Original packed attributes, weights and proven matrices are inputs. Compare
world basis rows and eye-minus-world-position outputs of the captured programs.
This does not test native animation, model import, complete shading or pixels.
Run via tools/run_test.py; all compiler/linker products stay in owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
from original_weapon_gpu_evidence import load_json

ROOT = Path(__file__).absolute().parents[1]


def sha(path):
    with Path(path).open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def words(values):
    return struct.unpack('<'+'I'*len(values), struct.pack('<'+'f'*len(values), *values))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--inspection', type=Path, required=True)
    parser.add_argument('--postvs', type=Path, required=True)
    parser.add_argument('--palette-proof', type=Path, required=True)
    parser.add_argument('--native-palettes', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--event', type=int, action='append', help='Restrict to a retained hand-basis draw')
    parser.add_argument('--max-vertices', type=int, help='Minimize a retained mismatch without changing source inputs')
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    output = args.output.absolute(); output.mkdir(parents=True, exist_ok=False)
    pins = {}

    def pin(path):
        pins[str(Path(path).absolute())] = sha(path)

    def blob(folder, record):
        path = folder / record['file']; pin(path); assert sha(path) == record['sha256']
        data = path.read_bytes(); assert len(data) == record['bytes']; return data

    inspection, post_folder = args.inspection.absolute(), args.postvs.absolute()
    post = load_json(post_folder / 'report.json', pins)
    proof = load_json(args.palette_proof, pins)
    native = load_json(args.native_palettes, pins)
    gpu = load_json(inspection / 'report.json', pins)
    assert post['passed'] and proof['passed'] and native['passed']
    assert post['capture_sha256'] == gpu['capture_sha256']
    assert post['geometry_sha256'] == proof['original_geometry_owner_comparison_sha256']
    for report in (proof, native):
        for name, digest in report['linked_files_sha256'].items():
            assert sha(name) == digest, name
            pins[name] = digest
    data_path = args.native_palettes.parent / 'native-output.bin'; pin(data_path)
    data = data_path.read_bytes(); offset = 8; global_palettes = {}
    for case in native['cases']:
        ordinal, model, base, region_count, count = struct.unpack_from('<5I', data, offset); offset += 20
        ranges = [struct.unpack_from('<II', data, offset+i*8) for i in range(region_count)]; offset += region_count*8
        raw = data[offset:offset+count*48]; offset += len(raw)
        assert ordinal == case['id'] and model == case['model']
        bank = {0:raw[:48]}
        for selection, (first, size) in zip(case['selections'], ranges):
            assert len(selection['node_map']) == size
            for local, node in enumerate(selection['node_map']):
                matrix = raw[(first+local)*48:(first+local+1)*48]
                if node in bank: assert bank[node] == matrix
                bank[node] = matrix
        assert set(bank) == set(range(case['node_count']))
        global_palettes[ordinal] = b''.join(bank[i] for i in range(case['node_count']))
    assert offset == len(data)
    selected_events = args.event or (55159,55250)
    assert set(selected_events).issubset({55159,55250})
    assert args.max_vertices is None or args.max_vertices > 0
    fixture = bytearray(struct.pack('<II', 0x534b4e31, len(selected_events)))
    cases = []
    for event in selected_events:
        observed = load_json(inspection / f'event-{event}.json', pins)
        draw = next(d for d in post['draws'] if d['event']==event)
        palette_match = next(d for d in proof['draws'] if d['event']==event)
        palette_id = palette_match['matching_original_CPU_and_native_palette']['case']
        palette_case = native['cases'][palette_id]
        assert draw['model'] == palette_case['model'] == 0xe9e90864
        program_path = inspection / observed['stages']['0']['source']['file']; pin(program_path)
        program = program_path.read_text()
        for axis, channel in enumerate('xyz'):
            for component, raw_attribute in zip('xyz', (6,5,4)):
                assert f'DP3(oT{axis+1},{component}, v{raw_attribute}.xyz, R{axis+4}.xyz);' in program
            assert f'ADD(oT{axis+1},w, c[7].{channel}, -R0.{channel});' in program
        constants = next(v for b in observed['stages']['0']['blocks'] for v in b['values'] if v['name']=='c')['members']
        attributes = {a['name']:a for a in observed['vertex_inputs'] if a['used'] and not a['genericEnabled']}
        buffers = {b['slot']:(b,blob(inspection,b['data'])) for b in observed['vertex_data'] if any(a['vertexBuffer']==b['slot'] for a in attributes.values())}

        def attribute(name, index, fmt):
            a = attributes[name]; b, raw = buffers[a['vertexBuffer']]
            return struct.unpack_from(fmt, raw, index*b['binding']['byteStride']+a['byteOffset'])

        for name in ('v1','v2'):
            fmt = attributes[name]['format']; assert (fmt['compByteWidth'],fmt['compType'],fmt['type']) == (1,2,0)
        active = attributes['v1']['format']['compCount']; assert active in (2,3)
        assert attributes['v2']['format']['compCount'] == active
        raw_indices = blob(inspection, observed['indices']); stride = observed['index_buffer']['byteStride']; assert stride in (2,4)
        indices = struct.unpack('<'+('H' if stride==2 else 'I')*(len(raw_indices)//stride), raw_indices)
        post_indices = blob(post_folder, draw['indices']); assert draw['mesh']['indexByteStride'] == 4
        assert tuple(indices) == struct.unpack('<'+'I'*(len(post_indices)//4), post_indices)
        vertices = sorted(set(indices))
        if args.max_vertices: vertices = vertices[:args.max_vertices]
        post_bytes = blob(post_folder, draw['vertices'])
        offsets = {}; at = 0
        for signature in draw['outputs']:
            assert signature['varType'] == 0
            offsets[signature['varName']] = at; at += signature['compCount']*4
        assert at == draw['mesh']['vertexByteStride']
        region = next(s for s in palette_case['selections'] if s['section']==draw['section'])
        fixture.extend(struct.pack('<III', event,palette_case['node_count'],len(vertices)))
        fixture.extend(global_palettes[palette_id])
        expected = []
        for vertex in vertices:
            raw_position = attribute('v0', vertex, '<3h')
            position = [f32(f32(max(-1.,f32(raw_position[i]/32767.))*constants[170]['f32'][i])+constants[171]['f32'][i]) for i in range(3)]
            basis = []
            for name in ('v4_cmp','v5_cmp','v6_cmp'):
                packed = attribute(name,vertex,'<I')[0]
                values = []
                for shift, width in ((0,11),(11,11),(22,10)):
                    value = (packed>>shift)&((1<<width)-1)
                    if value & (1<<(width-1)): value -= 1<<width
                    values.append(f32(value/((1<<(width-1))-1)))
                basis.extend(values)
            local_bones = attribute('v1',vertex,'<'+'B'*active)
            bones = [region['node_map'][n] for n in local_bones]+[0]*(4-active)
            stored = attribute('v2',vertex,'<'+'B'*active)+(0,)*(4-active)
            fixture.extend(struct.pack('<6I4B12f', vertex,active,*bones,*stored,*position,*basis))
            texture_rows = [struct.unpack_from('<4f', post_bytes, vertex*at+offsets[f'v_vtxT{i}']) for i in (1,2,3)]
            reference = [row[i] for i in (2,1,0) for row in texture_rows]+[row[3] for row in texture_rows]
            expected.append(dict(vertex=vertex, original_words=words(reference), stored_weights=list(stored)))
        cases.append(dict(event=event, section=draw['section'], palette_case=palette_id, vertices=expected,
                          eye=constants[7]['f32'][:3], active_influences=active))
    input_path = scratch / 'skin-input.bin'; input_path.write_bytes(fixture)
    sources = [ROOT / 'tools/probe_original_weapon_skin.cpp', ROOT / 'src/rendering/models/model.cpp']
    commands, dependencies = [], set(sources)
    log = output / 'compile.log'; log.write_text('')
    for index, source in enumerate(sources):
        dependency = scratch / f'source-{index}.d'; obj = scratch / f'source-{index}.o'
        command = ['c++','-std=c++20','-O2','-ffp-contract=off','-ffunction-sections','-fdata-sections',
                   '-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),'-MMD','-MF',str(dependency),'-c',str(source),'-o',str(obj)]
        commands.append(command); run = subprocess.run(command,capture_output=True,text=True)
        with log.open('a') as stream: stream.write((run.stdout+run.stderr)[-65536:])
        assert run.returncode == 0, run.stderr[-2000:]
        deps = dependency.read_text().replace('\\\n',' ').split(':',1)[1].split()
        dependencies.update(Path(p).absolute() for p in deps)
    binary = scratch / 'skin-probe'; command = ['c++',str(scratch/'source-0.o'),str(scratch/'source-1.o'),'-Wl,--gc-sections','-o',str(binary)]
    commands.append(command); run = subprocess.run(command,capture_output=True,text=True)
    with log.open('a') as stream: stream.write((run.stdout+run.stderr)[-65536:])
    assert run.returncode == 0, run.stderr[-2000:]
    native_output = scratch / 'skin-output.bin'
    run = subprocess.run([str(binary),str(input_path),str(native_output)],capture_output=True,text=True)
    (output/'run.log').write_text(run.stdout+run.stderr); assert run.returncode == 0,run.stderr
    result = native_output.read_bytes(); assert struct.unpack_from('<II',result) == (0x534b4e31,len(cases))
    offset = 8; checked = []
    for case in cases:
        event,count = struct.unpack_from('<II',result,offset); offset += 8
        assert event == case['event'] and count == len(case['vertices'])
        mismatches = []; simd_mismatches = 0; weight_mismatches = 0; max_error = 0.
        for expected in case['vertices']:
            vertex = struct.unpack_from('<I',result,offset)[0]; offset += 4
            weight = struct.unpack_from('<4f',result,offset); offset += 16
            scalar = struct.unpack_from('<12f',result,offset); offset += 48
            simd = struct.unpack_from('<12f',result,offset); offset += 48
            assert vertex == expected['vertex']
            original_weight = [1.,0.,0.,0.]
            for i in range(1,case['active_influences']): original_weight[i] = f32(expected['stored_weights'][i]/255.)
            for i in range(case['active_influences']-1,0,-1): original_weight[0] = f32(original_weight[0]-original_weight[i])
            weight_mismatches += sum(a!=b for a,b in zip(words(weight),words(original_weight)))
            simd_mismatches += sum(a!=b for a,b in zip(words(scalar),words(simd)))
            compared = list(scalar[3:])+[f32(case['eye'][i]-scalar[i]) for i in range(3)]
            different = [i for i,(a,b) in enumerate(zip(words(compared),expected['original_words'])) if a!=b]
            if different:
                original = struct.unpack('<12f',struct.pack('<12I',*expected['original_words']))
                max_error = max(max_error, max(abs(a-b) for a,b in zip(compared,original)))
                if len(mismatches)<8: mismatches.append(dict(vertex=vertex,components=different,native_words=words(compared),original_words=expected['original_words']))
            expected['mismatched_components'] = len(different)
        checked.append(dict(event=event,section=case['section'],vertices=count,compared_float_words=count*12,
            original_GPU_mismatched_words=sum(v['mismatched_components'] for v in case['vertices']),
            scalar_SIMD_mismatched_words=simd_mismatches,effective_weight_mismatched_words=weight_mismatches,
            maximum_absolute_error=max_error,examples=mismatches))
    assert offset == len(result)
    source_records = {}
    for source in sorted(dependencies):
        assert source.is_relative_to(ROOT)
        destination = output/'sources'/source.relative_to(ROOT); destination.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(source,destination); pin(destination)
        source_records[str(source)] = dict(sha256=sha(source),snapshot=str(destination))
    for source in (Path(__file__),Path(__file__).with_name('original_weapon_gpu_evidence.py')):
        destination=output/source.name;shutil.copyfile(source,destination);pin(destination)
    shutil.copyfile(input_path,output/'input.bin');shutil.copyfile(native_output,output/'native-output.bin')
    for path in (output/'input.bin',output/'native-output.bin',log,output/'run.log'): pin(path)
    report = dict(passed=all(not r['original_GPU_mismatched_words'] and not r['scalar_SIMD_mismatched_words'] and not r['effective_weight_mismatched_words'] for r in checked),
        scope=__doc__,draws=checked,source_files=source_records,compile_commands=commands,compiled_binary_sha256=sha(binary),
        linked_files_sha256=pins,runner_scratch=str(scratch),original_GPU_postvs_sha256=sha(args.postvs/'report.json'),
        original_pose_and_matrix_inputs=True,source_skin_scalar_and_SIMD_both_executed=True,
        native_pose_production_verified=False,native_model_import_verified=False,native_framebuffer_parity=False,
        physical_xbox_verified=False,full_weapon_parity=False)
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=report['passed'],draws=checked,report=str(output/'report.json'))),flush=True)
    assert report['passed'], str(output/'report.json')


if __name__ == '__main__':
    main()
