"""Authenticated lazy-heap before inputs, plus separately labelled variants."""
import hashlib
import json
import lzma
import subprocess


def load(reference,image):
    banks,provenance=reference.reference_inputs(image)
    manifest=json.loads(lzma.decompress(reference.ARCHIVE.with_name('all-file-hashes.json.xz').read_bytes()))
    def read(name):
        member=reference.MEMBER+name
        raw=subprocess.check_output(['tar','--zstd','-xOf',str(reference.ARCHIVE),member])
        assert len(raw)==manifest[member]['bytes'] and hashlib.sha256(raw).hexdigest()==manifest[member]['sha256']
        return raw
    capture=json.loads(read('capture.json'));members={}
    for address in (0x486074,0x48605c,0x4f2d34,0x1017d030,0x4f2d50,0x4f2d54,0x411568):
        region=next(r for r in capture['regions'] if r['address']<=address and address+4<=r['address']+r['size'])
        name=region['before_file'];raw=read(name);banks[region['address']]=raw
        members[name]=manifest[reference.MEMBER+name]
    def word(address):return next(reference.word(raw,address-first) for first,raw in banks.items() if first<=address and address+4<=first+len(raw))
    assert word(0x486074)==0 and word(0x48605c)==0x10061000
    assert word(0x4f2d34)==0x1017d000 and word(0x1017d030)==0x1ed1
    assert not any(first<=0x486060 and 0x486064<=first+len(raw) for first,raw in banks.items())
    banks[0x486060]=bytes(4)
    provenance.update(lazy_members=members,genuine_default_binding=0,genuine_pressure_count=0x1ed1,
        declared_uncaptured_output_cell=dict(address=0x486060,bytes=4,initial_value=0,read_by_prefix=False))
    return banks,provenance


def cases(reference):
    rows=[]
    for alignment in range(16):
        common=dict(entry=0x149930,arguments=[],receiver=0x486080,header=reference.HEADER,
            full_abi=True,stack_pointer=reference.SP+alignment,direction_flag=bool(alignment&1),
            input_registers=dict(eax=0x87654321,edx=0xc3c3c3c3,ebx=0xa5a5a5a5,ebp=0x11335577,esi=0x22446688,edi=0x99aabbcc))
        rows.append(dict(common,name=f'genuine_lazy_allocate_entry_{alignment}',cohort='genuine-slow-prefix',
            record_timestamp_inputs=True,required_entries=[0x12fe00]))
        rows.append(dict(common,name=f'genuine_lazy_clock_entry_{alignment}',cohort='genuine-clock-boundary',
            stop_before_sites=[0x149958],required_entries=[0x149958]))
        rows.append(dict(common,name=f'declared_lazy_no_pressure_{alignment}',cohort='declared-pressure-input',
            input_words=[(0x1017d030,0)],record_timestamp_inputs=True,required_entries=[0x1cd130]))
        rows.append(dict(common,name=f'declared_lazy_initialized_{alignment}',cohort='declared-existing-heap-binding',
            input_words=[(0x486074,0x10061000)]))
    for count,constructor in ((0xffffffff,0x10061000),(0,(-0x105440)&0xffffffff)):
        rows.append(dict(entry=0x149930,arguments=[],receiver=0x486080,header=reference.HEADER,full_abi=True,
            name=f'declared_lazy_constructor_{count:x}_{constructor:x}',cohort='declared-pressure-constructor-input',
            input_words=[(0x1017d030,count),(0x48605c,constructor)],record_timestamp_inputs=True,required_entries=[0x1cd130]))
    return rows
