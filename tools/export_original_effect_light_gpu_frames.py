"""Run with qrenderdoc --script to export retained original grenade GPU frames.

The original-only 33-draw index-count sequence locates candidate segments inside
the retained Xemu GL capture. Framebuffers, depth, bound images, original shader
source and current uniforms come from replay, never the native port. This does
not assert native framebuffer parity or physical Xbox GPU precision.
Set FX_RDC, FX_OUT and FX_REFERENCE to the capture, fresh output directory and
independently exported original draw-owner JSON.
FX_SELECTION may instead name a retained explicit frame/event selection. This
supports a separately authenticated TU1.5 capture whose actual draw count and
shader variants differ from the historical base-disc fixture.
"""
import hashlib
import json
import math
import os
from pathlib import Path
import traceback
import renderdoc as rd

OUT=Path(os.environ['FX_OUT'])
OUT.mkdir(parents=True,exist_ok=False)


def progress(message):
    with (OUT/'progress.txt').open('a') as f:
        f.write(message+'\n')


def plain(value,depth=0):
    if isinstance(value,float) and not math.isfinite(value):
        # RenderDoc also exposes unused registers. JSON has no NaN/Inf token;
        # null keeps that unknown/nonfinite input explicit for strict readers.
        return None
    if value is None or isinstance(value,(str,int,float,bool)):
        return value
    if isinstance(value,rd.ResourceId) or depth>12:
        return str(value)
    if isinstance(value,(list,tuple)) or type(value).__name__.startswith('rdcarray'):
        return [plain(v,depth+1) for v in value]
    result={}
    for key in dir(value):
        if key.startswith('_') or key in ('this','thisown'):
            continue
        try:
            member=getattr(value,key)
            if not callable(member):
                result[key]=plain(member,depth+1)
        except Exception:
            pass
    return result or str(value)


def flatten(actions):
    for action in actions:
        yield action
        yield from flatten(action.children)


def digest(path):
    with path.open('rb') as f:
        return hashlib.file_digest(f,'sha256').hexdigest()


try:
    capture=Path(os.environ['FX_RDC'])
    reference=Path(os.environ['FX_REFERENCE'])
    selection=Path(os.environ['FX_SELECTION']) if os.environ.get('FX_SELECTION') else None
    expected=None if selection else [r['count'] for r in json.loads(reference.read_text())['expected']]
    if expected is not None:
        assert len(expected)==33
    progress('opening retained original capture')
    handle=rd.OpenCaptureFile()
    assert handle.OpenFile(str(capture),'',None)==rd.ResultCode.Succeeded
    options=rd.ReplayOptions()
    options.optimisation=rd.ReplayOptimisationLevel.Fastest
    status,ctrl=handle.OpenCapture(options,None)
    assert status==rd.ResultCode.Succeeded
    progress('original replay opened')
    resources={str(t.resourceId):t for t in ctrl.GetTextures()}
    frames=[]
    draws=[]
    ordinal=0
    actions=list(flatten(ctrl.GetRootActions()))
    for action in actions if selection is None else []:
        if action.flags&rd.ActionFlags.Drawcall:
            draws.append(action)
        if action.flags&rd.ActionFlags.Present:
            counts=[a.numIndices for a in draws]
            found=[i for i in range(len(draws)-len(expected)+1)
                   if counts[i:i+len(expected)]==expected]
            for first in found:
                frames.append(dict(ordinal=ordinal,present=action.eventId,
                    first=draws[first].eventId,last=draws[first+32].eventId,
                    events=[a.eventId for a in draws[first:first+33]]))
            draws=[]
            ordinal+=1
    if selection:
        frames=json.loads(selection.read_text())['frames']
        actual={a.eventId:a for a in actions if a.flags&rd.ActionFlags.Drawcall}
        for frame in frames:
            assert frame['events'] and frame['events']==sorted(frame['events'])
            assert all(e in actual for e in frame['events'])
            assert frame['first']==frame['events'][0] and frame['last']==frame['events'][-1]
    # Index counts alone cannot identify a pass: the same source geometry can
    # occur in another phase. Preserve every candidate's actual pipeline and
    # uniforms before classifying any segment as a spherical-light draw.
    assert frames
    (OUT/'segments.json').write_text(json.dumps(frames,indent=2)+'\n')
    if os.environ.get('FX_FRAME'):
        frames=[f for f in frames if f['ordinal']==int(os.environ['FX_FRAME'])]
        assert frames
    exported=set()
    for frame in frames:
        directory=OUT/f"frame-{frame['ordinal']}"
        directory.mkdir()
        phases=[('before',frame['first']-1),('first',frame['first']),
                ('after',frame['last']),('present',frame['present']-2)]
        if os.environ.get('FX_ALL_DRAWS'):
            phases=[('before',frame['first']-1)]
            for i,event in enumerate(frame['events']):
                # A separately owned model draw can occur inside the sorted
                # light layer. Retain the actual target immediately after it
                # so each contiguous BSP segment has an explicit input seed.
                if str(i) in frame.get('reseeds',{}):
                    phases.append((f'draw-{i:02d}-before',frame['reseeds'][str(i)]))
                phases.append((f'draw-{i:02d}',event))
            phases += [('after',frame['last']),('present',frame['present']-2)]
        for phase,event in phases:
            progress(f"frame {frame['ordinal']} {phase} replay to {event}")
            ctrl.SetFrameEvent(event,True)
            pipe=ctrl.GetPipelineState()
            gl=ctrl.GetGLPipelineState()
            state=dict(event=event,pipeline=plain(gl),outputs=plain(pipe.GetOutputTargets()),
                       depth=plain(pipe.GetDepthTarget()))
            (directory/(phase+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
            targets=[('color'+str(i),r.resource) for i,r in enumerate(pipe.GetOutputTargets())]
            depth=pipe.GetDepthTarget().resource
            if depth!=rd.ResourceId.Null():
                targets.append(('depth',depth))
            if phase=='first' or (phase.startswith('draw-') and not phase.endswith('-before')):
                state['uniforms']={}
                state['read_resources']=plain(pipe.GetReadOnlyResources(rd.ShaderStage.Fragment))
                for stage in (rd.ShaderStage.Vertex,rd.ShaderStage.Fragment):
                    reflection=pipe.GetShaderReflection(stage)
                    if reflection is None:
                        continue
                    shader=pipe.GetShader(stage)
                    (directory/(str(stage)+'.glsl')).write_bytes(bytes(reflection.rawBytes))
                    banks=[]
                    for index,block in enumerate(reflection.constantBlocks):
                        binding=pipe.GetConstantBlock(stage,index,0).descriptor
                        contents=ctrl.GetCBufferVariableContents(pipe.GetGraphicsPipelineObject(),shader,stage,
                            pipe.GetShaderEntryPoint(stage),index,binding.resource,binding.byteOffset,binding.byteSize)
                        banks.append(dict(block=plain(block),contents=plain(contents)))
                    state['uniforms'][str(stage)]=banks
                for bound in pipe.GetReadOnlyResources(rd.ShaderStage.Fragment):
                    resource=bound.descriptor.resource
                    if str(resource) in resources:
                        targets.append(('bound-'+str(resource).split('::')[-1],resource))
            for label,resource in targets:
                if str(resource) not in resources:
                    continue
                description=resources[str(resource)]
                stem=directory/(phase+'-'+label)
                (stem.with_suffix('.json')).write_text(json.dumps(plain(description),indent=2)+'\n')
                (stem.with_suffix('.bin')).write_bytes(bytes(ctrl.GetTextureData(resource,rd.Subresource())))
                if label.startswith('bound-') and str(resource) not in exported:
                    # Cube faces and authored mip chains remain distinct inputs.
                    # A resource identity is only used to retain replay data.
                    texture_dir=OUT/'textures'/label
                    texture_dir.mkdir(parents=True)
                    (texture_dir/'description.json').write_text(json.dumps(plain(description),indent=2)+'\n')
                    for mip in range(description.mips):
                        for face in range(description.arraysize):
                            sub=rd.Subresource()
                            sub.mip=mip
                            sub.slice=face
                            (texture_dir/f'mip-{mip}-slice-{face}.bin').write_bytes(bytes(ctrl.GetTextureData(resource,sub)))
                    exported.add(str(resource))
                if label.startswith('color'):
                    save=rd.TextureSave()
                    save.resourceId=resource
                    save.destType=rd.FileType.PNG
                    save.mip=0
                    ctrl.SaveTexture(save,str(stem.with_suffix('.png')))
            (directory/(phase+'-state.json')).write_text(json.dumps(state,indent=2)+'\n')
        progress(f"frame {frame['ordinal']} exported")
    report=dict(scope=__doc__,capture_sha256=digest(capture),reference_sha256=digest(reference),
                tool_sha256=digest(Path(os.environ['FX_TOOL'])),frames=frames,native_framebuffer_parity=False)
    if selection:
        report.update(selection_sha256=digest(selection),selection=json.loads(selection.read_text()))
    (OUT/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    progress('original frame export complete')
except Exception:
    (OUT/'error.txt').write_text(traceback.format_exc())
os._exit(0)
