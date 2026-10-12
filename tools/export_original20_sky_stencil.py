"""Run in qrenderdoc: export immutable original20 sky depth/stencil resources.

Only standard replay/readback APIs; no new guest frame, shader replacement,
SDK patch, inferred raw packing or synthetic scene input.
"""
from pathlib import Path
import hashlib
import json
import os
import traceback
import renderdoc as rd

ROOT = Path('/home/eric/Developer/Halo-2-Decompiled')
CAPTURE = ROOT / 'analysis/multiplayer-shading/original-xemu/original_capture_20.rdc'
EXPECTED = 'd99a5c97c85916c7b07e30dc1f181d6db81b3a5246686bcbeebebbc5445642ee'
OUT = ROOT / 'analysis/multiplayer-shading/original20-sky-stencil'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def plain(obj):
    if obj is None or isinstance(obj, (str, bool, int, float)):
        return obj
    if isinstance(obj, rd.ResourceId):
        return str(obj)
    if isinstance(obj, (list, tuple)) or type(obj).__name__.startswith('rdcarray'):
        return [plain(v) for v in obj]
    return {k: plain(getattr(obj, k)) for k in dir(obj)
            if not k.startswith('_') and k not in ('this', 'thisown') and not callable(getattr(obj, k))}


def blob(name, raw):
    p = OUT / name; data = bytes(raw); p.write_bytes(data)
    return dict(file=name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest())


def enum_values(kind):
    result = {}
    for name in dir(kind):
        if name.startswith('_') or callable(getattr(kind, name)):
            continue
        try:
            result[name] = int(getattr(kind, name))
        except (ValueError, TypeError):
            pass
    return result


def run():
    OUT.mkdir(parents=True, exist_ok=True)
    assert digest(CAPTURE) == EXPECTED
    report = dict(capture=str(CAPTURE.relative_to(ROOT)), capture_sha256=EXPECTED,
        script_sha256=digest(ROOT / 'tools/export_original20_sky_stencil.py'), guest_IO=False,
        scope='Standard offline unchanged original20 Vulkan capture replay. Sky A/B only; not Lockout stencil attribution or physical Xbox precision.',
        raw_packing='Not assumed by exporter; independent actual PickPixel controls retained.',
        snapshots=[], events=[], status='opening')
    cap = ctrl = None
    try:
        cap = rd.OpenCaptureFile()
        assert cap.OpenFile(str(CAPTURE), '', None) == rd.ResultCode.Succeeded
        status, ctrl = cap.OpenCapture(rd.ReplayOptions(), None)
        assert status == rd.ResultCode.Succeeded, str(status)
        textures = {str(t.resourceId): t for t in ctrl.GetTextures()}
        for event in (8097, 8102):
            ctrl.SetFrameEvent(event, True)
            pipe = ctrl.GetPipelineState()
            depth = pipe.GetDepthTarget().resource
            texture = textures[str(depth)]
            assert str(depth) == 'ResourceId::8601' and texture.format.Name() == 'D24S8'
            assert texture.width == 640 and texture.height == 480 and texture.msSamp == 1
            vulkan = ctrl.GetVulkanPipelineState()
            report['events'].append(dict(event=event,
                depth_resource=str(depth), depth_state=plain(vulkan.depthStencil),
                raster=plain(vulkan.rasterizer), blend=plain(vulkan.colorBlend),
                enums=dict(compare=enum_values(rd.CompareFunction),
                    stencil=enum_values(rd.StencilOperation))))
            for label, state_event in (('before', event-1), ('after', event)):
                ctrl.SetFrameEvent(state_event, True)
                item = dict(draw_event=event, replay_inclusive_event=state_event, phase=label,
                    resource=str(depth), format=texture.format.Name(), width=texture.width,
                    height=texture.height, samples=texture.msSamp)
                item.update(blob(f'{event}-{label}-depth-d24s8.raw', ctrl.GetTextureData(depth, rd.Subresource())))
                item['pixel_controls'] = []
                for x,y in ((0,0),(320,240),(241,85),(367,120),(100,100),(500,100),(100,400),(500,400),(639,479)):
                    value = ctrl.PickPixel(depth,x,y,rd.Subresource(),rd.CompType.Typeless)
                    item['pixel_controls'].append(dict(x=x,y=y,floatValue=list(value.floatValue),
                                                       uintValue=list(value.uintValue)))
                report['snapshots'].append(item)
                (OUT / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
        report['status'] = 'complete'
    except Exception:
        report['status'] = 'failed'; report['error'] = traceback.format_exc()
    finally:
        if ctrl:ctrl.Shutdown()
        if cap:cap.Shutdown()
        report['capture_unchanged'] = digest(CAPTURE) == EXPECTED
        (OUT / 'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['status']=='complete' and report['capture_unchanged'] else 1


if __name__ == '__main__':
    os._exit(run())
