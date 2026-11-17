"""Backed TU1.5 1046C0 high-key clock, record, flag and unknown-return fixtures."""
from validate_native_object_record_ratio_public_tu15 import REGIONS, parent_rows

KEYS = {
    0x1700056c: 0x104c8d, 0x1700056b: 0x104c1d,
    0x16000571: 0x104c6a, 0x16000579: 0x104c4b,
    0x14000570: 0x104c6a, 0x14000578: 0x104be1,
    0x1500056e: 0x104cc5, 0x1700056f: 0x104cc5,
    0x16000572: 0x1049d0, 0x14000577: 0x1049d0, 0x1700057f: 0x1049d0,
}
CLOCK=(0,1,19,1234,-19,-32123,32767,-32768)
ROW=(0x00000000,0x80000000,0x3e800000,0xbf800000,0x3f800000,0x40000000,0x7fc01234,0x7fa05678)


def high_corpus(image):
    rows=parent_rows(image)[:len(KEYS)*len(CLOCK)]
    for n,row in enumerate(rows):
        case,selector=divmod(n,len(KEYS))
        key=list(KEYS)[selector]
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}

        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:return data,address-base
            raise ValueError(f'High key fixture escaped {address:x}')

        def get(address):
            data,offset=locate(address,4)
            return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value,width=4):
            data,offset=locate(address,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        datum=row['matrix'];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        sp=row['gp'][4]
        put(0x46a178,int.from_bytes(image.read(0x46a178,4),'little'))
        put(definition+0x2c0,(-1,0,1,2,1,3,4,2)[case])
        put(definition+0x2d0,(-1,0,2,2,1,2,3,4)[case])
        put(body+0x22c,CLOCK[case],2)
        put(body+0x16e,(0,2,4,6,0xff,1,2,4)[case],1)
        for index in range(2):
            put(body+index*0x34+0x1b4,ROW[(case+index)%len(ROW)])
            put(body+index*0x34+0x1b8,ROW[(case+index+2)%len(ROW)])
        if case==4:put(sp+16,0x61003e01)
        if case==7:put(sp+12,sp-4)
        put(sp+8,key)
        row.update(name=f'1046c0-high-key-{key:x}-{case}',source=key,
                   expected_high_route=KEYS[key],banks=[dict(address=b,before=banks[b].hex()) for b,_ in REGIONS])
    return rows
