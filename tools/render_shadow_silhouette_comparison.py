#!/usr/bin/env python3
"""Plot untouched original/native shadow RGB with nearest pixel enlargement."""
import argparse,hashlib,json
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/multiplayer-shading'

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--filtered',action='store_true');args=parser.parse_args()
 prefix='shadow-filter'if args.filtered else'shadow-silhouette'
 original=(ROOT/'tests/fixtures'/('retail_shadow_filtered_expected.bgra8'if args.filtered else'retail_shadow_silhouette_expected.bgra8')).read_bytes()
 native=(OUT/(prefix+'-native.bgra8')).read_bytes()
 assert len(original)==len(native)==128*128*4
 images=[]
 for label,data in [('original',original),('native',native)]:
  image=Image.frombytes('RGBA',(128,128),data,'raw','BGRA')
  image.save(OUT/f'{prefix}-{label}-128.png');images.append(image)
 mismatches=sum(original[i:i+4]!=native[i:i+4]for i in range(0,len(native),4))
 differing=sum(a!=b for a,b in zip(original,native))
 black=sum(original[i:i+3]==b'\0\0\0'for i in range(0,len(original),4))
 try:
  font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',27)
  title=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',34)
  small=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf',21)
 except OSError:font=title=small=ImageFont.load_default()
 canvas=Image.new('RGB',(1120,708),'#121820');draw=ImageDraw.Draw(canvas)
 draw.text((35,24),'Original filtered shadow replay'if args.filtered else'Original shadow silhouette replay',font=title,fill='white')
 draw.text((35,72),'Retained original capture and native Vulkan fixture',font=small,fill='#b8c4d1')
 for x,image,label in zip((35,573),images,('Original capture','Native Vulkan')):
  draw.text((x,115),label,font=font,fill='white')
  # Plot actual RGB, nearest only. Alpha remains unmodified in raw128 PNGs
  # and participates in the full byte comparison reported below.
  canvas.paste(image.convert('RGB').resize((512,512),Image.Resampling.NEAREST),(x,156))
 draw.text((35,678),f'{mismatches} differing pixels | RGBA all {65536-differing}/65536 bytes match | 4x nearest view',font=small,fill='#9df1c2')
 filename=prefix+'-comparison.png';canvas.save(OUT/filename)
 report=dict(comparison_png=filename,original_png=prefix+'-original-128.png',native_png=prefix+'-native-128.png',source_dimensions=[128,128],black_pixels=black,differing_pixels=mismatches,differing_bytes=differing,original_sha256=hashlib.sha256(original).hexdigest(),native_sha256=hashlib.sha256(native).hexdigest(),display='RGB shown with exact nearest4x scaling; raw PNGs preserve source alpha',boundary='Explicit retained seven-draw fixture'+(' and original two-pass filter'if args.filtered else'')+', not production caster selection or live gameplay')
 (OUT/(prefix+'-comparison.json')).write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report))
if __name__=='__main__':main()
