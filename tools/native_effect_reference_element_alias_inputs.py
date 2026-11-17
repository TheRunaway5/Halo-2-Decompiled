"""Authored caller/singleton stack aliases; unchanged CPU computes all results."""
import copy

def append(rows):
    aliases=[]
    def variant(seed,name):
        row=copy.deepcopy(seed);row.update(id=len(rows)+len(aliases),name=name)
        banks={bank['address']:bytearray.fromhex(bank['before'])for bank in row['banks']}
        def put(address,value,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:
                    data[address-base:address-base+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');return
            raise ValueError('Effect-reference authored alias escaped bank')
        def get(address,width=4):
            for base,data in banks.items():
                if base<=address<=base+len(data)-width:return int.from_bytes(data[address-base:address-base+width],'little')
            raise ValueError('Effect-reference alias stack escaped bank')
        def finish():
            sp=row['gp'][4];put(sp,0x63000000);row.update(matrix=get(sp+4),source=get(sp+8))
            for bank in row['banks']:bank['before']=banks[bank['address']].hex()
            aliases.append(row)
        return row,put,get,finish
    for seed in(rows[6],rows[16]):
        for which in('saved_si','return'):
            row,put,get,finish=variant(seed,'accessor-singleton-'+which+'-'+str(seed['id']))
            group=0x5052544d if seed['id']==6 else 0x70727433
            singleton=0x486048 if group==0x5052544d else 0x48603c;table=get(0x4f0414)
            if which=='saved_si':
                row['gp'][4]=singleton+8
                put(table+(row['gp'][2]&65535)*16+8,0x63000000)
            else:
                row['gp'][4]=singleton+4;row['gp'][6]=get(singleton);row['gp'][2]=0x63000000
                put(table,group);put(table+8,0x61002000)
            finish()
    for index in(0,7,0x8000,0xffff):
        row,put,get,finish=variant(rows[0],'static-reference-saved-si-index-'+str(index))
        sp=row['gp'][4];ref=sp-18;row['gp'][1]=ref;row['gp'][6]=(index<<16)|((index^0xa55a)&65535)
        table=get(0x4f0414);signed=index if index<32768 else index-65536
        put(table+signed*16,0x62736474);put(table+index*16+8,0x61002000)
        put(ref+14,0x1234,2);put(ref+0x10,index);finish()
    for event in(0,1,0xffffffff,0x80000000):
        row,put,get,finish=variant(rows[1],'effect-reference-saved-si-event-'+str(event))
        sp=row['gp'][4];ref=sp-24;row['gp'][1]=ref;row['gp'][6]=event
        put(ref+14,3,2);put(ref+0x10,0x1357ffff);put(ref+0x14,0x12345678)
        put((0x61004080+(event*56&0xffffffff)+0x34)&0xffffffff,0x61005000);finish()
    return rows+aliases
