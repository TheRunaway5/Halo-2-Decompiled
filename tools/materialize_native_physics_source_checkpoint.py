#!/usr/bin/env python3
"""Reconstruct authenticated source layers inside owned scratch, then run a command.

Only source inputs are reconstructed. The command receives HALO2_FROZEN_SOURCE;
the enclosing run_test.py owns every generated file and its cleanup.
"""
import argparse, hashlib, json, os, subprocess, tarfile
from pathlib import Path, PurePosixPath


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def safe_name(name):
    path = PurePosixPath(name)
    assert not path.is_absolute() and '..' not in path.parts and path.parts, name
    return path


def extract(archive, target):
    # Streaming avoids a second uncompressed source archive on disk. Links and
    # special files are excluded before any archive entry touches the target.
    process = subprocess.Popen(['zstd', '-dc', str(archive)], stdout=subprocess.PIPE)
    try:
        with tarfile.open(fileobj=process.stdout, mode='r|') as stream:
            count = total = 0
            for member in stream:
                if member.name in ('.', './'):
                    assert member.isdir()
                    continue
                relative = safe_name(member.name)
                assert member.isfile() or member.isdir(), member.name
                total += member.size
                count += 1
                assert count <= 20000 and total <= 2 * 1024**3
                path = target.joinpath(*relative.parts)
                assert target in path.resolve().parents
                if member.isdir():
                    path.mkdir(parents=True, exist_ok=True)
                    continue
                path.parent.mkdir(parents=True, exist_ok=True)
                with stream.extractfile(member) as source, path.open('wb') as output:
                    import shutil
                    shutil.copyfileobj(source, output)
                path.chmod(member.mode & 0o777)
        assert process.wait() == 0
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--receipt', type=Path, required=True)
    parser.add_argument('--receipt-sha256', required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('command', nargs=argparse.REMAINDER)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    target = scratch / 'frozen-source'
    target.mkdir()
    report = dict(passed=False, owned_scratch=str(scratch), source=str(target),
                  source_only=True, retained_large_artifacts=[], layers=[])
    seen = set()

    def apply(receipt, pin):
        receipt = receipt.resolve()
        assert receipt not in seen and sha(receipt) == pin
        seen.add(receipt)
        data = json.loads(receipt.read_text())
        assert data['passed']
        if 'parent_checkpoint' in data:
            apply(Path(data['parent_checkpoint']), data['parent_checkpoint_sha256'])
        else:
            baseline = data['baseline']
            archive = Path(baseline['path'])
            assert sha(archive) == baseline['sha256']
            extract(archive, target)
        archive_info = data.get('delta_archive', data.get('archive'))
        archive = Path(archive_info['path'])
        assert sha(archive) == archive_info['sha256']
        extract(archive, target)
        manifest = Path(data.get('source_manifest', receipt.parent / 'source-manifest.json'))
        assert sha(manifest) == data['source_manifest_sha256']
        files = json.loads(manifest.read_text())['source_files_sha256']
        for name, expected in files.items():
            path = target.joinpath(*safe_name(name).parts)
            assert path.is_file() and sha(path) == expected, name
        report['layers'].append(dict(receipt=str(receipt), receipt_sha256=pin,
                                     archive_sha256=sha(archive), files_verified=len(files)))

    try:
        apply(args.receipt, args.receipt_sha256)
        report.update(passed=True, files_verified=report['layers'][-1]['files_verified'])
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print('Authenticated frozen source:', target, flush=True)
        command = args.command[1:] if args.command[:1] == ['--'] else args.command
        if command:
            result = subprocess.run(command, env=dict(os.environ, HALO2_FROZEN_SOURCE=str(target)))
            report['command_exit_code'] = result.returncode
            assert result.returncode == 0, 'Frozen-source command failed'
    except BaseException as error:
        report.update(passed=False, error=str(error) or repr(error))
        raise
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(
            p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file())
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
