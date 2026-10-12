"""Append-only unchanged original20 coverage-state export; hardware lease required."""
from pathlib import Path
import json
import os
import runpy
import traceback
import renderdoc as rd
ROOT=Path('/home/eric/Developer/Halo-2-Decompiled')
helper=runpy.run_path(str(ROOT/'tools/export_source_sky_stencil_stage.py'))
OUT=helper['OUT']
cap=ctrl=None
report=dict(status='opening',capture_sha256=helper['EXPECTED'],guest_IO=False,shader_replacements=False,
    script_sha256=helper['digest'](ROOT/'tools/export_source_sky_stencil_coverage.py'),events=[])
try:
    assert helper['digest'](helper['CAPTURE'])==helper['EXPECTED']
    cap=rd.OpenCaptureFile()
    assert cap.OpenFile(str(helper['CAPTURE']),'',None)==rd.ResultCode.Succeeded
    result,ctrl=cap.OpenCapture(rd.ReplayOptions(),None)
    assert result==rd.ResultCode.Succeeded
    for event in (8097,8102,8110):
        ctrl.SetFrameEvent(event,True)
        vk=ctrl.GetVulkanPipelineState()
        item=dict(event=event,multisample=helper['plain'](vk.multisample),
            colorBlend=helper['plain'](vk.colorBlend))
        for field in ('conditionalRendering','predication'):
            if hasattr(vk,field):item[field]=helper['plain'](getattr(vk,field))
        report['events'].append(item)
    report['status']='complete'
except Exception:
    report['status']='failed';report['error']=traceback.format_exc()
finally:
    if ctrl:ctrl.Shutdown()
    if cap:cap.Shutdown()
    report['capture_unchanged']=helper['digest'](helper['CAPTURE'])==helper['EXPECTED']
    (OUT/'coverage-state.json').write_text(json.dumps(report,indent=2)+'\n')
os._exit(0 if report['status']=='complete' and report['capture_unchanged'] else 1)
