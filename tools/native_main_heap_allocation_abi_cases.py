"""Declared allocator inputs for the unchanged physical TU1.5 ABI proof."""
import hashlib
import json
import lzma
import subprocess


def extend(reference,image,banks,rows):
    manifest=json.loads(lzma.decompress(reference.ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
    name='before-268443648.bin';member=reference.MEMBER+name
    backing=subprocess.check_output(['tar','--zstd','-xOf',str(reference.ARCHIVE),member])
    proof=manifest[member]
    assert len(backing)==proof['bytes']==0x46000 and hashlib.sha256(backing).hexdigest()==proof['sha256']
    banks[0x10002000]=backing
    for address in (0x232c5c,0x232d70):banks[address]=image.read(address,29)
    for allocator in (0x10061000,0x10166410):
        for size in (0,1,15,16,32,128,512,8192,65536):
            rows.append(dict(name=f'actual_main_heap_sized_allocate_{allocator:x}_{size}',entry=0x232d00,
                arguments=[size,0x12],receiver=allocator,header=reference.HEADER,full_abi=True))
        for alignment in (0,1,2,4,8,16):
            rows.append(dict(name=f'actual_main_heap_allocate_class_{allocator:x}_{alignment}',entry=0x232ad0,
                arguments=[alignment,17,0x12],receiver=allocator,header=reference.HEADER,full_abi=True))
    for alignment in range(16):
        rows.append(dict(name=f'actual_main_heap_allocate_caller_alignment_{alignment}',entry=0x232d00,
            arguments=[16+alignment,0x12],receiver=0x10061000,header=reference.HEADER,full_abi=True,
            stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1)))
    for size in (8193,16384,65536):
        rows.append(dict(name=f'actual_sized_router_secondary_allocate_{size}',entry=0x149da0,
            arguments=[size,0x12],receiver=0x486080,header=reference.HEADER,full_abi=True))
    for size in (1,16,8192):
        rows.append(dict(name=f'actual_sized_router_fixed_allocate_{size}',entry=0x149da0,
            arguments=[size,0x12],receiver=0x486080,header=reference.HEADER,full_abi=True,
            input_words=[(0x486064,1)]))
    rows.append(dict(name='actual_sized_router_exhausted_secondary_default_entry',entry=0x149da0,
        arguments=[0x80000,0x12],receiver=0x486080,header=reference.HEADER,full_abi=True,
        required_entries=[0x149930]))
    for alignment in (0,1,2,4,8,16):
        for cursor,size,limit in ((0,1,0),(1,1,16),(15,16,31),(0,0,0),(0xfffffff0,32,0x100)):
            rows.append(dict(name=f'actual_main_heap_gap_{alignment}_{cursor:x}_{size}_{limit}',entry=0x232d30,
                arguments=[0x10061000,alignment],receiver=cursor,header=reference.HEADER,full_abi=True,
                input_registers=dict(edx=size,esi=reference.HEADER+0x100,edi=limit)))
    for sink in (reference.SP-8,reference.SP+4,reference.HEADER+0x101,
        0x10061018,0x1006101c,0x10061020):
        rows.append(dict(name=f'actual_main_heap_gap_sink_alias_{sink:x}',entry=0x232d30,
            arguments=[0x10061000,2],receiver=1,header=reference.HEADER,full_abi=True,direction_flag=True,
            input_registers=dict(eax=0xfedcba98,edx=17,ebx=0xa1b2c3d4,ebp=0x98765432,esi=sink,edi=64)))
    for entry in (0x232ad0,0x232d30):
        row=dict(name=f'actual_main_heap_invalid_alignment_boundary_{entry:x}',entry=entry,
            receiver=0x10166410 if entry==0x232ad0 else 0,header=reference.HEADER,full_abi=True,
            arguments=[3,16,0x12]if entry==0x232ad0 else [0x10061000,3],required_entries=[0])
        if entry==0x232d30:row['input_registers']=dict(edx=16,esi=reference.HEADER+0x100,edi=128)
        rows.append(row)
    return dict(member=member,**proof,bank_address=0x10002000)
