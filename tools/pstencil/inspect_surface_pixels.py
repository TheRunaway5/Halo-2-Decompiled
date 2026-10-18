"""Inspect selected actual native surface pixels without shader/state replacement.

Use bundled qrenderdoc --script. PSTENCIL_SURFACE_CAPTURE_RECEIPT identifies
an actual capture; PSTENCIL_SURFACE_INVENTORY, PIXELS and OUT identify the
retained inventory, selected top-left coordinates and fresh output directory.
"""
import json
import math
import os
from pathlib import Path
import traceback
import renderdoc as rd


def plain(value, depth=0):
    if isinstance(value, float) and not math.isfinite(value):
        return str(value)
    if value is None or isinstance(value, (str, int, float, bool)):
        return value
    if isinstance(value, rd.ResourceId) or depth > 8:
        return str(value)
    if isinstance(value, (list, tuple)) or type(value).__name__.startswith('rdcarray'):
        return [plain(v, depth + 1) for v in value]
    return {k: plain(getattr(value, k), depth + 1) for k in dir(value)
            if not k.startswith('_') and k not in ('this', 'thisown')
            and not callable(getattr(value, k))}


def actions(rows):
    for row in rows:
        yield row
        yield from actions(row.children)


out = Path(os.environ['PSTENCIL_SURFACE_OUT'])
out.mkdir(parents=True, exist_ok=False)
file = controller = None
try:
    receipt = json.loads(Path(os.environ['PSTENCIL_SURFACE_CAPTURE_RECEIPT']).read_text())
    inventory = json.loads(Path(os.environ['PSTENCIL_SURFACE_INVENTORY']).read_text())
    points = json.loads(Path(os.environ['PSTENCIL_SURFACE_PIXELS']).read_text())
    file = rd.OpenCaptureFile()
    assert file.OpenFile(receipt['capture'], '', None) == rd.ResultCode.Succeeded
    status, controller = file.OpenCapture(rd.ReplayOptions(), None)
    assert status == rd.ResultCode.Succeeded, str(status)
    target_name = max(inventory['targets'], key=lambda t: len(t['color_events']))['resource']
    target = next(t.resourceId for t in controller.GetTextures() if str(t.resourceId) == target_name)
    end = max(a.eventId for a in actions(controller.GetRootActions()))
    controller.SetFrameEvent(end, True)
    report = dict(scope=__doc__, capture=receipt['capture'], target=target_name, pixels=[])
    for point in points:
        (out / 'progress.txt').write_text('Inspecting ' + point['name'] + '\n')
        history = controller.PixelHistory(target, point['x'], point['y'], rd.Subresource(), rd.CompType.UNorm)
        report['pixels'].append(dict(**point, history=plain(history)))
        (out / 'pixel-history.json').write_text(json.dumps(report, indent=2) + '\n')
    (out / 'progress.txt').write_text('Complete\n')
except BaseException:
    (out / 'error.txt').write_text(traceback.format_exc())
    if controller is not None:
        controller.Shutdown()
    if file is not None:
        file.Shutdown()
    os._exit(1)
controller.Shutdown()
file.Shutdown()
os._exit(0)
