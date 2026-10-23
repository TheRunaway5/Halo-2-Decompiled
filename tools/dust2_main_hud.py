"""Author Dust II with the ordinary game's prepared HUD definitions/artwork.

NHDT exports come from export_dust2_hud after the existing Cache preparation.
References are resolved by exact group/name; salted handles and StringIDs are
rebound to the destination. Bitmap sequences, sprites, all mips and encoded
texture partitions are copied without resampling.
"""
import hashlib
import json
import struct
from pathlib import Path

from mcc_content import MccCache


def install_main_hud(cache, exported, imported):
    export_bytes = exported.read_bytes()
    hud = json.loads(export_bytes)
    assert hud['schema'] == 'H2_DUST2_MAIN_GAME_HUD_V1'
    donor_path = imported/'halo2/h2_maps_win64_dx11/lockout.map'
    assert donor_path.resolve() == Path(hud['source_map']).resolve()
    donor = MccCache(donor_path)
    shared_path = imported/'halo2/h2_maps_win64_dx11/shared.map'
    shared_donor = MccCache(shared_path)
    target = {(t['group'],t['name']):t for t in cache.tags.values()}
    source = {(t['group'],t['name']):t for t in donor.tags.values()}
    shared_source = {(t['group'],t['name']):t for t in shared_donor.tags.values()}
    data = cache.data
    reused_metadata = 0
    reused_partitions = 0
    table_magic = cache.u32(cache.index)-cache.index-32
    table = cache.pointer(cache.u32(cache.index+8),table_magic,len(cache.tags)*16)
    strings_at,strings_size,indices = (cache.u32(n) for n in (52,56,60))
    strings = bytearray(data[strings_at:strings_at+strings_size])
    offsets = [cache.u32(indices+i*4) for i in range(cache.u32(48))]
    string_ids = {cache.string(strings_at+at,strings_size-at):i for i,at in enumerate(offsets)}

    def sid(name):
        raw = name.encode('utf-8')
        assert len(raw)<256
        if name not in string_ids:
            string_ids[name]=len(offsets)
            offsets.append(len(strings));strings.extend(raw+b'\0')
        return len(raw)<<24 | string_ids[name]

    def align():
        data.extend(bytes((-len(data))%16))

    def publish(tag, payload, pointers):
        nonlocal reused_metadata
        reuse=tag['address'] and tag['size']==len(payload)
        if reuse:
            at=tag['offset'];address=tag['address'];reused_metadata+=1
        else:
            align();at=len(data);address=at+cache.magic
        for field,relative in pointers:
            assert 0<=field<=len(payload)-4 and 0<=relative<len(payload)
            struct.pack_into('<I',payload,field,address+relative)
        if reuse:data[at:at+len(payload)]=payload
        else:data.extend(payload)
        struct.pack_into('<II',data,table+(tag['datum']&65535)*16+8,address,len(payload))
        tag.update(offset=at,address=address,size=len(payload))

    definitions=[]
    for row in hud['definitions']:
        key=('nhdt',row['name'])
        if key not in target:continue # Main-game assets unused by this map.
        payload=bytearray.fromhex(row['hex'])
        for field,group,name in row['references']:
            binding=target[(group,name)]
            assert binding['address'],f'Nonresident HUD reference: {group} {name}'
            struct.pack_into('<I',payload,field,binding['datum'])
        for field,name in row['strings']:
            struct.pack_into('<I',payload,field,sid(name))
        publish(target[key],payload,row['pointers'])
        definitions.append(row['name'])
    assert set(definitions)=={t['name'] for t in cache.tags.values() if t['group']=='nhdt'}

    bitmap_rows=[]
    with (imported/'halo2/h2_maps_win64_dx11/textures.dat').open('rb') as textures:
        for key,tag in target.items():
            if key[0]!='bitm' or not key[1].startswith('ui\\hud\\') or key not in source:continue
            owner=source[key]
            bitmap_donor=donor
            if not owner['address']:
                owner=shared_source[key]
                bitmap_donor=shared_donor
            assert owner['address'],f'Missing main-game HUD bitmap metadata: {key}'
            root=owner['offset'];payload=bytearray(bitmap_donor.data[root:root+76]);pointers=[]
            current_images=cache.block(tag['offset']+68,168) if tag['address'] else []

            def block(descriptor,stride,destination):
                rows=bitmap_donor.block(descriptor,stride)
                if not rows:
                    struct.pack_into('<II',payload,destination,0,0)
                    return []
                payload.extend(bytes((-len(payload))%4));at=len(payload)
                payload.extend(bitmap_donor.data[rows[0]:rows[0]+len(rows)*stride])
                pointers.append((destination+4,at))
                return list(zip(rows,(at+i*stride for i in range(len(rows)))))

            sequences=block(root+60,60,60)
            for old,new in sequences:block(old+52,32,new+52)
            images=block(root+68,168,68)
            partitions=[]
            for image_index,(old,new) in enumerate(images):
                block(old+32,12,new+32) # Native mip information, if authored.
                for channel in range(3):
                    pointer,size=bitmap_donor.u32(old+56+channel*4),bitmap_donor.u32(old+80+channel*4)
                    if not size:
                        assert not pointer
                        continue
                    assert 0<size<128*1024**2
                    # The original bank pointer is a location token, not a
                    # metadata address. Keep the exact encoded partition.
                    textures.seek(pointer&0x3fffffff);raw=textures.read(size)
                    assert len(raw)==size
                    current=current_images[image_index] if len(current_images)==len(images) else None
                    previous=cache.u32(current+56+channel*4) if current is not None else 0
                    previous_size=cache.u32(current+80+channel*4) if current is not None else 0
                    if previous and previous>>30==0 and previous_size==size and data[previous:previous+size]==raw:
                        at=previous;reused_partitions+=1
                    else:
                        align();at=len(data);assert at+size<0x40000000
                        data.extend(raw)
                    struct.pack_into('<I',payload,new+56+channel*4,at)
                    partitions.append(dict(channel=channel,bytes=size,sha256=hashlib.sha256(raw).hexdigest()))
            publish(tag,payload,pointers)
            bitmap_rows.append(dict(name=key[1],images=len(images),partitions=partitions))
    assert bitmap_rows,'Main-game HUD bitmap transfer produced no artwork'
    align();strings_at=len(data);data.extend(strings)
    align();indices=len(data);data.extend(struct.pack('<'+'I'*len(offsets),*offsets))
    for at,value in ((48,len(offsets)),(52,strings_at),(56,len(strings)),(60,indices),(8,len(data))):
        struct.pack_into('<I',data,at,value)
    return dict(source='ordinary prepared Lockout HUD through Cache',source_map_sha256=hashlib.sha256(donor_path.read_bytes()).hexdigest(),
                shared_bitmap_metadata_sha256=hashlib.sha256(shared_path.read_bytes()).hexdigest(),
                export_sha256=hashlib.sha256(export_bytes).hexdigest(),definitions=definitions,bitmaps=bitmap_rows,
                metadata_allocations_reused=reused_metadata,encoded_partitions_reused=reused_partitions,
                renderer='existing MultiplayerHud',new_hud_renderer=False)
