#!/usr/bin/env python3
"""Read prepared MCC lightmap reference bounds before native2CFE0 integration.

Use run_test.py. Metadata only: no decoded frames, resource copies or assets
are written. Stored counts do not replace the original's unchecked reads.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


def inspect(path):
    raw=path.read_bytes()
    if len(raw)<896 or raw[:4]!=b'daeh' or struct.unpack_from('<I',raw,4)[0]!=13:
        raise ValueError('Lightmap census requires MCC v13 maps')
    u_raw=lambda at:struct.unpack_from('<I',raw,at)[0]
    extent=u_raw(8)
    if not 896<extent<=1024**3:raise ValueError('Invalid decoded MCC extent')
    if struct.unpack_from('<H',raw,28)[0]&1:
        chunk,table,count=u_raw(776),u_raw(784),u_raw(788)
        if not chunk or not 0<count<=65536 or table+8*count>len(raw):raise ValueError('Invalid MCC chunk table')
        data=bytearray(raw[:896])
        for k in range(count):
            stored,address=struct.unpack_from('<iI',raw,table+8*k)
            if not stored or address<896 or address+abs(stored)>len(raw):raise ValueError('Invalid MCC chunk extent')
            part=raw[address:address+abs(stored)]
            if stored>0:part=zlib.decompress(part)
            if not part or len(part)>min(chunk,extent-len(data)):raise ValueError('Invalid MCC decoded chunk')
            data.extend(part)
        if len(data)!=extent:raise ValueError('Decoded MCC extent differs')
    else:
        if extent!=len(raw):raise ValueError('Truncated prepared MCC map')
        data=raw
    u=lambda at:struct.unpack_from('<I',data,at)[0]
    index=u(16);header_magic=u(index)-index-32;table=u(index+8)-header_magic
    magic=u(720)-index;count=u(index+24)
    if not 0<count<=65536 or count!=u(32) or table<0 or table+16*count>len(data):raise ValueError('Invalid MCC directory')
    tags={}
    for k in range(count):
        at=table+16*k;datum,address,size=u(at+4),u(at+8),u(at+12)
        if datum==0xffffffff:continue
        if datum&65535!=k:raise ValueError('MCC tag datum/index mismatch')
        tags[datum]=dict(group=data[at:at+4][::-1].decode('ascii'),address=address,size=size)
    def tag(datum,group):
        t=tags[datum]
        if t['group']!=group or not t['address']:raise ValueError('Missing local '+group+' metadata')
        at=t['address']-magic
        if at<0 or at+t['size']>len(data):raise ValueError('MCC tag extent invalid')
        return at
    scenario=tag(u(index+12),'scnr');count=u(scenario+0x210);at=u(scenario+0x214)-magic
    if count>256 or at<0 or at+count*0x44>len(data):raise ValueError('Invalid scenario BSP block')
    rows=[]
    for index in range(count):
        record=at+0x44*index;datum=u(record+0x14);bsp=tag(datum,'sbsp');lightmap_datum=u(record+0x1c)
        def read(address):
            offset=address-magic
            if offset<0 or offset+4>len(data):raise ValueError('BSP census read outside map metadata')
            return u(offset)
        def block(address,stride):
            n=read(address);base=read(address+4)
            if n and (n>1000000 or base-magic<0 or base-magic+n*stride>len(data)):raise ValueError('BSP census block outside map metadata')
            return n,base
        bsp_address=tags[datum]['address']
        instances=u(bsp+0x140);lm=0xffffffff if lightmap_datum==0xffffffff else tags[lightmap_datum]['address']
        row=dict(bsp=index,datum=hex(datum),instances=instances,metadata_bytes=tags[datum]['size'],groups=0,
            gates_pass=False,instance_references=0,scenery_references=0,buckets=0,
            instance_missing_refs=0,instance_unbound=0,scenery_unbound=0,instance_out_of_range=0,scenery_out_of_range=0)
        if lm not in (0,0xffffffff):
            tag(lightmap_datum,'ltmp');groups,group=block(lm+0x80,104);row['groups']=groups
            if groups:
                row['gates_pass']=read(group+0x1c)!=0xffffffff and read(group+4)==u(bsp+8)
                buckets,_=block(group+0x40,56);nr,refs=block(group+0x50,12);ns,scenery=block(group+0x60,12)
                ni,_=block(group+0x58,12)
                row.update(buckets=buckets,instance_references=nr,scenery_references=ns,scenery_identities=ni,
                    instance_missing_refs=max(0,instances-nr),scenery_identity_count_mismatch=ni!=ns)
                for kind,n,base in [('instance',nr,refs),('scenery',ns,scenery)]:
                    bad=[]
                    for k in range(n):
                        bucket=struct.unpack_from('<H',data,base-magic+12*k+2)[0]
                        if bucket==65535:row[kind+'_unbound']+=1
                        elif bucket>=buckets:row[kind+'_out_of_range']+=1
                        else:continue
                        if len(bad)<32:bad.append(dict(index=k,bucket=bucket,source_address=hex(base+12*k+2)))
                    row[kind+'_bad_samples']=bad
        rows.append(row)
    return dict(path=str(path.resolve()),sha256=hashlib.sha256(raw).hexdigest(),decoded_sha256=hashlib.sha256(data).hexdigest(),bsps=rows)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--maps',type=Path,required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
    paths=[p for p in sorted(a.maps.glob('*.map'))if p.name not in ('shared.map','single_player_shared.map','mainmenu.map')]
    if not paths:raise ValueError('No prepared MCC maps')
    rows=[]
    for path in paths:
        row=inspect(path);rows.append(row);print(path.name,len(row['bsps']),'BSPs',flush=True)
    eligible=[b for m in rows for b in m['bsps']if b['gates_pass']]
    keys=['instance_missing_refs','instance_unbound','scenery_unbound','instance_out_of_range','scenery_out_of_range']
    totals={key:sum(b[key]for b in eligible)for key in keys}
    report=dict(passed=True,maps=rows,map_count=len(rows),bsp_count=sum(len(m['bsps'])for m in rows),eligible_bsps=len(eligible),
        eligible_reference_risks=totals,unchecked_reads_need_layout_qualification=any(totals.values()),
        helper_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        decoder_reference_sha256=hashlib.sha256(Path(__file__).resolve().parents[1].joinpath('src/assets/mcc_cache_decode.cpp').read_bytes()).hexdigest(),
        scope='Stored prepared MCC BSP and default lightmap-group metadata only. Original2CFE0 ignores reference/bucket bounds; no result is supplied for missing or unbound rows. Live bank layout and actually selected scenery remain separate qualification.',retained_large_artifacts=[])
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print('Reference risks in eligible groups:',totals)
    return 0


if __name__=='__main__':raise SystemExit(main())
