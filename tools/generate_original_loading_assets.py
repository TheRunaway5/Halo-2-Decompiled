#!/usr/bin/env python3
"""Retain the exact five original Xbox loading banks/all11pages and mips.

Run under tools/run_test.py. Generate only into its owned scratch; publish
the authenticated bounded source include explicitly. No original cache or
prepared MCC input is modified. No image is generated/resampled/decoded here.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import zlib
from audit_rendering import Cache

ROOT=Path(__file__).resolve().parents[1]
PIN='1b5de78212ee7439b086ef3554885a4188884864ce1040b24ec6604af4a0143b'
SOURCES=(('loading_screen',1,0x1c),('loading_screen_sweep',1,0x24),
    ('loading_screen_spinners',2,0x2c),('loading_screen_logos',2,0x3c),
    ('loading_screen_tickers',5,0x44))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    owned=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert args.output.resolve().is_relative_to(owned)
    cache=Cache(ROOT/'assets/disc/maps/mainmenu.map');rows=[];raw=bytearray();owners=[]
    try:
        assert hashlib.sha256(cache.data).hexdigest()==PIN
        globals=next(t for t in cache.tags.values()if t['group']=='matg')
        _,g=cache.tag(globals['datum'],'matg');raster=cache.block(g+0x108,0x1c8);assert len(raster)==1
        for bank,(suffix,count,reference)in enumerate(SOURCES):
            datum=cache.u32(raster[0]+reference);tag,at=cache.tag(datum,'bitm')
            name='globals\\loading_screen\\'+suffix;assert tag['name']==name
            pages=cache.block(at+68,116);assert len(pages)==count
            owners.append(dict(name=name,datum=datum,reference=reference,pages=count))
            for index,page in enumerate(pages):
                pointer,size=cache.u32(page+28),cache.u32(page+52)
                assert pointer>>30 in(0,1) and 0<size<=1024*1024
                offset=pointer&0x3fffffff;cache.require(offset,size)
                pixels=bytes(cache.data[offset:offset+size])
                rows.append(dict(bank=bank,index=index,width=cache.u16(page+4),height=cache.u16(page+6),
                    format=cache.u16(page+12),flags=cache.u16(page+14),additional_mips=cache.u16(page+20),
                    offset=len(raw),bytes=size,sha256=hashlib.sha256(pixels).hexdigest()))
                raw.extend(pixels)
    finally:cache.close()
    assert len(rows)==11 and 0<len(raw)<=2*1024*1024
    compressed=zlib.compress(raw,9);assert zlib.decompress(compressed)==raw and len(compressed)<=2*1024*1024
    encoded=compressed.hex();chunks=[encoded[i:i+16384]for i in range(0,len(encoded),16384)]
    lines=['// Exact pinned DVDloading bitmaps; all11pages, no reconstruction.',
        f'constexpr std::size_t original_loading_bytes={len(raw)};',
        f'constexpr std::size_t original_loading_compressed_bytes={len(compressed)};',
        'constexpr std::string_view original_loading_sha256="'+hashlib.sha256(raw).hexdigest()+'";',
        f'constexpr std::array<std::string_view,{len(chunks)}> original_loading_chunks{{{{']
    lines.extend('    "'+chunk+'",'for chunk in chunks);lines.append('}};')
    lines.append('constexpr std::array<OriginalLoadingOwner,5> original_loading_owners{{')
    for owner in owners:lines.append('    OriginalLoadingOwner{"'+owner['name'].replace('\\','\\\\')+'",'+str(owner['datum'])+'u,'+str(owner['pages'])+'},')
    lines.append('}};');lines.append('constexpr std::array<OriginalLoadingPage,11> original_loading_pages{{')
    for row in rows:lines.append('    OriginalLoadingPage{'+','.join(str(row[key])for key in('bank','index','width','height','format','flags','additional_mips','offset','bytes'))+'},')
    lines.append('}};');result=('\n'.join(lines)+'\n').encode();assert len(result)<=4*1024*1024
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result)
    report=dict(passed=True,schema='H2_ORIGINAL_LOADING_BITMAPS_V1',source_mainmenu_sha256=PIN,
        owners=owners,pages=rows,raw_bytes=len(raw),raw_sha256=hashlib.sha256(raw).hexdigest(),
        compressed_bytes=len(compressed),output_bytes=len(result),output_sha256=hashlib.sha256(result).hexdigest(),
        scratch=str(owned),original_assets_modified=False,raw_test_artifacts_retained=False)
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,pages=len(rows),raw_bytes=len(raw),compressed_bytes=len(compressed),source_include_bytes=len(result))))


if __name__=='__main__':main()
