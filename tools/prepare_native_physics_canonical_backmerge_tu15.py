#!/usr/bin/env python3
"""Prepare, never apply, the qualified1474-1478 source changes over dirty Root.

Run through run_test.py. Authenticate the retained source-only layers, then
merge their authored delta with canonical source. Binaries/assets are untouched.
"""
import argparse, difflib, hashlib, io, json, os, subprocess, tarfile
from collections import Counter
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/physics-live-tu15-20261012'
FOLDERS = ('mcc-native-agent-wake-conveyor-original-v1473-20261102',
           'mcc-native-pair-delete-child-original-v1474-20261102',
           'mcc-native-contact-body-original-v1475-20261102',
           'mcc-native-pool-release-original-v1476-20261102',
           'mcc-native-main-heap-look-original-v1477-20261102',
           'mcc-native-camera-query-original-v1478-20261102')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical(relative):
    return 'src/physics/physics_live_world_tu15.cpp' if relative == 'src/physics_live_world_tu15.cpp' else relative


def archive_data(receipt):
    archive = receipt.get('delta_archive', receipt.get('archive'))
    path = Path(archive['path'])
    assert sha(path.read_bytes()) == archive['sha256'], str(path)
    raw = subprocess.run(['zstd', '-dc', str(path)], check=True, capture_output=True).stdout
    assert len(raw) < 128*1024*1024, 'Source archive exceeds declared source-only bound'
    result = {}
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r:') as stream:
        for member in stream.getmembers():
            relative = str(PurePosixPath(member.name))
            assert not PurePosixPath(relative).is_absolute() and '..' not in PurePosixPath(relative).parts
            if member.isdir():
                continue
            assert member.isreg() and relative not in result and member.size < 32*1024*1024, relative
            data = stream.extractfile(member).read()
            assert len(data) == member.size
            result[relative] = data
    assert set(result) == set(receipt['delta_files']), str(path)
    for relative, record in receipt['delta_files'].items():
        assert sha(result[relative]) == record['sha256'], relative
    return {canonical(relative): data for relative, data in result.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); args.output = args.output.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report = dict(prepared=False, applied=False, owned_scratch=str(scratch), retained_large_artifacts=[])
    try:
        contents, history, chain = {}, {}, []
        for ordinal, folder in enumerate(FOLDERS):
            kind = 'required-source-checkpoint' if ordinal == 0 else 'required-source-layer'
            path = BASE/folder/kind/'retention.json'
            receipt = json.loads(path.read_text()); assert receipt['passed']
            for relative, data in archive_data(receipt).items():
                history.setdefault(relative, []).append((str(path.relative_to(ROOT)), data))
                contents[relative] = data
            chain.append(dict(receipt=str(path.relative_to(ROOT)), sha256=sha(path.read_bytes())))
            if ordinal == 0:
                initial = dict(contents)
        stage = ROOT/'analysis/physics/native-reorientation-joint-cohort1478-stage-20261102'
        pins = json.loads((stage/'source-pins.json').read_text()); assert pins['qualified']
        for relative, pin in pins['base_hashes'].items():
            if pin is not None:
                assert sha(contents[canonical(relative)]) == pin, relative
        for relative, pin in pins['staged_hashes'].items():
            data = (stage/'staged'/relative).read_bytes(); assert sha(data) == pin, relative
            relative = canonical(relative); contents[relative] = data
            history.setdefault(relative, []).append((str(stage.relative_to(ROOT)), data))
        records, patches = {}, []
        for relative, new in sorted(contents.items()):
            old = initial.get(relative)
            if old == new:
                continue
            path = ROOT/relative; current = path.read_bytes() if path.exists() else None
            if current is not None:
                checkpoint = args.output/'root-before'/relative
                checkpoint.parent.mkdir(parents=True, exist_ok=True)
                checkpoint.write_bytes(current)
            record = dict(frozen_base_sha256=sha(old) if old is not None else None,
                          qualified_sha256=sha(new), root_before_sha256=sha(current) if current is not None else None)
            merged = None
            if current == new:
                record['status'] = 'already_equal'; records[relative] = record; continue
            if current is None and old is None:
                merged = new; record['status'] = 'new_qualified_source'
            elif current == old:
                merged = new; record['status'] = 'unchanged_root_base'
            elif old is None:
                matches = [source for source, data in history[relative] if data == current]
                if matches:
                    merged = new; record.update(status='earlier_owned_source', matched_source_receipts=matches)
                else:
                    record['status'] = 'new_source_root_divergence'
            elif current is None:
                record['status'] = 'missing_root_existing_source'
            else:
                work = scratch/'merge'; work.mkdir(exist_ok=True)
                for name, data in (('current', current), ('base', old), ('qualified', new)):
                    (work/name).write_bytes(data)
                process = subprocess.run(['git', 'merge-file', '-p', '--diff3', '-L', 'canonical Root',
                    '-L', 'frozen1473', '-L', 'qualified1478', str(work/'current'), str(work/'base'),
                    str(work/'qualified')], capture_output=True)
                assert 0 <= process.returncode <= 127, process.stderr.decode()
                merged = process.stdout if process.returncode == 0 else None
                record['status'] = 'merged_preserving_root' if process.returncode == 0 else 'conflict'
                if process.returncode:
                    target = args.output/'conflicts'/relative
                    target.parent.mkdir(parents=True, exist_ok=True); target.write_bytes(process.stdout)
            if merged is not None:
                target = args.output/'staged'/relative; target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(merged); record['candidate_sha256'] = sha(merged)
                patches.extend(difflib.unified_diff((current or b'').decode().splitlines(True),
                    merged.decode().splitlines(True), fromfile='a/'+relative, tofile='b/'+relative))
            records[relative] = record
        (args.output/'delta.patch').write_text(''.join(patches))
        report.update(prepared=True, scope='Only authored1473final06 to qualified1478combined32 source delta; '
            'three-way candidate preserves unrelated dirty Root. Review conflicts before mutation; no runtime promotion.',
            layers=chain, final_qualified_stage=str(stage.relative_to(ROOT)),
            final_qualified_stage_sha256=sha((stage/'source-pins.json').read_bytes()), files=records)
        print(dict(Counter(record['status'] for record in records.values())))
        for relative, record in records.items():
            if record['status'] in ('conflict', 'new_source_root_divergence', 'missing_root_existing_source'):
                print(record['status'], relative)
    except BaseException as error:
        report['error'] = str(error) or repr(error); raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks*512 for p in scratch.rglob('*') if p.is_file())
        (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
