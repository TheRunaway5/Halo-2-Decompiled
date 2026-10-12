#!/usr/bin/env python3
"""Untouched1376C0/137800 with explicit WORLD/region/resident inputs.

No original process is read or changed. Diagnostic WORLD nodes are declared
inputs; literal inverse binds, maps and selected region bytes come from the
supplied caches. This does not restore original pose or registration history.
"""
from pathlib import Path
import argparse, collections, hashlib, json, random, struct, subprocess, sys
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn.x86_const import *
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe
sys.path.insert(0,str(ROOT/'tools'))
from audit_rendering import Cache
PIN='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
def packed(v):return struct.pack('<'+'I'*len(v),*v)
def words(v):return list(struct.unpack('<'+'I'*(len(v)//4),v))
def bits(v):return words(struct.pack('<f',v))[0]
IDENTITY=[1,1,0,0,0,1,0,0,0,1,0,0,0]
def literal_node_map(c,section):
    pointer,size,header=c.u32(section+0x38),c.u32(section+0x3c),c.u32(section+0x44)
    location,offset=pointer>>30,pointer&0x3fffffff
    path=c.path if location==0 else c.path.parent/('', 'mainmenu.map','shared.map','single_player_shared.map')[location]
    if not path.exists():path=ROOT/'assets/disc/maps'/path.name
    with path.open('rb')as f:f.seek(offset);raw=f.read(size)
    assert len(raw)==size and header+4<=size
    count=struct.unpack_from('<H',raw,0x6c)[0]
    resources=[r for r in c.block(section+0x48,16)if c.u16(r+4)==100]
    if count:
        assert len(resources)==1 and c.u32(resources[0]+8)>=count
        start=size-header-4+c.u32(resources[0]+12);nodes=list(raw[start:start+count]);assert len(nodes)==count
    else:nodes=[]
    return nodes,dict(file=str(path.relative_to(ROOT)),offset=offset,size=size,sha256=hashlib.sha256(raw).hexdigest())

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--binary',type=Path,default=ROOT/'build-mp-lightmap/source_model_palette_tests')
    p.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer-shading/model-palette-original.json')
    a=p.parse_args();x=Xbe(ROOT/'assets/disc/default.xbe');assert hashlib.sha256(x.data).hexdigest()==PIN
    u=Uc(UC_ARCH_X86,UC_MODE_32);u.mem_map(0x10000,0x600000)
    for s in x.sections:u.mem_write(s.va,x.section_bytes(s))
    BASE=0x2100000;u.mem_map(BASE,0x400000);STOP=0x3000000;u.mem_map(STOP,4096)
    MODE,TAGS,INVERSE,WORLD,GROUP,SECTIONS,HEADERS,MAPS,ACTIVE,OUTPUT,PART,MATERIAL,GEOMETRY=[BASE+i*0x10000 for i in range(13)]
    STACK=BASE+0x3fff00;counts=collections.Counter();written=[0];base_result=[[]];base_destination=[0];base_nodes=[0]
    def put(at,*v):u.mem_write(at,packed(v))
    def trace(_u,pc,_size,_data):
        if pc in [0x1376c0,0x137800,0x142a60,0xbd610,0x12de70,0x45ce0,0x1d6b0]:counts[f'{pc:X}']+=1
        if pc==0x1376c0:
            base_destination[0]=u.reg_read(UC_X86_REG_EDX)
            base_nodes[0]=struct.unpack('<I',u.mem_read(MODE+0x48,4))[0]
        if pc==0x1377f2:base_result[0]=words(u.mem_read(base_destination[0],base_nodes[0]*48))
    def write(_u,_access,at,size,_value,_data):
        if OUTPUT+0x44<=at<OUTPUT+0x10000:written[0]=max(written[0],at+size-(OUTPUT+0x44))
    u.hook_add(UC_HOOK_CODE,trace);u.hook_add(UC_HOOK_MEM_WRITE,write)
    def original(row):
        flags,lod,compound,n,regions=row[:5];assert compound==0 and 0<=lod<6;at=5
        for dst in [MODE,TAGS,INVERSE,WORLD,GROUP,SECTIONS,HEADERS,MAPS,ACTIVE,OUTPUT]:u.mem_write(dst,bytes(0x10000))
        put(0x4e3b44,TAGS);put(TAGS+8,MODE);u.mem_write(MODE+4,struct.pack('<H',flags))
        put(MODE+0x1c,regions);put(MODE+0x24,256,SECTIONS);put(MODE+0x34,1,GROUP)
        put(MODE+0x48,n,INVERSE);put(GROUP+4,0);put(0x4e6494,0)
        for i in range(n):
            u.mem_write(WORLD+i*52,packed(row[at:at+13]));at+=13
            u.mem_write(INVERSE+i*96+0x28,packed(row[at:at+13]));at+=13
        seen={};selected=[]
        for i in range(regions):
            section,fallback,count=row[at:at+3];at+=3;nodes=row[at:at+count];at+=count
            u.mem_write(ACTIVE+i,bytes([section]));
            selected.append(section)
            if section==255:continue
            bank=(fallback,nodes);assert section not in seen or seen[section]==bank;seen[section]=bank
            u.mem_write(SECTIONS+section*92+0x24,bytes([fallback]));put(SECTIONS+section*92+0x34,HEADERS+section*128)
            put(HEADERS+section*128+0x64,count,MAPS+section*256);u.mem_write(MAPS+section*256,bytes(nodes))
        assert at==len(row);written[0]=0;base_result[0]=[]
        for r in [UC_X86_REG_EAX,UC_X86_REG_EBX,UC_X86_REG_ESI,UC_X86_REG_EDI,UC_X86_REG_EBP]:u.reg_write(r,0)
        u.reg_write(UC_X86_REG_ECX,0xffffffff);u.reg_write(UC_X86_REG_EDX,0)
        put(STACK,STOP,WORLD,ACTIVE,lod,1,OUTPUT);u.reg_write(UC_X86_REG_ESP,STACK)
        u.reg_write(UC_X86_REG_FPCW,0x027f);u.reg_write(UC_X86_REG_FPTAG,0xffff)
        u.reg_write(UC_X86_REG_MXCSR,0x1f80);u.reg_write(UC_X86_REG_EFLAGS,2)
        u.emu_start(0x137800,STOP,count=200000);assert u.reg_read(UC_X86_REG_EIP)==STOP
        base,region_count=struct.unpack('<HH',u.mem_read(OUTPUT,4));assert region_count==regions and written[0]%48==0
        count=written[0]//48
        result=[base,region_count,count,*struct.unpack('<'+'H'*(regions*2),u.mem_read(OUTPUT+4,regions*4)),*words(u.mem_read(OUTPUT+0x44,count*48)),*base_result[0]]
        # Follow the real45CE0 model path using the137800 output context.
        # Part/material identity and rigid node are explicit arithmetic inputs;
        # no source visibility/material/center service is replaced.
        u.mem_write(PART,bytes(72));put(PART+0x2c,bits(1));u.mem_write(MATERIAL,bytes(32));put(MATERIAL+12,0xe0000001)
        put(MODE+0x64,MATERIAL);u.mem_write(0x4ba138,bytes(32));put(0x4ba13c,0);put(0x4ba140,(OUTPUT-0x487b1c)&0xffffffff)
        for region,section in enumerate(selected):
            if section==255:continue
            put(HEADERS+section*128,1,PART);put(0x4ba144,section<<9)
            for rigid in [-1,0,n-1]:
                u.mem_write(SECTIONS+section*92+0x2c,struct.pack('<h',rigid));u.mem_write(GEOMETRY,bytes(44))
                u.reg_write(UC_X86_REG_EAX,GEOMETRY);u.reg_write(UC_X86_REG_ECX,0);put(STACK,STOP,0,region)
                u.reg_write(UC_X86_REG_ESP,STACK);u.emu_start(0x45ce0,STOP,count=10000)
                assert u.reg_read(UC_X86_REG_EIP)==STOP and u.reg_read(UC_X86_REG_EAX)&255==1
                result.extend(words(u.mem_read(GEOMETRY+4,12)))
        return result
    rng=random.Random(0x137800);rows=[];labels=[]
    def add(flags,lod,pairs,regions,label):
        row=[flags,lod,0,len(pairs),len(regions)]
        for world,inverse in pairs:row.extend([*world,*inverse])
        for section,fallback,nodes in regions:row.extend([section,fallback,len(nodes),*nodes])
        rows.append(row);labels.append(label)
    for i in range(700):
        n=1+i%6;pairs=[]
        for j in range(n):
            values=[rng.uniform(-4,4)for _ in range(26)]
            if i%11==0:values=[rng.choice([0.,-0.,1.,-1.,1e-10,1e10])for _ in values]
            pairs.append(([bits(v)for v in values[:13]],[bits(v)for v in values[13:]]))
        regions=[]
        for j in range(1+i%5):
            nodes=[rng.randrange(n)for _ in range(1+(i+j)%6)]if (i+j)%3 else []
            regions.append((j if (i+j)%7 else 255,0 if (i+j)%2 else n,nodes))
        add(4 if i%2 else 0,i%6,pairs,regions,f'explicit-affine{i}')
    for flags in [0,4]:
        add(flags,5,[(list(map(bits,IDENTITY)),list(map(bits,IDENTITY)))],[(255,0,[]),(2,0,[0,0]),(2,0,[0,0]),(3,0,[])],f'NONE-duplicate-map{flags}')
    census=json.loads((ROOT/'analysis/multiplayer-shading/transparent-live-model-seam.json').read_text())['placements']
    selections={(g['map'],g['type'],g['placement']):g for g in json.loads((ROOT/'analysis/multiplayer-shading/object-geometry-lod-original.json').read_text())['placements']}
    maps={};assets=['# Literal D model flags regions inverse-count 6compound-counts inverse-transform-words; S section fallback mapcount nodebytes'];asset_modes=set();asset_sections=set();metadata_cases=0;raw_sections={}
    for owner in census:
        name,datum=owner['map'],owner['render_model']
        if name not in maps:
            path=ROOT/'assets/disc/maps'/(name+'.map')
            if not path.exists():path=ROOT/'assets/map-pack/maps'/(name+'.map')
            maps[name]=Cache(path)
        c=maps[name];_,mode=c.tag(datum,'mode');inverse=[words(c.data[n+0x28:n+0x5c])for n in c.block(mode+0x48,96)]
        regions=c.block(mode+0x1c,16);sections=c.block(mode+0x24,92);groups=c.block(mode+0x34,12)
        compounds=[c.u32(groups[struct.unpack_from('<b',c.data,mode+0x3c+lod)[0]]+4)for lod in range(6)]
        assert not any(compounds)
        if (name,datum)not in asset_modes:
            assets.append('D '+name+' '+' '.join(f'{v:x}'for v in [datum,owner['mode_flags'],len(regions),len(inverse),*compounds,*[v for transform in inverse for v in transform]]));asset_modes.add((name,datum))
        pairs=[]
        for node,inv in enumerate(inverse):
            transform=IDENTITY.copy();transform[0]=1.25;transform[10:13]=[node*.125-3,node*-.2+2,node*.3-1]
            pairs.append((list(map(bits,transform)),inv))
        # Regions not selected by the retained source collector stay NONE.
        # This preserves literal region slots rather than a compact preview list.
        selection=selections[(name,owner['type'],owner['placement'])]
        active=selection['active']
        for lod in range(6):
            bank=[]
            for region_index,region in enumerate(regions):
                permutation=active[region_index]
                selected=struct.unpack_from('<h',c.data,c.block(region+8,16)[permutation]+4+lod*2)[0]if permutation!=255 else -1
                if selected==-1:bank.append((255,0,[]));continue
                section=sections[selected];fallback=c.data[section+0x24]
                key=(name,datum,selected)
                if key not in raw_sections:raw_sections[key]=literal_node_map(c,section)
                literal,_raw_source=raw_sections[key]
                assert literal==next(g for g in owner['lods'][lod]['collectors']if g['section']==selected)['node_map']
                bank.append((selected,fallback,literal))
                if (name,datum,selected)not in asset_sections:
                    assets.append('S '+name+' '+' '.join(f'{v:x}'for v in [datum,selected,fallback,len(literal),*literal]));asset_sections.add((name,datum,selected))
            assert [section for section,_,_ in bank if section!=255]==[g['section']for g in owner['lods'][lod]['collectors']]
            # Repeat identical model/LOD/region byte inputs only once. The
            # asset census still retains every unique supplied section.
            label=f'{name}:{datum:08X}:LOD{lod}:'+','.join(str(s)for s,_,_ in bank)
            if label in labels:continue
            add(owner['mode_flags'],lod,pairs,bank,label);metadata_cases+=1
    retained=ROOT/'analysis/multiplayer-shading/original-xemu/zanzibar-player20-unit.bin'
    unit=retained.read_bytes();unit_hash=hashlib.sha256(unit).hexdigest()
    assert unit_hash=='5a15494237fb28b8ec48c7638a27df3d84ca1f7f85871a8e248249d2a10f202b'
    node_bytes,node_offset=struct.unpack_from('<hh',unit,0x114)
    c=maps['zanzibar'];_,definition=c.tag(struct.unpack_from('<I',unit)[0],'bipd')
    _,hlmt=c.tag(c.u32(definition+0x38),'hlmt');datum=c.u32(hlmt+4);_,mode=c.tag(datum,'mode')
    inverse=[words(c.data[n+0x28:n+0x5c])for n in c.block(mode+0x48,96)]
    assert node_bytes==len(inverse)*52 and node_offset+node_bytes<=len(unit)
    pairs=[(words(unit[node_offset+i*52:node_offset+(i+1)*52]),inv)for i,inv in enumerate(inverse)]
    # Full actual paused WORLD bank. Region NONE is an explicit arithmetic
    #control, not a claim that those regions were absent in the original view.
    add(c.u16(mode+4),5,pairs,[(255,0,[])]*c.u32(mode+0x1c),'retained-Zanzibar-WORLD33-explicit-NONE-regions')
    retained_bank=dict(file=str(retained.relative_to(ROOT)),sha256=unit_hash,render_model=f'{datum:08X}',node_offset=node_offset,node_count=len(inverse),node_sha256=hashlib.sha256(unit[node_offset:node_offset+node_bytes]).hexdigest(),regions='explicit all-NONE arithmetic control')
    expected=[original(row)for row in rows]
    text=''.join(f'{len(row):x} '+' '.join(f'{v:x}'for v in row)+'\n'for row in rows)
    actual=[list(map(lambda x:int(x,16),line.split()))for line in subprocess.check_output([str(a.binary.resolve()),'--probe'],input=text,text=True).splitlines()]
    assert len(actual)==len(expected)
    failures=[dict(index=i,label=labels[i],expected=e,actual=n)for i,(e,n)in enumerate(zip(expected,actual))if e!=n]
    scope='Untouched137800 ->1376C0 ->142A60 with explicit current WORLD nodes, post-BD610 caller composition admitted, geometry residency explicitly synchronous, and actual zero-compound groups. Synthetic affine and literal741-owner/21-map inverse-bind/node-map/selected-region banks are inputs; diagnostic WORLD nodes do not assert original controller phase or native pose equivalence. NONE regions/duplicates/no-map/mask4 are preserved. Original source async residency, compounds, live collector/registration order remain separate boundaries.'
    sources=[dict(map=name,mode=f'{mode:08X}',section=section,**record)for (name,mode,section),(_nodes,record)in raw_sections.items()]
    report=dict(executable_sha256=PIN,native_sha256=hashlib.sha256(a.binary.read_bytes()).hexdigest(),cases=len(rows),exact_cases=len(rows)-len(failures),compared_words=sum(map(len,expected)),metadata_cases=metadata_cases,maps=len(maps),models=len(asset_modes),sections=len(asset_sections),raw_section_sources=sources,retained_WORLD_bank=retained_bank,source_calls=dict(counts),replaced_source_services=[],explicit_services=['caller WORLD bank after BD610: parameter6=1','resident source bank:4E6494=0'],failures=failures[:8],scope=scope)
    a.output.write_text(json.dumps(report,indent=2)+'\n');assert not failures,failures[:3]
    fixture=ROOT/'tests/fixtures/retail_source_model_palette.tsv'
    subset=list(range(40))+list(range(40,len(rows),7))
    if len(rows)-1 not in subset:subset.append(len(rows)-1)
    fixture.write_text(''.join(f'{len(rows[i]):x} '+' '.join(f'{v:x}'for v in rows[i])+f' {len(expected[i]):x} '+' '.join(f'{v:x}'for v in expected[i])+'\n'for i in subset))
    asset_file=ROOT/'tests/fixtures/retail_source_model_palette_assets.tsv';asset_file.write_text('\n'.join(assets)+'\n')
    for f in [fixture,asset_file]:f.with_suffix('.provenance.json').write_text(json.dumps(dict(executable_sha256=PIN,oracle=str(Path(__file__).relative_to(ROOT)),oracle_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),fixture_sha256=hashlib.sha256(f.read_bytes()).hexdigest(),rows=len(f.read_text().splitlines()),scope=scope),indent=2)+'\n')
    for c in maps.values():c.close()
    print(json.dumps({k:v for k,v in report.items()if k not in ['failures','raw_section_sources']},indent=2))

if __name__=='__main__':main()
