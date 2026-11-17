"""Opt-in box/zero/polygon pairs backed by actual immutable vtable slots."""
import copy
import struct
def box_inputs(image,initial):
    tables={'polygon':0x4620d0,'zero':0x4613a0,'box':0x462090}
    for kind,target in(('polygon',0x1f6610),('zero',0x2e8d20),('box',0x306790)):
        if struct.unpack('<I',image.read(tables[kind]+0x30,4))[0]!=target:raise ValueError('Actual support slot drift '+kind)
    rows=list(initial)
    for first,second in(('box','polygon'),('polygon','box'),('box','zero'),('zero','box'),('box','box')):
        for seed in initial[:16]:
            row=copy.deepcopy(seed);row['id']=len(rows);row['name']=f'mixed-{first}-{second}-{seed["id"]}'
            bank=row['banks'][0];data=bytearray.fromhex(bank['before']);base=bank['address']
            for shape,kind in((0x61005000,first),(0x61006000,second)):
                struct.pack_into('<I',data,shape-base,tables[kind])
                if kind=='box':
                    bits=(0x3f800000,0x80000000,0x7fc00123,0xffc00456)if seed['id']%2 else(0xbf000000,0x40000000,0x3e800000,0x7fc01234)
                    struct.pack_into('<4I',data,shape+16-base,*bits)
            bank['before']=data.hex();rows.append(row)
    return rows
