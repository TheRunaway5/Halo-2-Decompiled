#!/usr/bin/env python3
"""Isolated read-only boot for the named complete kernel301 table capture."""
import hashlib,json,sys
from pathlib import Path
import capture_xbox_kernel_hardware_boot_20261012 as boot
boot.CAPTURE=Path(__file__).resolve().with_name('capture_tu15_status_tables_20261012.py')
if __name__=='__main__':
    boot.main()
    out=Path(sys.argv[sys.argv.index('--output')+1])
    report=json.loads((out/'validation.json').read_text())
    report['boot_probe_override_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    report['base_boot_driver_sha256']=hashlib.sha256(Path(boot.__file__).read_bytes()).hexdigest()
    (out/'validation.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
