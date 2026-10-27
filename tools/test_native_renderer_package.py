#!/usr/bin/env python3
"""Exercise the real production package and compiled instruction-read guard."""
import argparse
import json
import os
from pathlib import Path

from audit_native_renderer_kernels import source_runtime_calls,production_archive_violations
from stage_native_renderer_shaders import stage_production_shaders

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path,required=True)
    p.add_argument('--source-root',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    args=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    package=stage_production_shaders(args.build_dir,scratch/'production-shaders')
    calls=source_runtime_calls(args.source_root)
    archives=production_archive_violations(args.build_dir/'engine-targets.txt')
    report={'schema':'HALO2_NATIVE_RENDERER_PRODUCTION_ACCEPTANCE_V1',
        'passed':package['passed'] and not calls and not archives,'module_count':package['module_count'],
        'rejected_modules':0,'cpu_runtime_calls':calls,
        'production_archive_violations':archives,
        'inventory_sha256':package['inventory_sha256'],
        'scratch_owner':str(scratch),'large_outputs_lifecycle':'Removed by enclosing test wrapper',
        'modules':[{'path':r['path'],'sha256':r['sha256'],'bytes':r['bytes']} for r in package['modules']]}
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:report[k] for k in ('passed','module_count','rejected_modules','cpu_runtime_calls')}))
    return 0 if report['passed'] else 1

if __name__=='__main__':raise SystemExit(main())
