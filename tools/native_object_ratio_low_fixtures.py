"""Backed TU1.5 1046C0 low-key and signed-ratio fixtures."""
import copy
from validate_native_object_record_ratio_public_tu15 import REGIONS, parent_rows

KEYS = {
    0x05000024:0x1048b8, 0x09000568:0x104955, 0x04000567:0x1048d4, 0x03000577:0x1048e1,
    0x05000081:0x1048ee, 0x0a000066:0x10490f, 0x0b0005a9:0x10498e, 0x0c000569:0x10476f,
    0x0e000572:0x104a98, 0x10000573:0x104a98, 0x0f000574:0x104b50,
    0x1200056a:0x104b79, 0x1400056d:0x104b79, 0x0e000001:0x1049d0, 0x05000023:0x1049d0,
}
VALUES=(0,0x80000000,0x3e800000,0xbf800000,0x3f800000,0x40000000,0x7fc01234,0x7fa05678)


def low_corpus(image):
    source=parent_rows(image);rows=[]
    for case in range(len(VALUES)):
        for key,route in KEYS.items():
            row=copy.deepcopy(source[len(rows)%len(source)])
            banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}

            def locate(address,width):
                for base,data in banks.items():
                    if base<=address<=base+len(data)-width:return data,address-base
                raise ValueError(f'Low key fixture escaped {address:x}')

            def get(address):
                data,offset=locate(address,4)
                return int.from_bytes(data[offset:offset+4],'little')

            def put(address,value,width=4):
                data,offset=locate(address,width)
                data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

            datum=row['matrix'];slots=get(get(0x4ecbd0)+0x44)
            body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
            definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
            table=0x61002600;sp=row['gp'][4]
            put(0x46a0c4,int.from_bytes(image.read(0x46a0c4,4),'little'))
            put(0x61003808,(5,9,100,0xffffffff,0,2,0x80000000,1)[case])
            put(body+0x248,(3,9,98,0,0xffffffff,1,0x80000000,0)[case])
            put(definition+0x150,(0,0x3f800000,0x7fc01234,0x7fa05678,0x40000000,0xbf800000,0,0x3f800000)[case])
            put(definition+0x2d0,(0,1,2,2,4,5,2,1)[case])
            put(definition+0x2c0,(0,1,2,2,2,2,1,2)[case])
            put(definition+0x2c4,table)
            put(body+0x12c,(0,1,2,3,1,0x41,0xff,0x80)[case],1)
            put(body+0x16c,(0,2,0x20,0x22,0x22,0xff,2,0)[case],1)
            put(body+0x175,(0,1,3,17,128,255,2,7)[case],1)
            for index in range(5):
                put(body+index*0x34+0x1b4,VALUES[(case+index)%len(VALUES)])
                put(body+index*0x34+0x1bc,VALUES[(case+index+2)%len(VALUES)])
            put(body+0x180,VALUES[case]);put(body+0x184,VALUES[(case+1)%len(VALUES)])
            put(body+0x18c,VALUES[(case+2)%len(VALUES)])
            for index in range(2):
                put(table+index*0x5c+0xa,(0,7,0,-3,2,1,-1,32767)[case],2)
                put(body+index*16+0x22c,(0,1,-2,32767,-32768,13,6,-2)[(case+index)%8],2)
            if route==0x10476f:
                put(definition+0x2c8,0 if case%2==0 else 1)
                put(0x61003300+0x1c,0x40000000)
                put(0x61003300+0x28,0x3f000000)
                put(body+0x20c,2,1)
            if case==3:put(sp+16,0x61003e01)
            if case==7:put(sp+12,sp-4)
            put(sp+8,key)
            row.update(id=len(rows),name=f'1046c0-low-key-{key:x}-{case}',source=key,max_steps=300,
                       expected_low_route=route,banks=[dict(address=b,before=banks[b].hex()) for b,_ in REGIONS])
            rows.append(row)
    return rows
