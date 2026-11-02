#!/usr/bin/env python3
"""Independent public214-profile proof on the private shared HostVec4 overlay.

No kernel changes or new active-domain claim. The exact public numeric API
and all its alias/fault/observer/FP controls must remain source-equivalent.
"""
import inspect,json
from pathlib import Path
import validate_native_linear_jacobian_emitter_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-linear-jacobian-host-stage-20261102'
OVERLAY=STAGE/'staged/src/physics/native_tu15_linear_jacobian_emitter.cpp'
HOST=STAGE/'staged/include/halo2/native_tu15_linear_jacobian_host.hpp'
def main():
 pins=json.loads((STAGE/'source-pins.json').read_text());assert parent.sha(ROOT/parent.CPP)==pins['base_hashes'][parent.CPP]
 assert parent.sha(OVERLAY)==pins['staged_hashes'][parent.CPP]and parent.sha(HOST)==pins['staged_hashes']['include/halo2/native_tu15_linear_jacobian_host.hpp']
 text=inspect.getsource(parent.main).replace("source=(ROOT/CPP).read_text()","source=OVERLAY.read_text()")
 needle="shutil.copytree(ROOT/'include',project/'include');";assert text.count(needle)==1
 text=text.replace(needle,needle+"shutil.copyfile(HOST,project/'include/halo2/native_tu15_linear_jacobian_host.hpp');")
 text=text.replace("source_hashes={n:sha(ROOT/n)for n in(CPP,HPP,'tests/native_tu15_wrapped_contact_process_tests.cpp','tools/validate_native_linear_jacobian_emitter_tu15.py')}","source_hashes={str((OVERLAY if n==CPP else ROOT/n).relative_to(ROOT)):sha(OVERLAY if n==CPP else ROOT/n)for n in(CPP,HPP,'tests/native_tu15_wrapped_contact_process_tests.cpp','tools/validate_native_linear_jacobian_emitter_tu15.py')}")
 env=dict(parent.__dict__,OVERLAY=OVERLAY,HOST=HOST,__doc__=__doc__);exec(text,env);code=env['main']()
 output=Path(parent.sys.argv[parent.sys.argv.index('--output')+1]);path=output/'report.json';report=json.loads(path.read_text());report.update(scope=__doc__,overlay_manifest_sha256=parent.sha(STAGE/'source-pins.json'),overlay_cpp_sha256=parent.sha(OVERLAY),overlay_hpp_sha256=parent.sha(HOST),root_cpp_unchanged=parent.sha(ROOT/parent.CPP)==pins['base_hashes'][parent.CPP],active_HostVec4_domain_qualified=False,whole31D930_qualified=False);report['source_hashes'][str(HOST.relative_to(ROOT))]=parent.sha(HOST);report['source_hashes'][str(Path(__file__).relative_to(ROOT))]=parent.sha(__file__);parent.save(path,report);return code
if __name__=='__main__':raise SystemExit(main())
