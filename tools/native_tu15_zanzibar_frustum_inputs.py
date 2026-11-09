"""Extract literal Zanzibar cull inputs from existing genuine retail RAM.

Read the required reference in place. Numeric geometry, poses, ranges and
captured masks are unchanged; only addresses/handles are relocated by the
physical corpus builder. Declared pre-state clears each chosen unit's flag1
and its own mask, matching the replay that reproduced the captured output.
"""
import copy
import hashlib
from pathlib import Path
import struct
from oracle_effect_light_snapshot import Snapshot,u

ROOT=Path(__file__).resolve().parents[1]
PIN='ab7e518eda315ecd55e681d440c988d2814260c292bbbd69972754ac7d32bf55'


def inputs():
    path=ROOT/'analysis/multiplayer-shading/registration-snapshot-restore-20261008/exports'
    ram=Snapshot(path)
    if hashlib.sha256(ram.ram).hexdigest()!=PIN:raise ValueError('Wrong genuine Zanzibar cull reference')
    def words(at,n):return list(struct.unpack('<'+'I'*n,ram.read(at,n*4)))
    def half(at):return struct.unpack('<h',ram.read(at,2))[0]
    ctx=0x547f88;bank=u(ram.read(ctx,4));count=half(bank+0xa6c);nf=half(bank+0x1970)
    groups=half(bank);arena=words(ctx+0x25c,512);ranges=[];frustums=[]
    for i in range(count):
        r=struct.unpack('<13h',ram.read(bank+0xa6e+i*26,26));ranges.append(dict(cluster=r[0],counts=list(r[1:7]),first=list(r[7:13])))
    for i in range(nf):frustums.append(dict(bounds=words(bank+0x1974+i*0x108+0x60,6),
        planes=[words(bank+0x1974+i*0x108+0x78+j*16,4)for j in range(6)]))
    associations=[words(ctx+0xa60+i*16,4)for i in range(half(ctx+0xa5e))]
    pool=u(ram.read(0x4e0300,4));headers=u(ram.read(pool+0x44,4));tags=u(ram.read(0x4e3b44,4))
    items=[];original=[];captured_arena=arena[:]
    for index in (18,19,20,21,22):
        w=words(0x4ba138+index*32,8);owner=u(ram.read(w[4],4));body=u(ram.read(headers+12*(owner&65535)+8,4))
        node_bytes,node_offset=struct.unpack('<hh',ram.read(body+0x114,4));assert node_bytes>=0 and node_bytes%52==0
        mode=u(ram.read(tags+16*(w[1]&65535)+8,4));section=u(ram.read(mode+0x28,4))+92*((w[3]>>9)&511)
        geometry=u(ram.read(section+0x34,4));subparts,part_at,bounds,bound_at=struct.unpack('<iIiI',ram.read(geometry+8,16))
        assert half(mode+0x48)>=node_bytes//52 and struct.unpack('<i',ram.read(geometry+0x30,4))[0]==0
        parts=[struct.unpack('<H',ram.read(part_at+8*j+4,2))[0]for j in range(subparts)]
        bound_rows=[[*words(bound_at+20*j,4),ram.read(bound_at+20*j+16,1)[0]]for j in range(bounds)]
        assert all(p<len(bound_rows)for p in parts)and all(b[4]<node_bytes//52 for b in bound_rows)
        offset=(w[5]-(ctx+0x25c))//4;mask_count=(subparts+31)//32
        assert 0<=offset and offset+mask_count<=512 and w[0]&0x40 and w[0]&8
        arena[offset:offset+mask_count]=[0]*mask_count
        inverse=u(ram.read(mode+0x4c,4))
        items.append(dict(parts=parts,bounds=bound_rows,count=subparts,mask_offset=offset,
            nodes=[words(body+node_offset+52*j,13)for j in range(node_bytes//52)],
            inverse=[words(inverse+96*j+0x28,13)for j in range(node_bytes//52)],
            flags=w[0]&~1,association=w[6]&65535))
        original.append(dict(unit=index,words=w,owner=owner,subparts=subparts,bounds=bounds,
            captured_mask=captured_arena[offset:offset+mask_count]))
    assert groups==1 and count==15 and nf==15
    expected=dict(flags=[r['words'][0]for r in original],arena=captured_arena,original_units=original,
        source_snapshot_sha256=PIN,source_path=str(path/'physical-ram.bin'),
        declaration='Clear chosen flag1 and own mask words; relocate addresses/handles only. Literal finite banks and captured expected flags/masks unchanged.')
    rows=[]
    for mode in (0,1):
        for control,mxcsr in ((0x27f,0x1f80),(0x23f,0x1fa0)):
            rows.append(dict(name=f'genuine Zanzibar PRT main bank; pass{mode} CW{control:x}',units=copy.deepcopy(items),
                groups=groups,ranges=ranges,frustums=frustums,arena=arena,associations=associations,
                mode=mode,fp=[control,0,mxcsr],expected=expected))
    return rows
