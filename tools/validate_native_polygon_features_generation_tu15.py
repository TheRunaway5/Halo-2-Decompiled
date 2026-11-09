#!/usr/bin/env python3
"""Both complete generators preserve the qualified native feature/support owners."""
import json
import os
from pathlib import Path
import sys
import native_polygon_features_replacements as features
import validate_native_polygon_support_generation_tu15 as baseline
from specialize_native_operands import specialize

TARGETS={
    'world':'9123a413f743a838e220e6a664168b6cd847d8e684c7f1ea9a319851b6b3c7f5',
    'contact':'2db24ff300af1f07f1d6ee93c247bae9871a8d607bbbb65566b3e8b4ced4c817',
}
def main():
    baseline.FILES+=('tools/native_polygon_features_replacements.py','tools/validate_native_polygon_features_generation_tu15.py',
        'src/physics/runtime/native_tu15_polygon_features_abi.hpp','include/halo2/native_tu15_polygon_features_kernel.hpp',
        'include/halo2/native_tu15_polygon_features_value.hpp','src/physics/native_tu15_contact_features.cpp',
        'cmake/NativeOperandSpecialization.cmake')
    result=baseline.main();report_path=Path(sys.argv[sys.argv.index('--report')+1]);report=json.loads(report_path.read_text())
    try:
        scratch=Path(os.environ['HALO2_TEST_SCRATCH']);units=[]
        for label in ('world','contact'):
            generated=(scratch/(label+'.cpp')).read_text();owner=features.authenticate(generated)
            if not owner or features.PREFIX not in owner[0][0]or features.replace_polygon_features(generated)!=generated:raise ValueError('Complete generated native feature prefix absent')
            prepared,receipt=specialize(generated);body=features.BODY.search(prepared)[0]
            if features.digest(body)!=TARGETS[label]or receipt['native_polygon_feature_entries']!=['1f6690']:raise ValueError('Prepared complete polygon-feature body drift')
            units.append(dict(owner=label,raw_target_sha256=features.digest(owner[0][0]),prepared_target_sha256=features.digest(body),native_polygon_feature_entries=receipt['native_polygon_feature_entries']))
        report['native_polygon_features']=units;report['complete_source_and_both_owner_groups_preserved']=True
    except BaseException as error:report.update(passed=False,feature_error=repr(error));raise
    finally:report_path.write_text(json.dumps(report,indent=2)+'\n')
    return result
if __name__=='__main__':raise SystemExit(main())
