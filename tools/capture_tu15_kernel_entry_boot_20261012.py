#!/usr/bin/env python3
"""Isolated owned boot stopped before the kernel's first instruction."""
import hashlib,json,sys
from pathlib import Path
import capture_xbox_kernel_hardware_boot_20261012 as original

# Keep the older capture driver immutable. This driver changes only the stop
# phase and adds an execution breakpoint, never a guest instruction/data patch.
source=Path(original.__file__).read_text()
source=source.replace("q('qmp_capabilities'); q('cont'); time.sleep(5); q('stop')",
 "q('qmp_capabilities')")
source=source.replace("status = q('query-status'); assert not status['running']",
 "status = q('query-status'); assert not status['running']")
source=source.replace("'-ex','target remote 127.0.0.1:'+str(port), '-ex','source '+str(CAPTURE),",
 "'-ex','target remote 127.0.0.1:'+str(port), '-ex','hbreak *0x8005504c', '-ex','continue', '-ex','source '+str(CAPTURE),")
source=source.replace("env=env,capture_output=True,text=True,timeout=20)",
 "env=env,capture_output=True,text=True,timeout=40)")
source=source.replace("if __name__ == '__main__': main()",'')
scope={'__file__':str(Path(original.__file__)), '__name__':'isolated_entry_driver'}
exec(compile(source,str(Path(__file__)), 'exec'),scope)
scope['CAPTURE']=Path(__file__).resolve().with_name('capture_tu15_kernel_entry_20261012.py')
if __name__=='__main__':
 scope['main']()
 out=Path(sys.argv[sys.argv.index('--output')+1]);report=json.loads((out/'validation.json').read_text())
 report['boot_driver_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
 report['base_boot_driver_sha256']=hashlib.sha256(Path(original.__file__).read_bytes()).hexdigest()
 report['stop_phase']='before kernel entry instruction'
 report['execution_breakpoint']=0x8005504c
 (out/'validation.json').write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
