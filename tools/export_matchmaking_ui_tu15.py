#!/usr/bin/env python3
"""Export the unchanged Xbox matchmaking/PCR widget definitions for native UI.

Run under tools/run_test.py. Source-authenticated logical definitions retain
actual admitted MCC artwork through explicit name/widget/scenario role joins.
No DVD bitmap/font/emblem pixels are included in the runtime payload.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import shutil
import zlib

from audit_rendering import Cache, file_sha
from import_mcc_hud import MccCache
from audit_dlc_map_previews_tu15 import DEFAULT_ROOT as DLC_ROOT, export_dlc_map_preview_descriptors

ROOT=Path(__file__).resolve().parents[1]
XBOX_PIN='1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
SCREENS=(7,8,14,16,23,25,185,210,219,237)

def original_frames(cache, name):
    """The same first raw-data extent and descriptor flags as Cache UI load."""
    tag=next(t for t in cache.tags.values() if t['group']=='bitm' and t['name']==name)
    _,p=cache.tag(tag['datum'],'bitm');frames=[]
    for at in cache.block(p+68,116):
        pointer,size=cache.u32(at+28),cache.u32(at+52)
        assert 0<size<=1024*1024
        location=pointer>>30
        owner=cache.path if not location else cache.path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[location]
        with owner.open('rb') as stream:
            stream.seek(pointer&0x3fffffff);pixels=stream.read(size)
        assert len(pixels)==size
        frames.append(dict(width=cache.u16(at+4),height=cache.u16(at+6),format=cache.u16(at+12),flags=cache.u16(at+14),
            bytes=size,sha256=hashlib.sha256(pixels).hexdigest(),zlib_hex=zlib.compress(pixels,9).hex()))
    return dict(frames=frames,descriptor=Definition(cache).image(tag['datum']),source_mainmenu_sha256=XBOX_PIN)

class Definition:
    def __init__(self,cache):
        self.c=cache;self.images=set();self.groups=set()
        self.u16=cache.u16;self.u32=cache.u32
    def values(self,fmt,at):return list(struct.unpack_from('<'+fmt,self.c.data,at))
    def name(self,datum,group):
        if datum==0xffffffff:return ''
        tag=self.c.tags[datum]
        assert tag['group']==group,(hex(datum),group,tag)
        if group=='bitm':self.images.add(datum)
        if group=='skin':self.groups.add(datum)
        return tag['name']
    def text(self,p):
        return dict(flags=self.u32(p),animation=self.u16(p+4),delay=self.u16(p+6),
            font=self.values('h',p+10)[0],color=self.values('4f',p+12),
            bounds=self.values('4h',p+28),string=self.u32(p+36),layer=self.values('h',p+40)[0])
    def bitmap(self,p):
        return dict(flags=self.u32(p),animation=self.u16(p+4),delay=self.u16(p+6),
            blend=self.u16(p+8),frame=self.values('h',p+10)[0],position=self.values('2h',p+12),
            scroll=self.values('2f',p+16),datum=self.name(self.u32(p+28),'bitm'),
            layer=self.values('h',p+32)[0],fps=self.values('f',p+36)[0],
            placeholder=self.u32(p+44),scale=self.values('2f',p+48))
    def grid(self,p):
        return dict(skin=self.name(self.u32(p+8),'skin'),position=self.values('2h',p+12),
            order=self.c.data[p+16],count=self.c.data[p+17],rows=self.c.data[p+18],
            columns=self.c.data[p+19],step=list(reversed(self.values('2h',p+20))))
    def emblem(self,p):
        return dict(flags=self.u32(p),animation=self.u16(p+4),delay=self.u16(p+6),
            layer=self.values('h',p+8)[0],page=self.values('h',p+10)[0],
            bitmap=self.name(self.u32(p+16),'bitm'),shader=self.name(self.u32(p+24),'shad'),
            bounds=self.values('4h',p+28))
    def track(self,duration,p):
        return dict(milliseconds=duration,keys=[dict(alpha=self.values('f',k+4)[0],position=self.values('3f',k+8)) for k in self.c.block(p,20)])
    def skin(self,datum):
        tag,p=self.c.tag(datum,'skin')
        return dict(name=tag['name'],arrows=self.name(self.u32(p+8),'bitm'),arrow_up=self.values('2h',p+12),arrow_down=self.values('2h',p+16),
            animations=[self.track(self.u32(a+4),a+8) for a in self.c.block(p+20,16)],
            texts=[self.text(a) for a in self.c.block(p+28,44)],bitmaps=[self.bitmap(a) for a in self.c.block(p+36,56)],
            emblems=[self.emblem(a) for a in self.c.block(p+44,36)],grids=[self.grid(a) for a in self.c.block(p+52,24)])
    def screen(self,tag):
        _,p=self.c.tag(tag['datum'],'wgit');panes=[]
        for q in self.c.block(p+32,76):
            buttons=[]
            for a in self.c.block(q+4,60):
                text=self.text(a);text.update(string=self.u32(a+48),layer=self.values('h',a+52)[0])
                buttons.append(dict(text=text,bitmap=self.name(self.u32(a+40),'bitm'),offset=self.values('2h',a+44),hit=0,vista_flags=0))
            lists=[]
            for a in self.c.block(q+12,24):
                lists.append(dict(flags=self.u32(a),skin=self.u16(a+4),visible=self.u16(a+6),position=self.values('2h',a+8),animation=self.u16(a+12),delay=self.u16(a+14),labels=[self.u32(i+16) for i in self.c.block(a+16,20)]))
            scenes=[dict(flags=self.u32(a),animation=self.u16(a+4),delay=self.u16(a+6),scale=self.values('3f',a+28),camera=self.values('3f',a+40),fov=self.values('f',a+52)[0],viewport=self.values('4h',a+56),objects=[self.c.string(n,32) for n in self.c.block(a+12,32)],lights=[self.c.string(n,32) for n in self.c.block(a+20,32)]) for a in self.c.block(q+44,76)]
            panes.append(dict(animation=self.u16(q+2),buttons=buttons,lists=lists,
                texts=[self.text(a) for a in self.c.block(q+28,44)],bitmaps=[self.bitmap(a) for a in self.c.block(q+36,56)],
                scenes=scenes,emblems=[self.emblem(a) for a in self.c.block(q+60,36)],grids=[self.grid(a) for a in self.c.block(q+68,24)]))
        return dict(name=tag['name'],id=self.u16(p+4),flags=self.u32(p),button_key=self.u16(p+6),shape=self.u16(p+40),header=self.u32(p+44),color=self.values('4f',p+8),bitmap_resources=[self.name(self.u32(a+4),'bitm') for a in self.c.block(p+56,8)],panes=panes)
    def image(self,datum):
        _,p=self.c.tag(datum,'bitm');stride=168 if isinstance(self.c,MccCache) else 116
        frames=[dict(width=self.u16(a+4),height=self.u16(a+6),format=self.u16(a+12)) for a in self.c.block(p+68,stride)]
        sequences=[]
        for a in self.c.block(p+60,60):
            start,count=self.u16(a+32),self.u16(a+34)
            sprites=self.c.block(a+52,32)
            sequences.append([dict(image=self.u16(b),left=self.values('f',b+8)[0],right=self.values('f',b+12)[0],top=self.values('f',b+16)[0],bottom=self.values('f',b+20)[0],registration=self.values('2f',b+24)) for b in sprites] if sprites else [dict(image=i,left=0,right=1,top=0,bottom=1,registration=[0,0]) for i in range(start,start+count)])
        return dict(frames=frames,sequences=sequences)
    def strings(self,tag):
        _,s=self.c.tag(tag['datum'],'wgit');datum=self.u32(s+28)
        return {} if datum==0xffffffff else self.unicode_strings(datum)
    def unicode_strings(self,datum):
        _,u=self.c.tag(datum,'unic')
        matg=next(t for t in self.c.tags.values() if t['group']=='matg');_,g=self.c.tag(matg['datum'],'matg')
        count,size,indices,data=struct.unpack_from('<4I',self.c.data,g+0x190)
        start,n=self.u16(u+16),self.u16(u+18);assert start+n<=count
        return {str(self.u32(indices+i*8)):self.c.string(data+self.u32(indices+i*8+4),size-self.u32(indices+i*8+4)) for i in range(start,start+n)}
    def global_strings(self):
        # String IDs such as "1" and "header" are reused by local banks.
        # The shared UI owner is this exact UNIC range, never all locales.
        _,g=self.c.tag(next(t['datum'] for t in self.c.tags.values() if t['group']=='wigl'),'wigl')
        return self.unicode_strings(self.u32(g+0x11c))
    def map_images(self):
        _,g=self.c.tag(next(t['datum'] for t in self.c.tags.values() if t['group']=='matg'),'matg')
        levels=self.c.block(g+0x178,24)
        if not levels:return {}
        result={}
        for p in self.c.block(levels[0]+16,0xc64):
            datum=self.u32(p+8)
            if self.u32(p)==0xffffffff or datum==0xffffffff:continue
            path=self.c.string(p+0xb4c,256)
            assert path and path not in result
            result[path]=dict(name=self.c.tags[datum]['name'],image=self.image(datum))
        return result
    def export(self):
        screens={};strings={};background=None;friends_footer=None;friends_footer_strings={}
        for tag in self.c.tags.values():
            if tag['group']!='wgit' or not tag['address']:continue
            _,p=self.c.tag(tag['datum'],'wgit');id=self.u16(p+4)
            if id==5:background=self.screen(tag)
            if id==27:
                friends_footer=self.screen(tag)
                if not isinstance(self.c,MccCache):friends_footer_strings=self.strings(tag)
            if id in SCREENS:
                screens[str(id)]=self.screen(tag)
                if not isinstance(self.c,MccCache):strings[str(id)]=self.strings(tag)
        _,g=self.c.tag(next(t['datum'] for t in self.c.tags.values() if t['group']=='wigl'),'wigl')
        refs=self.c.block(g+0x138,8)
        skins={str(i):self.skin(self.u32(refs[i]+4)) for i in (0,*range(15,21))}
        groups={}
        while self.groups:
            datum=self.groups.pop();name=self.c.tags[datum]['name']
            if name not in groups:groups[name]=self.skin(datum)
        images={self.c.tags[d]['name']:self.image(d) for d in sorted(self.images)}
        keys={}
        if not isinstance(self.c,MccCache):
            ids=(0x0f0006ce,0x110006cf,0x100006d0,0x130006d1,0x110006d2,0x0e0006d3,0x180006d4,0x090006d5,0x080006d6,0x1a0006d7,0x180006d8,0x080006d9,0x080006da,0x230006db,0x250006dc,0x240006dd,0x1c0006de,0x230006df,0x1c0006e0,0x1c0006e1,0x180006e2,0x040006e3)
            bank=self.unicode_strings(self.u32(g+0x144))
            keys={str(s['button_key']):bank.get(str(ids[s['button_key']-1]),'') for s in screens.values() if s['button_key']}
        return dict(screens=screens,background=background,friends_footer=friends_footer,friends_footer_strings=friends_footer_strings,
            skins=skins,groups=groups,images=images,strings=strings,
            global_strings={} if isinstance(self.c,MccCache) else self.global_strings(),map_images=self.map_images(),button_keys=keys)

def main():
    from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--xbox',type=Path,default=ROOT/'assets/disc/maps/mainmenu.map')
    p.add_argument('--mcc',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--report',type=Path,required=True)
    p.add_argument('--cpp-output',type=Path)
    p.add_argument('--publish',action='store_true',help='publish the generated metadata-only profile/include in this checkout')
    p.add_argument('--dlc-root',type=Path,default=DLC_ROOT)
    a=p.parse_args();scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert file_sha(a.xbox)==XBOX_PIN and file_sha(NEW_DEFAULT)==NEW_PIN
    xbox=Cache(a.xbox);mcc=MccCache(a.mcc)
    try:
        original=Definition(xbox).export();raw=Definition(mcc).export()
        available={t['name']:t for t in mcc.tags.values() if t['group']=='bitm'}
        missing=sorted(set(original['images'])-set(available))
        dynamic=['ui\\global_bitmaps\\map_shots\\map_mp_zanzibar']
        embedded={};bindings={};projected={}
        quick='ui\\screens\\game_shell\\settings_screen\\variant_settings\\quick_options_display'
        for name,descriptor in original['images'].items():
            if name in dynamic:continue
            owner=None
            if name==quick:
                # Same waiting-panel widget in the two authenticated screen
                # definitions; this is not a guessed art/frame ordinal.
                before=original['screens']['210']['panes'][1]['bitmaps'][0]
                after=raw['screens']['210']['panes'][1]['bitmaps'][0]
                assert before['datum']==name and before['frame']==0
                mcc_name=after['datum'];frames=[after['frame']]
                owner=dict(screen=210,pane=1,bitmap=0,original=before,mcc=after)
                policy='source'
            else:
                assert name in available,('Missing admitted MCC artwork role',name)
                mcc_name=name
                actual=Definition(mcc).image(available[name]['datum'])
                assert actual['sequences']==descriptor['sequences'],('Different source sprite UVs',name)
                reduced=len(actual['frames'])<len(descriptor['frames'])
                # The raw selected MCC screen retains these same frame fields
                # despite its reduced banks. Its already-qualified logical
                # descriptor and native actual-bank clamp remain authoritative.
                frames=list(range(len(actual['frames'])if reduced else len(descriptor['frames'])))
                policy='retained_mcc'if reduced else 'source'
            projected[mcc_name]=Definition(mcc).image(available[mcc_name]['datum'])
            assert all(0<=frame<len(projected[mcc_name]['frames'])for frame in frames)
            bindings[name]=dict(mcc_name=mcc_name,frames=frames,logical_policy=policy)
            if owner:bindings[name]['widget_owner']=owner
        map_images={path:dict(original=entry,mcc=raw['map_images'][path]) for path,entry in original['map_images'].items() if path in raw['map_images']}
        downloaded=export_dlc_map_preview_descriptors(a.dlc_root,a.xbox)
        for path,entry in downloaded.items():
            if path not in map_images and path in raw['map_images']:
                map_images[path]=dict(original=dict(image=entry['image'],downloaded=entry),mcc=raw['map_images'][path])
        for path,entry in map_images.items():
            assert entry['original']['image']['sequences']==entry['mcc']['image']['sequences'],('Different map sprite UVs',path)
            assert len(entry['original']['image']['frames'])==len(entry['mcc']['image']['frames']),('Different map page count',path)
            alias='ui\\native_og_matchmaking\\map_previews\\'+path
            entry['raster_alias']=alias
            projected[entry['mcc']['name']]=entry['mcc']['image']
        result=dict(schema='H2_TU15_MATCHMAKING_LAYOUT_V1',asset_policy='admitted-mcc-artwork-source-tu15-logical-layout',
            original_mainmenu_sha256=XBOX_PIN,original_xbe_sha256=NEW_PIN,mcc_mainmenu_sha256=file_sha(a.mcc),
            original=original,raw=raw,available_images=projected,embedded_images=embedded,
            mcc_image_bindings=bindings,dynamic_images=dynamic,dynamic_map_images=map_images)
        # ASCII source literals avoid dependence on the compiler execution
        # character set; JSON restores every exact authored Unicode value.
        payload=json.dumps(result,sort_keys=True,separators=(',',':'),ensure_ascii=True)+'\n'
        assert len(original['images'])==63 and len(map_images)==23 and not embedded and len(bindings)==62
        assert original['friends_footer']['id']==27 and len(original['friends_footer']['panes'])==1
        assert len(payload.encode())<=8*1024*1024
        a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(payload)
        if a.cpp_output:
            a.cpp_output.parent.mkdir(parents=True,exist_ok=True)
            encoded=payload.encode();chunks=[];start=0
            while start<len(encoded):
                end=min(start+16384,len(encoded))
                while end<len(encoded) and encoded[end]&0xc0==0x80:end-=1
                chunk=encoded[start:end].decode();assert ')H2OG"' not in chunk
                chunks.append('  std::string_view{R"H2OG('+chunk+')H2OG",'+str(end-start)+'},\n');start=end
            a.cpp_output.write_text('// Generated by export_matchmaking_ui_tu15.py; exact canonical payload in bounded independent literals.\n'
                'inline constexpr std::size_t og_matchmaking_layout_bytes='+str(len(encoded))+';\n'
                'inline constexpr std::string_view og_matchmaking_layout_sha256="'+hashlib.sha256(encoded).hexdigest()+'";\n'
                'inline constexpr std::array<std::string_view,'+str(len(chunks))+'> og_matchmaking_layout_chunks{{\n'+''.join(chunks)+'}};\n')
        if a.publish:
            assert a.cpp_output and a.output.resolve().is_relative_to(scratch)and a.cpp_output.resolve().is_relative_to(scratch)
            shutil.copyfile(a.output,ROOT/'resources/frontend/tu15-matchmaking-layout.json')
            shutil.copyfile(a.cpp_output,ROOT/'src/ui/frontend_og_matchmaking_layout_data.inc')
        unresolved=[name for name in missing if name not in dynamic and name not in bindings]
        report=dict(passed=not unresolved,missing_mcc_images=missing,unresolved_images=unresolved,dynamic_matched_map_images=dynamic,embedded_original_frames={k:sum(f['bytes'] for f in v['frames']) for k,v in embedded.items()},original_mainmenu_sha256=XBOX_PIN,original_xbe_sha256=NEW_PIN,mcc_mainmenu_sha256=result['mcc_mainmenu_sha256'],profile_sha256=hashlib.sha256(payload.encode()).hexdigest(),screen_panes={id:len(s['panes']) for id,s in original['screens'].items()},images=len(original['images']),groups=len(original['groups']),dynamic_map_scenarios=len(map_images),downloaded_scenarios=sum('downloaded' in e['original'] for e in map_images.values()),profile=str(a.output))
        a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
    finally:xbox.close()

if __name__=='__main__':main()
