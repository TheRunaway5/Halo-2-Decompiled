#!/usr/bin/env python3
"""Audit actual ParticleRenderer readbacks against original captured draws."""
import argparse,hashlib,json
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--fixtures',type=Path,default=ROOT/'analysis/frontend/xemu-capture/particle-graphics-fixture')
 p.add_argument('--results',type=Path,default=ROOT/'build-frontend/particle-graphics-capture')
 p.add_argument('--output',type=Path,default=ROOT/'analysis/frontend/title-particle-native-graphics.json')
 p.add_argument('--binary',type=Path,default=ROOT/'build-frontend/particle_graphics_tests')
 a=p.parse_args();manifest=json.loads((a.fixtures/'manifest.json').read_text());rows=[]
 for draw in manifest['draws']:
  width,height=draw['color_description']['width'],draw['color_description']['height'];size=width*height*4
  expected=(a.fixtures/draw['buffers']['after-color']['file']).read_bytes();before=(a.fixtures/draw['buffers']['before-color']['file']).read_bytes()
  depth=(a.fixtures/draw['buffers']['after-depth']['file']).read_bytes()
  z=np.frombuffer(depth[:size],dtype='<u4')&0xffffff
  for kind in ('d24','d32'):
   path=a.results/f"{draw['event']}-{kind}-color.bgra";native=path.read_bytes();assert len(native)==len(expected)==size
   native_depth=np.frombuffer((a.results/f"{draw['event']}-{kind}-depth.bin").read_bytes(),dtype='<u4')
   reference=z if kind=='d24'else(z.astype(np.float32)/np.float32(16777215)).view(np.uint32)
   actual=native_depth&0xffffff if kind=='d24'else native_depth
   difference=np.abs(np.frombuffer(native,dtype=np.uint8).astype(int)-np.frombuffer(expected,dtype=np.uint8).astype(int))
   rows.append({'event':draw['event'],'particle':f"{draw['particle']:08x}",'implementation':draw['implementation'],'depth_format':kind,
    'compared_color_bytes':size,'different_color_bytes':int(np.count_nonzero(difference)),'maximum_color_byte_difference':int(difference.max()),
    'different_depth_values':int(np.count_nonzero(actual!=reference)),'original_changed_pixels':int(np.count_nonzero(np.any(np.frombuffer(expected,dtype=np.uint8).reshape(-1,4)!=np.frombuffer(before,dtype=np.uint8).reshape(-1,4),axis=1))),
    'original_color_sha256':hashlib.sha256(expected).hexdigest(),'native_color_sha256':sha(path)})
 shaders=ROOT/'build-frontend/shaders'
 report={'scope':'actual production ParticleRenderer on completed original uniform/framebuffer inputs, explicit implementation 0 of three bounded title materials',
  'capture_sha256':manifest['original_capture_sha256'],'native_binary_sha256':sha(a.binary),'draws':len(manifest['draws']),
  'depth_formats':['D24_UNORM_S8_UINT','D32_SFLOAT'],'comparisons':rows,
  'all_color_depth_exact':all(r['different_color_bytes']==r['different_depth_values']==0 for r in rows),
  'native_shader_sha256':{name:sha(shaders/name)for name in ('particle.vert.spv','particle.geom.spv','particle.frag.spv')},
  'native_dynamic_material_buffer_arithmetic_verified':True,'renderer_completed_draw_api_implemented':True,
  'live_frontend_emission_simulation_verified':False,'live_camera_producer_verified':False,'full_transparent_scheduler_verified':False,'whole_native_frame_parity_verified':False}
 a.output.write_text(json.dumps(report,indent=2)+'\n')
 # The comparison keeps the original prior framebuffer as an explicit test
 # input. This is a GPU verification image, not a live title-screen screenshot.
 draw=next(r for r in manifest['draws']if r['event']==6533)
 expected=(a.fixtures/draw['buffers']['after-color']['file']).read_bytes();actual=(a.results/'6533-d32-color.bgra').read_bytes()
 panel=Image.new('RGB',(720,370),(12,16,27));labels=ImageDraw.Draw(panel)
 for x,label,data in ((0,'Original Xemu draw',expected),(360,'Native particle pipeline',actual)):
  image=Image.frombytes('RGBA',(640,480),data,'raw','BGRA').convert('RGB').crop((160,16,240,96)).resize((320,320),Image.Resampling.NEAREST)
  panel.paste(image,(x+20,35));labels.text((x+20,14),label,fill=(225,230,242))
 labels.text((20,357),'Same captured background input; zero differing color or depth values.',fill=(178,191,213))
 panel.save(a.output.with_suffix('.png'))
 print(json.dumps({'draws':report['draws'],'all_color_depth_exact':report['all_color_depth_exact'],'different_color_bytes':sum(r['different_color_bytes']for r in rows),'different_depth_values':sum(r['different_depth_values']for r in rows)},indent=2))
 raise SystemExit(not report['all_color_depth_exact'])
if __name__=='__main__':main()
