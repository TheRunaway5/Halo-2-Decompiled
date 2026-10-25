#!/usr/bin/env python3
"""Keep private native body partitions stable during this active investigation.

No root build tool or emitted instruction body changes. The owned splitter
reuses its already checked entry/chunk assignments when the exact declaration
set is unchanged, and retains timestamps for byte-identical generated files.
"""
import hashlib,importlib.util,json,os
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    root_hash=sha(ROOT/'tools/split_native_translation_unit.py')
    script=owner/'source/tools/split_native_translation_unit.py';before=script.read_text();text=before
    needle='    rows = []\n    for m in sorted(bodies, key=lambda m: (-len(m[0]), m.start())):\n        index = min(range(count), key=lambda n: (sizes[n], n))\n'
    assert text.count(needle)==1
    text=text.replace(needle,'''    rows = []
    prior_chunks = {}
    prior_manifest = directory/'manifest.json'
    if prior_manifest.is_file():
        prior = json.loads(prior_manifest.read_text())
        if prior.get('source') == str(source) and prior.get('chunk_count') == count:
            candidate = {row['entry']: row['chunk'] for row in prior['entries']}
            if set(candidate) == set(declarations) and all(0 <= n < count for n in candidate.values()):
                prior_chunks = candidate
    for m in sorted(bodies, key=lambda m: (-len(m[0]), m.start())):
        index = prior_chunks[m[1]] if prior_chunks else min(range(count), key=lambda n: (sizes[n], n))
''')
    needle='def split(source, directory, count):'
    text=text.replace(needle,'''def publish_private(path, text):
    if not path.exists() or path.read_text() != text:
        path.write_text(text)


'''+needle)
    for old,new in (("(directory/'common.hpp').write_text(common)","publish_private(directory/'common.hpp',common)"),
        ("(directory/f'body_{n:03d}.cpp').write_text(value)","publish_private(directory/f'body_{n:03d}.cpp',value)"),
        ("(directory/'router.cpp').write_text(router)","publish_private(directory/'router.cpp',router)")):
        assert text.count(old)==1;text=text.replace(old,new)
    backup=owner/'object-function-partitioner-original.py';backup.write_text(before);script.write_text(text)
    directory=owner/'build/native-translations/physics_live_support_tu15_generated'
    manifest=json.loads((directory/'manifest.json').read_text())
    files=[directory/'common.hpp',directory/'router.cpp',*sorted(directory.glob('body_*.cpp'))]
    states={str(p):dict(sha256=sha(p),mtime_ns=p.stat().st_mtime_ns) for p in files}
    spec=importlib.util.spec_from_file_location('private_body_partition',script);module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    current=module.split(Path(manifest['source']),directory,manifest['chunk_count'])
    assert current['entries']==manifest['entries']
    assert all(states[str(p)]==dict(sha256=sha(p),mtime_ns=p.stat().st_mtime_ns) for p in files)
    receipt=dict(scope=__doc__,root_tool_unchanged=sha(ROOT/'tools/split_native_translation_unit.py')==root_hash,
        before_sha256=hashlib.sha256(before.encode()).hexdigest(),after_sha256=sha(script),
        native_bodies_checked=current['body_count'],generated_files_hash_and_timestamp_unchanged=len(files),
        declaration_chunk_assignments_unchanged=True,backup=str(backup),private_tool=str(script))
    (OUT/'owned-partition-optimization.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(receipt),flush=True)
if __name__=='__main__':main()
