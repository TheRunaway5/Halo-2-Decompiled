#!/usr/bin/env python3
"""Read-only inventory of the supplied version-6 resident tag tables.

The table layout is checked structurally on every file: its stored address,
count, 32-byte entries, salted datum ordinal, bounded name/payload pointers and
ASCII group/name fields must agree. This inventory is not a render trace or a
decoder for object/shader fields. Original 00037CA0 proves the 32-byte stride,
full salted datum at +0C and count at cache-header +0C; 00038120 checks group
at +0/+4/+8 and returns the payload pointer at +14. The presentation reference
links the unchanged original instructions; unresolved headers are reported, never normalized.
"""
import argparse
import hashlib
import json
import mmap
from pathlib import Path
import struct


def inventory(path):
    with path.open('rb') as f, mmap.mmap(f.fileno(), 0, access=mmap.ACCESS_READ) as data:
        u32=lambda offset: struct.unpack_from('<I',data,offset)[0]
        assert data[:4]==b'daeh' and u32(4)==6, 'not a version-6 header'
        assert u32(8)==len(data), 'declared file length differs'
        start,size=u32(16),u32(20)
        assert 0x800<=start<len(data) and 28<=size<=len(data)-start, 'resident extent'
        assert data[start+24:start+28]==b'sgat', 'tag table signature'
        count=u32(start+12);assert 0<count<=65535, 'tag count'
        table=start+28;assert count*32<=size-28, 'tag table extent'
        magic=u32(start)-table
        def pointer(address,extent=1):
            offset=address-magic
            assert start<=offset<=start+size and extent<=start+size-offset, 'resident pointer extent'
            return offset
        def name(address):
            offset=pointer(address);end=data.find(b'\0',offset,min(start+size,offset+1024))
            assert end>=offset, 'unterminated tag name'
            value=data[offset:end].decode('ascii')
            assert value and all(32<=ord(c)<127 for c in value), 'tag name text'
            return value,offset
        tags=[]
        for ordinal in range(count):
            at=table+ordinal*32;words=struct.unpack_from('<8I',data,at)
            group=struct.pack('<I',words[0])[::-1].decode('ascii')
            assert all(32<=ord(c)<127 for c in group), 'tag group text'
            assert words[3]&65535==ordinal, 'salted datum ordinal'
            text,name_offset=name(words[4])
            payload=None
            if words[5]:
                try:payload=pointer(words[5])
                except AssertionError:pass
            tags.append(dict(ordinal=ordinal,group=group,datum=f'{words[3]:08x}',name=text,
                entry_file_offset=f'0x{at:x}',entry_words=[f'{v:08x}'for v in words],
                name_file_offset=f'0x{name_offset:x}',
                payload_file_offset=f'0x{payload:x}' if payload is not None else None,
                payload_status='resident pointer' if payload is not None else 'external or absent; not decoded'))
        return dict(path=str(path),sha256=hashlib.sha256(data).hexdigest(),version=6,
            tag_data_offset=f'0x{start:x}',tag_data_bytes=size,stored_table_address=f'{u32(start):08x}',
            table_file_offset=f'0x{table:x}',pointer_bias=magic,tag_count=count,tags=tags)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--maps',type=Path,default=Path('/mnt/SSD4TB/Leaks/Halo 2/2003/game/maps'))
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    result=dict(scope=__doc__,maps=[],unresolved=[])
    for path in sorted(args.maps.glob('*.map')):
        try:
            report=inventory(path);name=path.stem+'.json'
            (args.output/name).write_text(json.dumps(report,indent=2)+'\n')
            result['maps'].append(dict(name=path.name,sha256=report['sha256'],tag_count=report['tag_count'],report=name))
        except (AssertionError,UnicodeError,struct.error) as error:
            result['unresolved'].append(dict(path=str(path),error=str(error)))
    (args.output/'summary.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'maps':len(result['maps']),'unresolved':result['unresolved']}))


if __name__=='__main__':main()
