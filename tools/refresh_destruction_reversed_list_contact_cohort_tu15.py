#!/usr/bin/env python3
"""Apply only the independently qualified reversed-list delta to its live owner."""
import difflib
import hashlib
import json
import os
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True);source=scratch/'source'
    base=ROOT/'build/test-results/vehicle-destruction-phantom-live-20261101'
    prior=base/'scorpion-factory-capture-refresh.json';report=json.loads(prior.read_text())
    refresh=json.loads((base/'scorpion-factory-capture-build/refresh.json').read_text())
    assert Path(report['scratch'])==scratch
    qualifications=[]
    for name in ('v3','reversed-v1'):
        path=ROOT/'build/test-results/vehicle-destruction-list-contact-agent-20261101'/name/'report.json'
        result=json.loads(path.read_text());assert result['passed'] and result['current_sources_unchanged']
        assert all(sha(ROOT/path)==pin for path,pin in result['source_hashes'].items())
        qualifications.append(dict(path=str(path),sha256=sha(path),cases=6,compiler_count=2))
        report['original_qualified_dependency_closure'].update(result['source_hashes'])
    for name in ('include/halo2/native_tu15_list_contact_agent.hpp','src/physics/native_tu15_list_contact_agent.cpp',
                 'src/physics/native_tu15_world_integration_residuals.cpp'):
        path=source/name;before=path.read_text();assert sha(path)==report['patches'][name]['after']
        assert not path.is_symlink()
        if 'world_integration' in name:
            old='      if(target.identity==0x2f8030u){'
            assert before.count(old)==1
            text=before.replace(old,'      if(target.identity==0x2f8030u||target.identity==0x2f81f0u){')
            old='          const auto result=native_tu15_create_list_contact_agent(m,arguments,{retained,retained+16u},current,list,observe);'
            assert text.count(old)==1
            text=text.replace(old,'          const auto result=target.identity==0x2f81f0u?\n'
                '            native_tu15_create_reversed_list_contact_agent(m,arguments,{retained,retained+16u},current,list,observe):\n'
                '            native_tu15_create_list_contact_agent(m,arguments,{retained,retained+16u},current,list,observe);')
        else:text=(ROOT/name).read_text()
        path.write_text(text)
        report['patches'][name]=dict(before=hashlib.sha256(before.encode()).hexdigest(),after=sha(path),
            refreshed_mtime_ns=path.stat().st_mtime_ns,
            diff='\n'.join(difflib.unified_diff(before.splitlines(),text.splitlines(),fromfile='before/'+name,tofile='after/'+name)))
    report['patches']['tests/destruction_runtime_tu15_tests.cpp']=dict(before=refresh['fixture_refresh']['before'],after=refresh['fixture_refresh']['after'])
    report.update(cmake_sha256=refresh['registered_cmake_sha256'],previous_production_refresh=str(prior),
        qualification={'scope':'Native2F8030 and reversed2F81F0 against actual typed BEFORE inputs; ordinary collection filter only in this frozen cohort',
                       'reports':qualifications},unrelated_root_world_changes_not_imported=True)
    (base/'reversed-list-production-refresh.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Applied only qualified reversed-list owner; separate concurrent root collection implementation remains untouched')


if __name__=='__main__':main()
