#!/usr/bin/env python3
"""Same Source2E5320/cache closure with isolated native sweep fault overlay."""
import inspect,sys
from pathlib import Path
import validate_native_moving_shape_process_cache_tu15 as parent
ROOT=Path(__file__).resolve().parents[1]
STAGE=ROOT/'analysis/physics/native-sweep-support-fault-phase-stage-20261102/staged'
def main():
 text=inspect.getsource(parent.main)
 text=text.replace('source=inspect.getsource(base.main)','source=inspect.getsource(base.main).replace("shutil.copyfile(ROOT/\'src/physics\'/name,project/name)","shutil.copyfile((STAGE/\'src/physics\'/name)if name==\'native_tu15_contact_shape.cpp\'else(ROOT/\'src/physics\'/name),project/name)")')
 text=text.replace('environment=dict(base.__dict__,EXTRA=EXTRA,driver_source=driver_source,__doc__=__doc__)','environment=dict(base.__dict__,EXTRA=EXTRA,STAGE=STAGE,driver_source=driver_source,__doc__=__doc__)')
 environment=dict(parent.__dict__,STAGE=STAGE,__doc__=__doc__);exec(text,environment);rc=environment['main']();p=Path(sys.argv[sys.argv.index('--output')+1])/'report.json';j=parent.base.json.loads(p.read_text());j['compiled_sweep_overlay_sha256']=parent.base.sha(STAGE/'src/physics/native_tu15_contact_shape.cpp');j['source_hashes'].pop('src/physics/native_tu15_contact_shape.cpp',None);j['source_hashes'][str((STAGE/'src/physics/native_tu15_contact_shape.cpp').relative_to(ROOT))]=j['compiled_sweep_overlay_sha256'];j['scope']=__doc__;parent.base.save(p,j);return rc
if __name__=='__main__':raise SystemExit(main())
