#!/usr/bin/env python3
"""Run the registered production callback and binding regressions in the owned worker."""
import json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    out=ROOT/'analysis/combat-presentation-audit-20261022/repair-object-function-symbols/production-validation'
    assert not out.exists();out.mkdir()
    command=[sys.executable,str(ROOT/'tools/run_tests.py'),'--build-worker','--source',str(owner/'source'),
        '--target','object_function_symbol_tu15_tests','--target','source_effect_compiled_symbols_tu15_tests',
        '--filter','^(object_function_symbol_namespace|source_effect_compiled_symbols_tu15)$',
        '--cmake-arg=-DHALO2_OBJECT_FUNCTION_MCC_MAP='+str(ROOT/'assets/imported/mcc-dire-valley-menu-20261023/halo2/h2_maps_win64_dx11/coagulation.map'),
        '--jobs','8','--test-jobs','1','--retain-report','object-function-symbol.json','--output',str(out)]
    with (out/'command.log').open('w') as log:code=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT).returncode
    print(json.dumps(dict(command=command,exit_code=code)),flush=True);return code
if __name__=='__main__':raise SystemExit(main())
