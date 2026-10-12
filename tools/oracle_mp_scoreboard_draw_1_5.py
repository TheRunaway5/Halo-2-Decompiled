#!/usr/bin/env python3
"""Original scoreboard pulse and font6 text draw leaves.

The frozen original layout row/widget bytes, literal conduit-13 header/glyphs,
clock and full-renderer text-state values are declared kernel inputs. Ready font
lookup/pixel residency is an explicit provider, not recovered font startup.
Void rectangle/glyph submission records the requested geometry and returns with
EAX unchanged; no GPU, world, readiness or gameplay result is supplied.
"""
import argparse, hashlib, json, struct, copy
from pathlib import Path
from unicorn.x86_const import *
from oracle_mp_full_scoreboard_1_5 import Trace, ROOT, NEW, NEW_PIN, HudCache, F, C, S, literal, sha, Xbe
from oracle_smg_firing_1_1 import _Code
from research_mp_lobby_later_update import complete_tokens, references

OUT = ROOT / 'analysis/multiplayer/title-update-1.5/scoreboard-painter'
LAYOUT = ROOT / 'analysis/multiplayer/title-update-1.5/full-scoreboard/measured/report.json'
LAYOUT_PIN = 'a0eecfb5dd9752c4d1185e64215aa569cf63b27b81c786664df2b81015c8fdf2'
ROW, FONT, POOL, GLYPHS, TEXT, RECT = [F + n for n in (0x60000, 0x64000, 0x66000, 0x68000, 0x70000, 0x74000)]

class DrawTrace(Trace):
 def __init__(self, cache):
  super().__init__(cache); self.draws=[]; self.rectangles=[]; self.chars=[]
 def returning_void(self, cleanup):
  sp=self.g.reg_read(UC_X86_REG_ESP)
  self.services.append(dict(phase=self.phase,entry=f'{self.g.reg_read(UC_X86_REG_EIP):08x}',registers=self.reg(),stack64_hex=bytes(self.g.mem_read(sp,64)).hex(),callee_stack_cleanup=cleanup,boundary='declared ready glyph/pixel lookup or void draw submission; see oracle contract'))
  self.g.reg_write(UC_X86_REG_EIP,self.u(sp));self.g.reg_write(UC_X86_REG_ESP,sp+4+cleanup)
 def hook(self,g,a,n,user):
  if a==0x1640dd and self.block==a:
   self.stopped=dict(kind='bounded_background_fragment_end_before_text_state',entry=f'{a:08x}',registers=self.reg());g.emu_stop();return
  if a == 0x36870:
   sp=g.reg_read(UC_X86_REG_ESP);p=self.u(sp+4);color=g.reg_read(UC_X86_REG_EAX)
   self.rectangles.append(dict(rectangle8_hex=bytes(g.mem_read(p,8)).hex(),color16_hex=bytes(g.mem_read(color,16)).hex()))
   self.returning_void(4);return
  if a == 0x143620:
   ch=g.reg_read(UC_X86_REG_ECX)
   g.reg_write(UC_X86_REG_EAX,self.chars.index(ch) if ch in self.chars else 0xffffffff)
   self.returning_void(0);return
  if a==0x143370:
   g.reg_write(UC_X86_REG_EAX,1);self.returning_void(4);return
  if a==0x36390:
   sp=g.reg_read(UC_X86_REG_ESP);p=self.u(sp+4)
   self.draws.append(bytes(g.mem_read(p,80)).hex());self.returning_void(4);return
  super().hook(g,a,n,user)

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'measured/report.json');args=ap.parse_args();args.output.parent.mkdir(parents=True,exist_ok=True)
 assert sha(NEW)==NEW_PIN and sha(LAYOUT)==LAYOUT_PIN
 source=json.loads(LAYOUT.read_text());raw=bytes.fromhex(source['full_layout']['output']['raw293c_hex']);cache=HudCache(ROOT/'assets/disc/maps/lockout.map');t=DrawTrace(cache)
 row=raw[0xcb8+0x1c8:0xcb8+2*0x1c8] # actual current player Ada row
 assert row[0x1c4]==1
 t.g.mem_write(ROW,row);t.w(0x51df54,C)
 pulse=[]
 for tick in (0,1,7,15,30,45,60,101,-1,2147483647):
  for rate in (30,60):
   t.h(C+2,rate);t.w(C+8,tick);t.rectangles=[]
   call=t.run(f'pulse_{tick}_{rate}',0x162a00,regs=((UC_X86_REG_EAX,ROW),));assert call['completion']=='complete_original_return'
   assert len(t.rectangles)==4
   pulse.append(dict(clock12_hex=bytes(t.g.mem_read(C,12)).hex(),row1c8_hex=row.hex(),call=call,rectangles=t.rectangles))
 # Exact existing produced layout in the renderer's original stack offsets.
 # This is the post-36570 fragment, not a successful substitute for GPU begin.
 t.g.mem_write(S-0x294c+0x10,raw);t.g.mem_write(C,bytes.fromhex(source['explicit_layout_inputs']['raw_records']['clock']));t.rectangles=[]
 backgrounds=t.run('original_post_begin_widget_background_quality_pulse_fragment',0x163ef5,regs=((UC_X86_REG_EBX,0),),tail=0x294c,block=0x1640dd)
 assert backgrounds['completion']=='stopped_original_instruction' and backgrounds['stop']['kind']=='bounded_background_fragment_end_before_text_state'
 backgrounds.update(rectangles=t.rectangles,source_layout=source['full_layout']['output'],clock12_hex=bytes(t.g.mem_read(C,12)).hex(),boundary='original post-36570 stack/register fragment; full begin/context admission, emblems, and GPU submission remain unavailable')
 # Exact ready font cache API boundary; header and glyph records are original.
 font=(ROOT/'assets/disc/maps/fonts/conduit-13').read_bytes();t.g.mem_write(FONT,font[0x200:0x3c0]);t.g.mem_write(FONT+0x1c8,b'\1')
 # Actual font lookup124EB0 executes: selector6 resolves this explicit ready cache.
 t.g.mem_write(0x4ef1f0+6*0x1d0,font[0x200:0x3c0]);t.g.mem_write(0x4ef1f0+6*0x1d0+0x1c8,b'\1');t.w(0x4ef1c4+6*4,6)
 t.w(POOL+0x44,GLYPHS);t.w(0x55c404,POOL)
 t.w(0x4f3c70,6);t.w(0x4f3c78,0xffffffff);t.w(0x4f3c74,0)
 t.g.mem_write(0x4f3c90,b'\1');t.g.mem_write(0x4f3c94,t.image.read(0x44aaf8,16))
 t.g.mem_write(0x4c2ac0,b'\1');t.w(0x4c2acc,FONT);t.g.mem_write(FONT+4,struct.pack('<2h',256,256));t.g.mem_write(FONT+0xe,b'\0')
 t.g.mem_write(0x4c65f0,struct.pack('<4h',0,0,480,640));t.g.mem_write(0x4c65f8,struct.pack('<4h',24,32,456,608))
 cases=[]
 widgets=source['full_layout']['output']['players']
 inputs=[(f"{r['index']}_{name}", w) for r in widgets for name,w in r['widgets'].items() if name in ('place','name','wins','score') and w['valid']]
 for label,which,text in [('long_name_clip','name','W'*19),('center_fractional_clip','place','WWWWi'),('literal_LF_ignored','name','A\nB')]:
  w=copy.deepcopy(widgets[0]['widgets'][which]);w['text']=text;w['text_utf16_hex']=(text+'\0').encode('utf-16le').hex();w['text_stimulus_boundary']='explicit text API stimulus in an original produced widget, not a source player-name producer';inputs.append((label,w))
 for label,w in inputs:
  text=w['text'];t.chars=list(dict.fromkeys(map(ord,text)))
  for index,ch in enumerate(t.chars):
   p=GLYPHS+index*0x38;t.w(p+0x10,4);offset=(struct.unpack_from('<I',font,0x400+ch*4)[0]+0x4040)*16
   t.g.mem_write(p+0x1c,font[offset:offset+16]);t.w(p+0x34,0);t.w(p+0x2c,0)
  t.g.mem_write(RECT,struct.pack('<4h',*w['text_rectangle']));t.g.mem_write(TEXT,struct.pack('<'+str(len(text)+1)+'I',*map(ord,text),0))
  t.w(0x4f3c7c,2 if w['right_justified'] else 0);t.g.mem_write(ROW,bytes.fromhex(''.join(bytes.fromhex(v)[::-1].hex() for v in w['text_color_bits'])))
  color=t.run('text_color_'+label,0x141750,regs=((UC_X86_REG_EAX,ROW),));t.draws=[]
  # Direct downstream text leaf; original1F050 graphics/context admission is
  # not replaced with a successful begin/end. For these bar-free strings the
  # literal UTF16->DWORD code-unit delivery is an explicit caller input.
  call=t.run('font6_text_'+label,0x141bc0,(0x1f470,RECT,0,RECT,0,0x3f800000,TEXT))
  assert call['completion']=='complete_original_return',(label,call['stop'])
  cases.append(dict(label=label,widget=w,color_call=color,call=call,quads80_hex=t.draws))
 # Separately retain the real outer renderer dependency instead of returning
 # success from GPU/context begin. No downstream leaf is executed afterwards.
 t.g.mem_write(TEXT,b'A\0\0\0');outer=t.run('actual_UTF16_outer_draw_dependency_stop',0x1fa30,(RECT,0,0,0,0x3f800000,0,0),((UC_X86_REG_ECX,TEXT),))
 assert outer['completion']=='stopped_original_instruction'
 instructions=args.output.parent/'instructions';instructions.mkdir(exist_ok=True);manifest={}
 for e in sorted(t.entries|{0x162a00,0x163ea0,0x36870,0x1fa30,0x141bc0,0x141a20,0x142080,0x141f50,0x1421e0,0x1f470,0x1f380,0x36390}):
  _,ins=literal(t.code,e,t.starts);p=instructions/f'{e:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins));manifest[p.name]=sha(p)
 old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');oldcode=_Code(old,'.text');pairs=[]
 for old_entry,new_entry in ((0x1403e0,0x141750),(0x1402d0,0x141640),(0x140850,0x141bc0),(0x1406b0,0x141a20),(0x140be0,0x141f50),(0x140e70,0x1421e0),(0x140d10,0x142080),(0x140dd0,0x142140),(0x1f470,0x1f470),(0x1f380,0x1f380),(0x161620,0x162a00)):
  _,oi=literal(oldcode,old_entry,set());_,ni=literal(t.code,new_entry,t.starts);assert complete_tokens(oi,old)==complete_tokens(ni,t.image)
  p=instructions/f'TU11-{old_entry:08x}.asm';p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n'for i in oi));manifest[p.name]=sha(p)
  pairs.append(dict(old_entry=f'{old_entry:08x}',u5_entry=f'{new_entry:08x}',instructions=len(oi),complete_shape_and_internal_branches_equal=True,relocated_data_operands=references(oi,ni,old,t.image)))
 (args.output.parent/'instruction-manifest.json').write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
 out=dict(schema='bounded-original-scoreboard-draw-v1',oracle_sha256=sha(__file__),source_xbe_sha256=NEW_PIN,layout_report_sha256=LAYOUT_PIN,font_sha256=hashlib.sha256(font).hexdigest(),boundaries=__doc__,pulse_cases=pulse,background_fragment=backgrounds,text_cases=cases,outer_draw_dependency_stop=outer,instruction_manifest=manifest,complete_TU11_pairs=pairs,style_words={f'{a:08x}':t.image.read(a,n).hex() for a,n in((0x46a438,4),(0x46a2ec,4),(0x46a0d0,4),(0x46a0cc,4),(0x46a148,4),(0x44aaf8,16))},TU11_style_words={f'{a:08x}':old.read(a,n).hex()for a,n in((0x465398,4),(0x46524c,4),(0x465030,4),(0x46502c,4),(0x4650a8,4),(0x447050,16),(0x46fb74,4),(0x46fbb0,4),(0x447034,12))},font_record1c0_hex=font[0x200:0x3c0].hex(),instruction_patches=[])
 args.output.write_text(json.dumps(out,indent=2,sort_keys=True)+'\n');assert sha(LAYOUT)==LAYOUT_PIN
 print(json.dumps(dict(pulse_cases=len(pulse),text_cases=len(cases),glyph_quads=sum(len(c['quads80_hex']) for c in cases))))
if __name__=='__main__':main()
