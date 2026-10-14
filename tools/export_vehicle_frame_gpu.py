"""qrenderdoc --script: inspect original vehicle pixel history and draw inputs."""
import hashlib,json,math,os,traceback
from pathlib import Path
import renderdoc as rd
OUT=Path(os.environ['VEHICLE_GPU_OUT']);OUT.mkdir(parents=True,exist_ok=False)
def progress(s):
    with (OUT/'progress.txt').open('a') as f:f.write(s+'\n')
def plain(v,depth=0):
    if isinstance(v,float) and not math.isfinite(v):return None
    if v is None or isinstance(v,(str,int,float,bool)):return v
    if isinstance(v,rd.ResourceId) or depth>12:return str(v)
    if isinstance(v,(list,tuple)) or type(v).__name__.startswith('rdcarray'):return [plain(x,depth+1) for x in v]
    out={}
    for k in dir(v):
        if k.startswith('_') or k in ('this','thisown'):continue
        try:
            x=getattr(v,k)
            if not callable(x):out[k]=plain(x,depth+1)
        except Exception:pass
    return out or str(v)
def flatten(actions):
    for a in actions:yield a;yield from flatten(a.children)
try:
    capture=Path(os.environ['VEHICLE_GPU_RDC']);progress('opening original capture')
    handle=rd.OpenCaptureFile();assert handle.OpenFile(str(capture),'',None)==rd.ResultCode.Succeeded
    options=rd.ReplayOptions();options.optimisation=rd.ReplayOptimisationLevel.Fastest
    result,ctrl=handle.OpenCapture(options,None);assert result==rd.ResultCode.Succeeded
    actions=list(flatten(ctrl.GetRootActions()));draws=[a for a in actions if a.flags&rd.ActionFlags.Drawcall]
    textures={str(t.resourceId):t for t in ctrl.GetTextures()}
    (OUT/'actions.json').write_text(json.dumps([dict(event=a.eventId,name=str(a.customName),indices=a.numIndices,flags=str(a.flags)) for a in actions],indent=2)+'\n')
    (OUT/'textures.json').write_text(json.dumps([plain(t) for t in textures.values()],indent=2)+'\n')
    progress(f'opened {len(actions)} actions, {len(draws)} draws')
    if os.environ.get('VEHICLE_GPU_COPIES_ONLY'):
        copies=[dict(event=a.eventId,name=str(a.customName),source=str(a.copySource),destination=str(a.copyDestination)) for a in actions
            if str(a.copyDestination) in ('ResourceId::51399','ResourceId::51388')]
        (OUT/'copies.json').write_text(json.dumps(copies,indent=2)+'\n')
        progress(f'exported {len(copies)} source/display copy links');os._exit(0)
    ctrl.SetFrameEvent(draws[-1].eventId,True);pipe=ctrl.GetPipelineState();targets=list(pipe.GetOutputTargets());assert targets
    color=targets[0].resource
    if os.environ.get('VEHICLE_GPU_TARGET'):
        color=next(t.resourceId for t in textures.values() if str(t.resourceId)=='ResourceId::'+os.environ['VEHICLE_GPU_TARGET'])
        ctrl.SetFrameEvent(int(os.environ['VEHICLE_GPU_EVENT']),True)
    (OUT/'final-target.json').write_text(json.dumps(plain(textures[str(color)]),indent=2)+'\n')
    save=rd.TextureSave();save.resourceId=color;save.destType=rd.FileType.PNG;save.mip=0;ctrl.SaveTexture(save,str(OUT/'final.png'))
    if os.environ.get('VEHICLE_GPU_BINDINGS_ONLY'):
        for action in draws[-2:]:
            ctrl.SetFrameEvent(action.eventId,True);pipe=ctrl.GetPipelineState();directory=OUT/f'event-{action.eventId}';directory.mkdir()
            bindings=pipe.GetReadOnlyResources(rd.ShaderStage.Fragment)
            (directory/'bindings.json').write_text(json.dumps(plain(bindings),indent=2)+'\n')
            for stage in (rd.ShaderStage.Vertex,rd.ShaderStage.Fragment):
                reflection=pipe.GetShaderReflection(stage)
                if reflection:(directory/(str(stage)+'.glsl')).write_bytes(bytes(reflection.rawBytes))
            for bound in bindings:
                resource=bound.descriptor.resource
                if str(resource) not in textures:continue
                stem=directory/str(resource).replace('::','-')
                (stem.with_suffix('.json')).write_text(json.dumps(plain(textures[str(resource)]),indent=2)+'\n')
                save.resourceId=resource;ctrl.SaveTexture(save,str(stem.with_suffix('.png')))
        progress('final compositor source bindings exported; no pixel parity claim')
        os._exit(0)
    histories=[]
    explicit_events=os.environ.get('VEHICLE_GPU_EVENTS')
    progress('querying vehicle pixel history')
    history_events=set()
    pixels=[tuple(int(v) for v in pair.split(',')) for pair in os.environ.get('VEHICLE_GPU_PIXELS','280,190;280,290').split(';')]
    for x,y in ([] if explicit_events else pixels):
        history=ctrl.PixelHistory(color,x,y,rd.Subresource(),rd.CompType.Typeless)
        histories.append(dict(x=x,y=y,events=plain(history)))
        history_events.update(h.eventId for h in history)
    (OUT/'pixel-history.json').write_text(json.dumps(histories,indent=2)+'\n')
    events=[int(e) for e in explicit_events.split(',')] if explicit_events else sorted(history_events)
    progress(f'exporting {len(events)} original contributing events')
    for event in events:
        ctrl.SetFrameEvent(event,True);pipe=ctrl.GetPipelineState();directory=OUT/f'event-{event}';directory.mkdir()
        data=dict(event=event,pipeline=plain(ctrl.GetGLPipelineState() if ctrl.GetAPIProperties().pipelineType==rd.GraphicsAPI.OpenGL else ctrl.GetVulkanPipelineState()),vertex_inputs=plain(pipe.GetVBuffers()),index_input=plain(pipe.GetIBuffer()))
        if explicit_events:
            data['samplers']=plain(pipe.GetSamplers(rd.ShaderStage.Fragment));data['textures']=[]
            for used in pipe.GetReadOnlyResources(rd.ShaderStage.Fragment):
                resource=used.descriptor.resource;texture=textures.get(str(resource))
                if not texture or used.access.staticallyUnused:continue
                item=dict(binding=plain(used),description=plain(texture),files=[])
                for face in range(max(1,texture.arraysize)):
                    for mip in range(texture.mips):
                        sub=rd.Subresource();sub.mip=mip;sub.slice=face
                        name=f'texture-{str(resource).split("::")[-1]}-face{face}-mip{mip}.bin'
                        raw=bytes(ctrl.GetTextureData(resource,sub));(directory/name).write_bytes(raw);item['files'].append(dict(name=name,sha256=hashlib.sha256(raw).hexdigest(),size=len(raw),face=face,mip=mip))
                data['textures'].append(item)
            mesh=ctrl.GetPostVSData(0,0,rd.MeshDataStage.VSOut);data['postvs']=plain(mesh)
            if mesh.indexResourceId!=rd.ResourceId.Null():
                (directory/'postvs-indices.bin').write_bytes(bytes(ctrl.GetBufferData(mesh.indexResourceId,mesh.indexByteOffset,mesh.numIndices*mesh.indexByteStride)))
            if mesh.vertexResourceId!=rd.ResourceId.Null():
                (directory/'postvs.bin').write_bytes(bytes(ctrl.GetBufferData(mesh.vertexResourceId,mesh.vertexByteOffset,mesh.numIndices*mesh.vertexByteStride)))
        for stage in (rd.ShaderStage.Vertex,rd.ShaderStage.Fragment):
            reflection=pipe.GetShaderReflection(stage)
            if reflection is None:continue
            data[str(stage)+'_outputs']=plain(reflection.outputSignature)
            (directory/(str(stage)+'.glsl')).write_bytes(bytes(reflection.rawBytes));shader=pipe.GetShader(stage);uniforms=[]
            for i,block in enumerate(reflection.constantBlocks):
                binding=pipe.GetConstantBlock(stage,i,0).descriptor
                uniforms.append(dict(block=plain(block),contents=plain(ctrl.GetCBufferVariableContents(pipe.GetGraphicsPipelineObject(),shader,stage,pipe.GetShaderEntryPoint(stage),i,binding.resource,binding.byteOffset,binding.byteSize))))
            data[str(stage)]=uniforms
        (directory/'state.json').write_text(json.dumps(data,indent=2)+'\n')
    (OUT/'manifest.json').write_text(json.dumps(dict(capture=str(capture),capture_sha256=hashlib.file_digest(capture.open('rb'),'sha256').hexdigest(),events=events,pipeline_api=str(ctrl.GetAPIProperties().pipelineType),original_only=ctrl.GetAPIProperties().pipelineType==rd.GraphicsAPI.OpenGL,framebuffer_parity=False),indent=2)+'\n')
    progress('original vehicle GPU export complete')
except Exception:(OUT/'error.txt').write_text(traceback.format_exc())
os._exit(0)
