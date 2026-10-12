#!/usr/bin/env python3
"""Compare native Lockout at a retained original effects frame's exact camera.

Only the captured camera/projection and output extent are restored. Original
player/weapon/effect state, simulation clock, clear and target history are not
restored. This diagnostic pair is not matched effects or framebuffer parity.
"""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--native',type=Path,required=True);p.add_argument('--frames',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();a.output.mkdir(parents=True,exist_ok=False)
 header=a.frames/'004-header.bin';original=a.frames/'004.png';raw=header.read_bytes();assert len(raw)==0x298
 assert all((a.frames/f'{i:03}-header.bin').read_bytes()[0x18:0x48]==raw[0x18:0x48]for i in range(1,9)), 'Adjacent original cameras differ'
 camera=struct.unpack_from('<9f',raw,0x18);projection=struct.unpack_from('<2f',raw,0x40);assert all(math.isfinite(f)for f in (*camera,*projection))
 viewport=struct.unpack_from('<4h',raw,0x48);assert viewport==(0,0,480,640)
 native=a.native.resolve();maps=ROOT/'assets/disc/maps';image=a.output/'native.ppm';xbe=ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
 inputs=[native,header,original,maps/'lockout.map',xbe,*sorted((native.parent/'shaders').rglob('*.spv'))];before={str(f):sha(f)for f in inputs}
 command=[str(native),'--multiplayer','--map',str(maps/'lockout.map'),'--shared-maps',str(maps),'--gameplay-xbe',str(xbe),'--mp-starting-weapon','5','--local-players','1','--viewport','640','480','--hidden','--mute','--validation','--frames','3','--mp-reference-camera',*[repr(f)for f in camera],'--mp-reference-projection',*[repr(f)for f in projection],'--screenshot',str(image.resolve())]
 with (a.output/'native.log').open('w')as f:r=subprocess.run(command,cwd=ROOT,env=os.environ.copy(),stdout=f,stderr=subprocess.STDOUT,timeout=120)
 assert r.returncode==0,'Native camera capture failed';log=(a.output/'native.log').read_text();assert 'Validation Error'not in log and 'ERROR / SPEC'not in log
 for path,digest in before.items():assert sha(Path(path))==digest,'Input changed during camera capture'
 with Image.open(image)as im:current=im.convert('RGB');assert current.size==(640,480);current.save(a.output/'native.png')
 with Image.open(original)as im:source=im.convert('RGB');assert source.size==current.size
 comparison=Image.new('RGB',(1280,542),'#181818');comparison.paste(source,(0,30));comparison.paste(current,(640,30));label=ImageDraw.Draw(comparison)
 label.text((8,8),'Original Lockout effects frame 004',fill='white');label.text((648,8),'Native: same retained camera/projection',fill='white');label.text((8,520),'Camera only. Weapon/effect/tick/clear/target history differ. Full parity is NOT established.',fill='white');comparison.save(a.output/'comparison.png')
 report=dict(boundary=__doc__,command=command,input_sha256=before,camera_word_hex=[f'{v:08x}'for v in struct.unpack_from('<12I',raw,0x18)],adjacent_original_camera_matches=8,camera_restored=True,original_effect_state_restored=False,original_clear_history_restored=False,matched_effects_frame=False,framebuffer_parity=False,validation_errors=0,output_sha256={f.name:sha(f)for f in a.output.iterdir()if f.is_file()})
 (a.output/'provenance.json').write_text(json.dumps(report,indent=2)+'\n');print('PASS retained original camera/projection diagnostic pair; effect/weapon/history parity remains open')
if __name__=='__main__':main()
