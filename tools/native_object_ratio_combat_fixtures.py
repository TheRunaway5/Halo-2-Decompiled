"""Backed TU1.5 1046C0 four-key CombatAction composition fixtures."""
from validate_native_object_table_ratio_public_tu15 import BANKS, seeded_rows

REGIONS=(*BANKS,(0x4ea000,4096))
KEYS=(0x0f0006e5,0x0e0006e6,0x0d0006e7,0x120006e4)
VALUES=(0,0,0,0,0x3f000000,0x3e000000,0x3f800000,0x3f000000,0x3e800000,
        0x7fc01234,0x7fa05678,0x80000000)


def combat_corpus(image):
    source=[r for r in seeded_rows(image) if r['entry']==0x1046c0][:len(KEYS)*len(VALUES)]
    for n,row in enumerate(source):
        case,selector=divmod(n,len(KEYS));key=KEYS[selector]
        banks={b['address']:bytearray.fromhex(b['before']) for b in row['banks']}
        banks[0x4ea000]=bytearray(4096)

        def locate(address,width):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:return data,address-base
            raise ValueError(f'CombatAction fixture escaped {address:x}')

        def get(address):
            data,offset=locate(address,4)
            return int.from_bytes(data[offset:offset+4],'little')

        def put(address,value,width=4):
            data,offset=locate(address,width)
            data[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little')

        datum=row['matrix'];slots=get(get(0x4ecbd0)+0x44)
        body=get((slots+(datum&0xffff)*12+8)&0xffffffff)
        definition=get(get(0x4f0414)+(get(body)&0xffff)*16+8)
        options=get(0x4f3218);table=get(0x52d710);scale=get(0x51df54);sp=row['gp'][4]
        index=(-3,0,1,6)[selector]
        put(options+8,3 if case==0 else 2)
        put(body+0x17e,-1 if case==1 else index,2)
        put(definition+0x290,0 if case==2 else 1,2)
        put(options+0x180,8 if case==3 else 9)
        put(options+0x244,-2 if case==8 else 0 if case==9 else 1,2)
        put(table+0x170+index,8 if case==6 else 0,1)
        put(table+0x1a0+index*2,(1,1,1,1,0,2,1,4,3,1,2,1)[case],2)
        put(scale+4,VALUES[case])
        if case==7:put(sp+16,0x61003e01)
        if case==11:put(sp+12,sp-4)
        put(sp+8,key)
        row.update(name=f'1046c0-combat-key-{key:x}-{case}',source=key,
            expected_combat_route=0x104b13 if selector==3 else 0x1049ee,
            expected_combat_definition=int(case>=2),expected_combat_ratio=int(case>=4),
            banks=[dict(address=b,before=banks[b].hex()) for b,_ in REGIONS])
    return source
