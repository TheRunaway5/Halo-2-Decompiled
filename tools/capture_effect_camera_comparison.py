#!/usr/bin/env python3
"""Camera-aligned original/native diagnostic, with effect-state limits explicit.

Camera vectors, vertical angle and lighting scale come from the saved132D0
header. Native simulation, first-person pose, effects and render history are
not restored. A screenshot at a render-entry stop may show the preceding
presented frame. This comparison is not matched explosion-state acceptance.
"""
import argparse,hashlib,json,os,shutil,struct,subprocess
from pathlib import Path
from PIL import Image,ImageDraw

ROOT=Path(__file__).resolve().parents[1]
def sha(path):
 with path.open('rb')as f:return hashlib.file_digest(f,'sha256').hexdigest()

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('native','map','header','original-frame','output'):p.add_argument('--'+name,type=Path,required=True)
 p.add_argument('--gameplay-xbe',type=Path,default=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03')
 a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 raw=a.header.read_bytes();assert len(raw)==0x298
 camera=struct.unpack_from('<9f',raw,0x18);projection=struct.unpack_from('<2f',raw,0x40)
 original=Image.open(a.original_frame).convert('RGB');width,height=original.size
 command=[str(a.native.resolve()),'--multiplayer','--map',str(a.map.resolve()),'--shared-maps',str(a.map.resolve().parent),
  '--gameplay-xbe',str(a.gameplay_xbe.resolve()),'--mp-starting-weapon','5','--local-players','1','--viewport',str(width),str(height),
  '--mp-reference-camera',*[repr(x)for x in camera],'--mp-reference-projection',*[repr(x)for x in projection],
  '--frames','1','--hidden','--mute','--validation','--screenshot',str((a.output/'native.ppm').resolve())]
 files=[a.native,a.map,a.header,a.original_frame,a.gameplay_xbe,*sorted((a.native.parent/'shaders').rglob('*.spv'))]
 before={str(f.resolve()):sha(f)for f in files}
 frozen=a.output/'frozen';frozen.mkdir();shutil.copy2(a.native,frozen/a.native.name);shutil.copytree(a.native.parent/'shaders',frozen/'shaders')
 shutil.copy2(a.header,a.output/'original-header.bin');original.save(a.output/'original.png')
 with(a.output/'native.log').open('w')as f:result=subprocess.run(command,cwd=ROOT,env=os.environ.copy(),stdout=f,stderr=subprocess.STDOUT,timeout=120)
 log=(a.output/'native.log').read_text();errors=[line for line in log.splitlines()if'Validation Error'in line or'ERROR / SPEC'in line]
 manifest=dict(boundary=__doc__,command=command,camera_words=[*struct.unpack_from('<9I',raw,0x18),*struct.unpack_from('<2I',raw,0x40)],
  camera=camera,projection=projection,input_sha256=before,exit_code=result.returncode,validation_errors=errors,
  effects_matched=False,first_person_pose_matched=False,render_history_matched=False)
 (a.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
 if result.returncode or errors:raise RuntimeError('Native camera replay failed; retained native.log')
 assert before=={str(f.resolve()):sha(f)for f in files},'Capture inputs changed'
 native=Image.open(a.output/'native.ppm').convert('RGB');assert native.size==original.size;native.save(a.output/'native.png')
 sheet=Image.new('RGB',(width*2,height+64),(18,18,22));draw=ImageDraw.Draw(sheet)
 draw.text((12,8),'ORIGINAL: retained Lockout gameplay',fill='white');draw.text((width+12,8),'NATIVE: captured camera/projection replay',fill='white')
 sheet.paste(original,(0,28));sheet.paste(native,(width,28))
 draw.text((12,height+38),'Camera aligned. Effects, first-person pose and render history remain unmatched.',fill=(255,205,100))
 sheet.save(a.output/'comparison.png')
 manifest['output_sha256']={f.name:sha(f)for f in a.output.iterdir()if f.is_file()and f.name!='manifest.json'}
 (a.output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n');print('Camera-aligned comparison saved; appearance acceptance remains open')
if __name__=='__main__':main()
