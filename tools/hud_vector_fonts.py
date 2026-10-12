"""Trace complete donor fonts and retain every original glyph/fallback mapping."""
import hashlib
import shutil
import struct
from pathlib import Path
from collections import defaultdict
from hud_vector_trace import vectorize,decode_glyph,rasterize


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def fnv(data):
    value=0xcbf29ce484222325
    for byte in data:value=((value^byte)*0x100000001b3)&0xffffffffffffffff
    return value

def lookup(data):
    records=defaultdict(list)
    for c,r in enumerate(struct.unpack_from('<65536I',data,0x400)):
        if r!=0xffffffff:records[r].append(c)
    return records

def normal(c):return 32<=c<0xfffe and not(0x7f<=c<=0x9f or 0xd800<=c<=0xf8ff or 0xfdd0<=c<=0xfdef)


def trace_record(data,index,scale,svg):
    at=(index+0x4040)*16;w,h=struct.unpack_from('<HH',data,at+4)
    if not w or not h:
        svg.parent.mkdir(parents=True,exist_ok=True)
        svg.write_text('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1 1"><title>Empty authored font glyph</title></svg>')
        return b'',(w*scale,h*scale),{}
    image,metrics=vectorize(decode_glyph(data,at),svg,scale,font=True)
    return image.tobytes(),image.size,metrics


def build_fonts(maps,vista,output):
    names=sorted(set((maps/'fonts/font_table.txt').read_text().splitlines()));reports=[]
    for name in names:
        source=maps/'fonts'/name;original=source.read_bytes();originals=lookup(original)
        donor_source=vista/'fonts'/name.lower()
        if not donor_source.exists():donor_source=source
        donor=donor_source.read_bytes();donors=lookup(donor)
        density=3 if donor_source!=source else 6
        # Every authored donor record receives its own editable SVG, even if
        # the original font never selects that added Vista codepoint.
        donor_rasters={};donor_vectors=[]
        for record,codes in sorted(donors.items()):
            relative=Path('vectors/fonts-donor')/name/f'{record:04}.svg'
            pixels,size,metrics=trace_record(donor,record,density,output/relative)
            donor_rasters[record]=(pixels,size,relative)
            donor_vectors.append(dict(record=record,codepoints=codes,vector=str(relative),vector_sha256=digest(output/relative),**metrics))
        native=bytearray(original);struct.pack_into('<I',native,0x200,0xf0000003)
        donor_indexes=struct.unpack_from('<65536I',donor,0x400);glyphs=[]
        reviewed=name in {'conduit-12','conduit-13','handel_gothic-11','handel_gothic-13','handel_gothic-24'}
        for record,codes in sorted(originals.items()):
            at=(record+0x4040)*16;w,h=struct.unpack_from('<HH',original,at+4)
            code=codes[0];d=donor_indexes[code];kind='Xbox'
            # Match the existing reviewed Vista glyph selection. Fallback
            # aliases and other private characters are traced from Xbox.
            use_donor=(donor_source!=source and len(codes)==1 and d!=0xffffffff and len(donors[d])==1
                       and (normal(code) or reviewed and 0xe100<=code<=0xe111) and w and h)
            relative=Path('vectors/fonts')/name/f'{record:04}.svg'
            if use_donor:
                pixels,size,previous=donor_rasters[d];(output/relative).parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(output/previous,output/relative)
                kind='Vista';outline_record=d
            else:
                pixels,size,_=trace_record(original,record,6,output/relative);outline_record=record
            if w and h:
                if size[0]<w or size[1]<h:raise ValueError('Vector glyph shrinks logical extent')
                struct.pack_into('<I',native,at+12,len(native));native.extend(pixels)
            struct.pack_into('<HH',native,at+4,*size)
            glyphs.append(dict(record=record,codepoints=codes,original_size=(w,h),reconstructed_size=size,
                               outline_source=kind,outline_record=outline_record,vector=str(relative),vector_sha256=digest(output/relative)))
        native.extend(b'H2VT'+struct.pack('<IQ',3 if donor_source!=source else 6,fnv(original)))
        (output/'fonts').mkdir(exist_ok=True);(output/'fonts-source').mkdir(exist_ok=True)
        (output/'fonts'/name).write_bytes(native);shutil.copyfile(source,output/'fonts-source'/name)
        reports.append(dict(name=name,source=str(source),source_sha256=digest(source),
            outline_donor=str(donor_source),outline_donor_sha256=digest(donor_source),
            output_sha256=digest(output/'fonts'/name),density_multiplier=3 if donor_source!=source else 6,
            authored_glyph_records=len(glyphs),donor_glyph_records=len(donor_vectors),glyphs=glyphs,donor_vectors=donor_vectors))
        print(f'{name}: {len(glyphs)} original records including fallback/private symbols; {len(donor_vectors)} complete donor vectors',flush=True)
    shutil.copyfile(maps/'fonts/font_table.txt',output/'fonts/font_table.txt')
    return reports
