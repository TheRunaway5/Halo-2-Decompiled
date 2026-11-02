#!/usr/bin/env python3
"""Owned composition of native point removal with genuine island320100 DATA.

The current receiver uses authenticated TU1.5 table41F080 in declared source
DATA. This does not claim a captured live receiver. Both participating native
kernels remain frozen and unchanged; no CPU/register postimages are supplied.
"""
import argparse
import hashlib
import importlib.util
import json
import lzma
import os
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode',choices=('children','lifetime'),required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compilers',nargs='+',default=['g++','clang++'])
    args=parser.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True,exist_ok=False)
    private=scratch/'composed-tools';private.mkdir()
    source=(ROOT/'tools/validate_native_contact_point_removal_tu15.py').read_text()
    source=source.replace('0x8fdb0','0x320100')
    source=source.replace("        assert image.read(0x320100,3)==bytes.fromhex('c20800')\n",'')
    anchor='        row = dict(name=name'
    assert source.count(anchor)==1
    source=source.replace(anchor,'        words += [(0xc00,0x41f080),(0xc0c,0x1000),(0xc10,0x100),(0xc18,0x10)]\n'+anchor)
    anchor="        fixture,authority=environment['original'](image,{},all_cases,scratch)"
    assert source.count(anchor)==1
    source=source.replace(anchor,"        original_builder=environment['original'];environment['original']=lambda image,banks,rows,scratch:original_builder(image,{0x41f080:image.read(0x41f080,20)},rows,scratch)\n"+anchor)
    anchor='"tests/native_tu15_array_growth_tests.cpp"'
    # Source pins name the real reusable module, not merely the adapter recipe.
    anchor="'tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp')"
    assert source.count(anchor)==1
    source=source.replace(anchor,"'tools/observe_native_array_growth_tu15.cpp','tests/native_tu15_array_growth_tests.cpp','include/halo2/native_tu15_island_partition_metadata.hpp','src/physics/native_tu15_island_partition_metadata.cpp','tools/validate_native_contact_point_island_tu15.py')")
    anchor="        shutil.copyfile(ROOT/'src/physics/native_tu15_contact_point_removal.cpp',project/'native.cpp')"
    assert source.count(anchor)==1
    source=source.replace(anchor,anchor+"\n        shutil.copyfile(ROOT/'src/physics/native_tu15_island_partition_metadata.cpp',project/'metadata.cpp')")
    anchor='check(owner.identity==0x320100u,"Unqualified component collector update");'
    assert source.count(anchor)==1
    source=source.replace('info,16)}});mutate(owner.identity);};','info,16)}});mutate(owner.identity);native_tu15_remove_island_constraint_info(memory,{receiver},{info},observe);};')
    assert 'native_tu15_remove_island_constraint_info(memory,{receiver},{info},observe)'in source
    anchor="        driver='#include \"halo2/native_tu15_contact_point_removal.hpp\"\\n'+driver[:begin]+replacement+driver[end:]"
    assert source.count(anchor)==1
    source=source.replace(anchor,anchor+"\n        driver='#include \"halo2/native_tu15_island_partition_metadata.hpp\"\\n'+driver")
    source=source.replace('point driver.cpp native.cpp)','point driver.cpp native.cpp metadata.cpp)')
    path=private/'validate_native_contact_point_removal_tu15.py';path.write_text(source)
    (args.output/'composed-validator.py.xz').write_bytes(lzma.compress(source.encode()))
    report=dict(passed=False,owned_scratch=str(scratch),mode=args.mode,
                recipe_sha256=sha(__file__),private_composed_validator_sha256=sha(path),
                authenticated_declared_table=0x41f080,source_callback_owner=0x320100,
                actual_captured_receiver_claimed=False,retained_large_artifacts=[])
    try:
        spec=importlib.util.spec_from_file_location('validate_native_contact_point_removal_tu15',path)
        parent=importlib.util.module_from_spec(spec);sys.modules[spec.name]=parent;spec.loader.exec_module(parent)
        child_output=args.output/'qualification'
        sys.argv=['point-island-composition','--output',str(child_output),'--compilers',*args.compilers]
        if args.mode=='children':
            sys.argv+=['--children-only'];result=parent.main()
        else:
            spec=importlib.util.spec_from_file_location('contact_point_island_lifetime',ROOT/'tools/validate_native_contact_point_data_lifetime_tu15.py')
            lifetime=importlib.util.module_from_spec(spec);spec.loader.exec_module(lifetime);result=lifetime.main()
        qualification=json.loads((child_output/'report.json').read_text())
        qualification.update(composed_native_collector_owner=0x320100,
                             authenticated_declared_receiver_table=0x41f080,
                             inherited_component_scope=qualification['scope'])
        qualification['scope']=('Strict direct DATA comparison with native320100 callback; all ordered operands/preimages/full DATA banks/FP checked.'if args.mode=='children'else
                                'Separate nonlast-point initialized-DATA lifetime comparison with native320100 callback; strict raw BEFORE ledger remains RED. All raw deltas retained.')+' Declared current41F080 receiver; actual event listener readers and live receiver correspondence remain separate gates.'
        (child_output/'report.json').write_text(json.dumps(qualification,indent=2,sort_keys=True)+'\n')
        report.update(passed=result==0 and qualification['passed'],qualification=str(child_output/'report.json'),
                      strict_all_events_passed=qualification.get('strict_all_events_passed',args.mode=='children'),
                      scope='Native325D90/320100 current info DATA composition; declared source-valid receiver. Actual event callback readers, last-point316AA0 continuation and live persistent frame are separate gates.')
        return result
    except BaseException as error:
        report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(path.stat().st_blocks*512 for path in scratch.rglob('*')if path.is_file())
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')


if __name__=='__main__':
    raise SystemExit(main())
