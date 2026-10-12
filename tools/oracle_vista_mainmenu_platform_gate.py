#!/usr/bin/env python3
"""Generate the native producer fixture from unchanged original instructions.

Win32 handles and SLDL results are explicit input boundaries; no native license
result, installed-license validity, or real platform initialization is inferred.
"""
from pathlib import Path
import hashlib,importlib.util,json
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'analysis/frontend/vista-session-flow-audit/mainmenu-network-original/execute_pbuffer_initializer.py'
spec=importlib.util.spec_from_file_location('original_gate',SCRIPT)
original=importlib.util.module_from_spec(spec)
spec.loader.exec_module(original)
if __name__=='__main__':
    original.main()
    report_path=SCRIPT.parent/'pbuffer-initializer/execution.json'
    report=json.loads(report_path.read_text())
    fixture=ROOT/'tests/fixtures/vista_mainmenu_platform_gate_original.json'
    fixture.write_text(json.dumps(dict(rows=report['rows']),separators=(',',':'))+'\n')
    report['fixture_sha256']=hashlib.sha256(fixture.read_bytes()).hexdigest()
    report_path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(fixture=str(fixture),sha256=report['fixture_sha256'])))
