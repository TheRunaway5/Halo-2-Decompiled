"""Declared whole166B60 and31EC2..31EDA/31C10 cases; no service outcomes."""
import itertools

def append_cases(new):
    d=0x1000000;pool=d+0xa000;headers=d+0xb000;body=d+0xc000;queue=d+0xd000;definition=d+0xd100
    flags=d+0xe000;objects=d+0xe100;values=d+0xe200;associations=d+0xe300;context=0x5598f8
    def prepared(entry,capacity=5,count=0):
        row,put,finish=new(entry);row['kind']='light-parent'if entry==0x31ec2 else'queue-append'
        put(0x4ecbd0,pool);put(pool+0x44,headers);put(context+0x14,queue)
        for index,kind in enumerate((2,0,1),1):
            p=body+(index-1)*256;put(headers+12*index+8,p);put(p+0xaa,kind,1)
            put(p+0x14,0x80010000+index+1 if index<3 else 0xffffffff)
        put(queue,capacity);put(queue+4,count,2);put(queue+8,flags);put(queue+0xc,objects)
        put(queue+0x10,values);put(queue+0x14,associations)
        return row,put,finish
    for capacity,count in [(0,0),(1,0),(5,0),(5,1),(5,3),(5,4),(100,99),(0xffffffff,0),(0x80000000,0),(5,65535)]:
        row,put,finish=prepared(0x166b60,capacity,count);row['name']=f'queue-cap{capacity}-count{count}'
        row['gp'][0]=0x8aabcd42;row['gp'][1]=queue
        row['queue_arguments']=[0x80010001,0x1234abcd,0x11445566,0x5678ffff]
        row['matrix'],row['source']=row['queue_arguments'][:2]
        put(row['gp'][4]+12,row['queue_arguments'][2]);put(row['gp'][4]+16,row['queue_arguments'][3]);finish()
    for alias in ('objects-count','flags-count','value-association-pointer','association-wrap'):
        row,put,finish=prepared(0x166b60);row['name']='queue-alias-'+alias;row['gp'][0]=0x8aabcd42;row['gp'][1]=queue
        args=[1,2,associations+16,65535];row['queue_arguments']=args;row['matrix'],row['source']=args[:2]
        put(row['gp'][4]+12,args[2]);put(row['gp'][4]+16,args[3])
        if alias=='objects-count':put(queue+0xc,queue+4)
        elif alias=='flags-count':put(queue+8,queue+4)
        elif alias=='value-association-pointer':put(queue+0x10,queue+0x14)
        else:put(queue+0x14,queue+4)
        finish()
    for bit,shadow,owner,full in itertools.product((False,True),(0,1,255),(0xffffffff,0x80010001,0x80010002,0x80010003),(False,True)):
        row,put,finish=prepared(0x31ec2,count=4 if full else 0);row['name']=f'parent-bit{bit}-shadow{shadow}-owner{owner}-full{full}'
        row['gp'][3]=owner;row['gp'][6]=context;row['definition_data']=definition;row['shadow_input']=row['gp'][4]+0x10
        put(row['gp'][4]+0x1c,definition);put(row['shadow_input'],shadow,1);put(definition,0x400000 if bit else 0)
        row['stop_pc']=0x31eda;finish()
    # A biped with a vehicle parent must stop at the biped, not at the root.
    # With no biped at all, the same original walk appends the final root.
    row,put,finish=prepared(0x31ec2);row['name']='parent-no-biped';row['gp'][3]=0x80010001;row['gp'][6]=context
    row['definition_data']=definition;row['shadow_input']=row['gp'][4]+0x10
    put(definition,0x400000);put(body+256+0xaa,2,1);put(row['gp'][4]+0x1c,definition);put(row['shadow_input'],0,1)
    row['stop_pc']=0x31eda;finish()
