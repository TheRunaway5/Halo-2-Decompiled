"""Declared31650 inputs. Outcomes come only from unchanged original execution."""
import struct

SECTIONS_MAP={'rdata':(0x46a000,4096),'objects':(0x4ec000,4096),'tags':(0x4f0000,4096),
    'contexts':(0x559000,0x4000),'data':(0x1000000,0x10000),'stack':(0x62000000,0x20000)}
SP,STOP=0x62018000,0x63000000
CTX,D=0x5598f8,0x1000000
LPOOL,LRECS,DEF,TAGS=D,D+0x100,D+0x400,0x4f0100
LIGHT,DEFINITION=0x77010001,0x88020002
REC=LRECS+0x110
QUEUES={'c':(0xc,D+0x2000,64),'o':(0x14,D+0x5000,256),'i':(0x18,D+0x8000,96)}
NONE=0xffffffff
A,B,C=0xe0010001,0xe0030003,0xe0050005


def cases(image):
    rows=[]
    def row(name,flags,owner=A,root=B,queues=None,a2=1,shadow=None,changes=(),xmm=None):
        queues=queues or {};banks=[dict(address=at,before=bytearray(size))for at,size in SECTIONS_MAP.values()]
        def put(at,value,size=4):
            for b in banks:
                if b['address']<=at and at+size<=b['address']+len(b['before']):
                    b['before'][at-b['address']:at-b['address']+size]=(value&((1<<(8*size))-1)).to_bytes(size,'little');return
            raise ValueError('Undeclared31650 input '+hex(at))
        banks[0]['before'][:]=image.read(0x46a000,4096)
        put(0x4ecbdc,LPOOL);put(LPOOL+0x44,LRECS);put(REC+0xf8,0x40400000)
        put(0x4f0414,TAGS);put(TAGS+16*(DEFINITION&65535)+8,DEF);put(DEF,flags)
        typed=True
        for key,(offset,header,capacity)in QUEUES.items():
            records=queues.get(key,[]);count=queues.get(key+'#',len(records));put(CTX+offset,header)
            put(header,capacity);put(header+4,count,2);typed&=count==len(records)and count<capacity
            for i in range(4):put(header+8+4*i,header+0x400*(i+1))
            for i,(obj,bits)in enumerate(records):
                put(header+0x400+2*i,bits,2);put(header+0x800+4*i,obj)
                put(header+0xc00+4*i,0x5a5a0000+i);put(header+0x1000+2*i,0x700+i,2)
        put(SP,STOP);put(SP+4,owner);put(SP+8,root);put(SP+12,a2|0xc3c3c300)
        put(SP+16,SP+0x80 if shadow is None else shadow);put(SP+0x80,0x5a,1)
        for at,value,size in changes:put(at,value,size)
        r=dict(id=len(rows),name=name,entry=0x31650,stop_pc=STOP,max_steps=50000,
            gp=[LIGHT,DEFINITION,0x55667788,0xb0b0b0b,SP,0xe0e0e0e,CTX,0xd0d0d0d],
            matrix=owner,source=root,return_pc=STOP,before_fp=[0x27f,0x220,0x1fa0],live_x87=[],
            typed=typed and not changes and shadow is None,
            banks=[dict(address=b['address'],before=b['before'].hex())for b in banks])
        r['xmm']=xmm or [[0x6b000000+r['id']*37+4*n+k for k in range(4)]for n in range(8)]
        rows.append(r);return r
    objs=[(A,0x3000),(B,0),(C,0),(A,0),(B,0xf7ff),(C,0xe800)]
    row('bit16 clusters then instances',0x10000,queues={'c':[(1,0x3000),(2,0),(3,0x2400),(4,0xc800),(5,0x1000),(6,0x2000),(7,0xffff)],'i':[(1,0x3c00),(2,0),(3,0x2000)],'o':objs})
    row('bit17 every object',0x20000,queues={'c':[(1,0x2000)],'o':objs})
    for bits in (0,0x800,0x1000,0x1800,0x2000,0x2800,0x3000,0x3800):row('owner/root bits '+hex(bits),bits,queues={'o':objs})
    for bits,owner,root in ((0x800,NONE,NONE),(0x3800,NONE,NONE),(0x3800,A,A),(0x3800,A,NONE),(0x3800,NONE,B)):
        row('sentinel/root '+hex(bits)+' '+hex(owner)+' '+hex(root),bits,owner,root,{'o':objs})
    row('bit17 before owner/root clear',0x23800,queues={'o':objs})
    row('BSP then objects then owner/root',0x31000,queues={'c':[(1,0x400)],'i':[(1,0)],'o':objs})
    for count in (0,1,2,0x7fff,0x8000,0xffff):
        row('bit2 signed cluster count '+hex(count),4,queues={'c':[(i+1,0x3000)for i in range(min(count,3))],'c#':count,'o':objs})
    row('bit2 ignores a2',4,queues={'c':[(1,0x3000),(2,0x3000)],'o':objs},a2=0)
    row('bit2 and bit16',0x10004,queues={'c':[(1,0x3000),(2,0)],'o':objs})
    row('negative cluster count skips bit16',0x10000,queues={'c':[(1,0x3000)],'c#':0xffff,'o':objs})
    row('negative object count skips bit17',0x20000,queues={'o':objs,'o#':0x8000})
    row('MOVSS with other entry lanes',4,queues={'c':[(1,0x3000),(2,0x3000)],'o':objs},xmm=[[0x11111111*(i+1),2,3,4]for i in range(8)])
    full={'c':[(n,0xe400 if n%2 else 0x3000)for n in range(63)],
        'i':[(n,0xc800 if n%2 else 0xffff)for n in range(95)],
        'o':[((A,B,C)[n%3],(0xffff,0,0x2400)[n%3])for n in range(32)]}
    row('full reserved-slot BSP queues and object duplicates',0x33804,queues=full)
    row('full object queue NONE owner twice',0x21800,NONE,B,{'o':[((A,B,C)[n%3],n*173&65535)for n in range(255)]})
    for bits in (0x800,0x1000,0x2800,0x3000,0x3800):
        row('unrelated flag bits '+hex(bits),0x80040081|bits,queues={'c':[(1,0xdead)],'i':[(2,0xbeef)],'o':objs})
    row('BSP edit changes definition before owner policy',0x13800,queues={'c':[(1,0)],'o':objs},changes=((QUEUES['c'][1]+8,DEF,4),))
    row('owner pinned before BSP edits its argument',0x13800,queues={'c':[(1,0)],'o':objs},changes=((QUEUES['c'][1]+8,SP+4,4),))
    row('root read after BSP edits its argument',0x13800,queues={'c':[(1,0)],'o':objs},changes=((QUEUES['c'][1]+8,SP+8,4),))
    row('root reread after each matching object edit',0x3800,root=0xe0033003,queues={'o':[(A,0),(0xe0033003,0),(0xe0030003,0)]},changes=((QUEUES['o'][1]+8,SP+6,4),))
    row('MOVSS copies the pinned source word without arithmetic',4,queues={'c':[(1,0),(2,0)]},changes=((0x46a0d0,0x7f812345,4),))
    row('late shadow-pointer read after F8 aliases its argument',4,queues={'c':[(1,0),(2,0)]},changes=((LPOOL+0x44,SP+16-0xf8-0x110,4),))
    row('NULL shadow with nonzero a2 returns',4,queues={'c':[(1,0),(2,0)]},shadow=0)
    row('NULL shadow with one cluster returns before dead branch',4,queues={'c':[(1,0)]},shadow=0,a2=0)
    fault=row('NULL shadow original prefix ends before byte0 fault',4,queues={'c':[(1,0x400),(2,0xc000)]},shadow=0,a2=0)
    fault.update(stop_before_entries=[0x318aa],expect_zero_fault=True)
    for fp in ([0x27f,0x4500,0x5f80],[0x7f,0x200,0x9fc0]):
        r=row('MOVSS preserves arbitrary FP controls '+str(fp),4,queues={'c':[(1,0),(2,0)]});r['before_fp']=fp
    # The original resolves only the low16 bits, without a salt check.
    r=row('different full datum salts',0x23804,queues={'c':[(1,0),(2,0)],'o':objs});r['gp'][:2]=[0xffff0001,0x12340002]
    return rows
