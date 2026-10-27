#!/usr/bin/env python3
"""Export two exact DVD UI sound payloads; no decoding or resynthesis.

The generated bounded include stays in the run_test owned scratch directory.
Only original UI sound10/12 are retained; other menu/global audio is untouched.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import zlib
from audit_rendering import Cache

PIN='1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
EXPECTED=((10,0xE4D502B5,'sound\\game_sfx\\multiplayer\\countdown_for_respawn'),
          (12,0xE4D602B6,'sound\\game_sfx\\multiplayer\\pickup_health'))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mainmenu',type=Path,default=Path('assets/disc/maps/mainmenu.map'))
    p.add_argument('--output',type=Path,required=True);p.add_argument('--report',type=Path,required=True)
    a=p.parse_args();owned=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert a.output.resolve().is_relative_to(owned)
    cache=Cache(a.mainmenu);raw=bytearray();rows=[]
    try:
        assert hashlib.sha256(cache.data).hexdigest()==PIN
        tag=next(t for t in cache.tags.values()if t['group']=='wigl'and t['name']=='ui\\ui_shared_globals')
        _,ui=cache.tag(tag['datum'],'wigl')
        gestalt=next(t for t in cache.tags.values()if t['group']=='ugh!')
        _,g=cache.tag(gestalt['datum'],'ugh!')
        ranges=cache.block(g+32,12);permutations=cache.block(g+40,16);chunks=cache.block(g+64,12)
        for slot,datum,name in EXPECTED:
            assert cache.u32(ui+0x94+slot*8)==datum
            tag,at=cache.tag(datum,'snd!');assert tag['name']==name
            rate,channels,codec=cache.data[at+3],cache.data[at+4]+1,cache.data[at+5]
            assert rate<=2 and channels==1 and codec==1 and cache.data[at+10]==1
            r=ranges[cache.u16(at+8)];assert cache.u16(r+10)==1
            permutation=permutations[cache.u16(r+8)]
            first,count=cache.u16(permutation+12),cache.u16(permutation+14)
            assert count>0 and first+count<=len(chunks)
            payload=bytearray()
            for chunk in chunks[first:first+count]:
                pointer,size=cache.u32(chunk),cache.u32(chunk+4)&0x3fffffff
                assert pointer>>30==0
                cache.require(pointer,size);payload.extend(cache.data[pointer:pointer+size])
            rows.append(dict(slot=slot,datum=datum,name=name,rate=(22050,44100,32000)[rate],
                channels=channels,codec=codec,samples=cache.u32(permutation+8),sound_class=cache.data[at+2],
                offset=len(raw),bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest()))
            raw.extend(payload)
    finally:cache.close()
    assert 0<len(raw)<=65536
    compressed=zlib.compress(raw,9);assert zlib.decompress(compressed)==raw
    encoded=compressed.hex();strings=[encoded[i:i+4096]for i in range(0,len(encoded),4096)]
    lines=['// Generated from the pinned original DVD mainmenu.map; immutable ADPCM only.',
        'constexpr std::array<std::string_view,'+str(len(strings))+'> original_countdown_audio_chunks{{',*['"'+v+'",'for v in strings],'}};',
        'constexpr std::size_t original_countdown_audio_compressed_bytes='+str(len(compressed))+';',
        'constexpr std::size_t original_countdown_audio_bytes='+str(len(raw))+';',
        'constexpr std::string_view original_countdown_audio_sha256="'+hashlib.sha256(raw).hexdigest()+'";',
        'constexpr std::array<OriginalCountdownAudioRecord,2> original_countdown_audio_records{{']
    for row in rows:
        lines.append('{'+','.join(str(row[v])for v in ('slot','rate','channels','codec','samples','sound_class','offset','bytes'))+',"'+row['name'].replace('\\','\\\\')+'"},')
    lines+=['}};','']
    a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text('\n'.join(lines))
    report=dict(source_sha256=PIN,rows=rows,raw_bytes=len(raw),raw_sha256=hashlib.sha256(raw).hexdigest(),
        compressed_bytes=len(compressed),compressed_sha256=hashlib.sha256(compressed).hexdigest(),
        include_bytes=a.output.stat().st_size,include_sha256=hashlib.sha256(a.output.read_bytes()).hexdigest(),
        no_decoding=True,no_resynthesis=True,no_resampling=True)
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
