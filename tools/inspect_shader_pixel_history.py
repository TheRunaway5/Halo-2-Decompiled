"""Run with qrenderdoc --script; inspect a retained original/native pixel pair.

SHADER_ORIGINAL_RDC and SHADER_NATIVE_RDC name immutable captures.
SHADER_PIXEL_OUT must be a fresh directory. SHADER_ORIGINAL_EVENT names the
original draw; the native event is selected by SHADER_NATIVE_DRAW (zero based).
This tool records observed GPU history, never alters either capture or shader.
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
    if isinstance(value, rd.ResourceId) or depth > 10:
        return str(value)
    if isinstance(value, (list, tuple)) or type(value).__name__.startswith('rdcarray'):
        return [plain(item, depth + 1) for item in value]
    return {key: plain(getattr(value, key), depth + 1) for key in dir(value)
            if not key.startswith('_') and key not in ('this', 'thisown')
            and not callable(getattr(value, key))}


def actions(rows):
    for row in rows:
        yield row
        yield from actions(row.children)


out = Path(os.environ['SHADER_PIXEL_OUT'])
out.mkdir(parents=True, exist_ok=False)
try:
    for label, key in [('original', 'SHADER_ORIGINAL_RDC'), ('native', 'SHADER_NATIVE_RDC')]:
        capture = rd.OpenCaptureFile()
        assert capture.OpenFile(os.environ[key], '', None) == rd.ResultCode.Succeeded
        status, controller = capture.OpenCapture(rd.ReplayOptions(), None)
        assert status == rd.ResultCode.Succeeded
        draws = [row for row in actions(controller.GetRootActions())
                 if row.flags & rd.ActionFlags.Drawcall]
        event = (int(os.environ['SHADER_ORIGINAL_EVENT']) if label == 'original'
                 else draws[int(os.environ.get('SHADER_NATIVE_DRAW', '1'))].eventId)
        controller.SetFrameEvent(event, True)
        pipeline = controller.GetPipelineState()
        target = pipeline.GetOutputTargets()[0].resource
        result = {'capture': os.environ[key], 'event': event,
                  'draw_events': [row.eventId for row in draws], 'pixels': {}}
        # Retain both vertical conventions explicitly. No inferred image flip
        # is permitted to disguise which actual GPU pixel was inspected.
        for x, y in [(639, 385), (639, 94)]:
            result['pixels'][f'{x},{y}'] = plain(controller.PixelHistory(
                target, x, y, rd.Subresource(), rd.CompType.UNorm))
        (out / f'{label}.json').write_text(json.dumps(result, indent=2) + '\n')
        controller.Shutdown()
        capture.Shutdown()
except Exception:
    (out / 'error.txt').write_text(traceback.format_exc())
    os._exit(1)
os._exit(0)
