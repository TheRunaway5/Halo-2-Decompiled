"""Authored split programs leading to genuine scene-query leaves and resumes."""
import copy

AXIS={0x13:2,0x14:1,0x15:2,0x16:0,0x17:1,0x18:0,0x19:2,0x1a:1,0x1b:0,0x1c:0}

def compose(image,rows,smoke):
    # Plain collector fixtures already own all scenario/query banks. Change
    # authored MOPP program/variables/cache DATA only; no child outcome or
    # returned register packet is supplied.
    seeds=rows if smoke else [rows[index*4+variant]for index in (0,18,21,34,41,61,81)for variant in range(4)]
    opcodes=(0x10,)if smoke else (*range(0x10,0x1d),*range(0x20,0x26))
    result=[]
    for opcode in opcodes:
        axis=AXIS.get(opcode,opcode-0x10 if opcode<0x20 else opcode-0x20 if opcode<0x23 else opcode-0x23)
        for reverse in (False,True):
            for original in seeds:
                row=copy.deepcopy(original)
                def update(address,data):
                    bank=next(bank for bank in row['banks']if bank['address']<=address and address+len(data)<=bank['address']+len(bank['before'])//2)
                    content=bytearray.fromhex(bank['before']);start=address-bank['address'];content[start:start+len(data)]=data;bank['before']=content.hex()
                def word(address,value):update(address,int(value&0xffffffff).to_bytes(4,'little'))
                program=bytearray(64)
                if opcode<0x20:program[:4]=bytes((opcode,64,192,16));left=4;right=20
                elif opcode<0x23:program[:3]=bytes((opcode,64,16));left=3;right=19
                else:program[:7]=bytes((opcode,64,192,0,0,0,16));left=7;right=23
                program[left]=program[right]=0x30;update(0x61000100,program)
                word(0x61000814,0x3f800000)
                word(0x61000910,0x61000e00);word(0x61000914,0x3f800000)
                update(0x61000e00,bytes(16))
                fraction=0x3e800000 if row['name'].endswith('_collector2')else 0x3f800000
                word(0x61000954,fraction)
                start=[0,0,0,0x3f800000];end=[0,0,0,0xbf800000]
                start[axis]=0x447a0000 if reverse else 0xc47a0000
                end[axis]=0xc47a0000 if reverse else 0x447a0000
                for lane,value in enumerate(start+end):
                    word(0x61000f00+lane*4,value);word(0x61000920+lane*4,value)
                row.update(id=len(result),name=row['name']+f'_split{opcode:x}_reverse{int(reverse)}',
                    complete_controller=True,native_inverse=True,max_steps=30000,
                    bounds=0x61000f00,program_opcode=opcode)
                result.append(row)
    return result


def setup_callers(image,rows):
    """Turn authored ray DATA into actual30B890 formals; compute no child result."""
    import struct
    normalization=int.from_bytes(image.read(0x46a1bc,4),'little')
    # Actual normalization is a positive normal power of two. Select its exact
    # reciprocal as code scale so the authored spatial split coordinates stay
    # unchanged after the genuine caller computes its quantization.
    assert normalization&0x807fffff==0 and 0<(normalization>>23)<254
    code_scale=(254-(normalization>>23))<<23
    for row in rows:
        def bank(at,size):
            return next(b for b in row['banks']if b['address']<=at and at+size<=b['address']+len(b['before'])//2)
        def read(at,size):
            b=bank(at,size);start=at-b['address'];return bytes.fromhex(b['before'])[start:start+size]
        def put(at,data):
            b=bank(at,len(data));content=bytearray.fromhex(b['before']);start=at-b['address'];content[start:start+len(data)]=data;b['before']=content.hex()
        def word(at,value):put(at,struct.pack('<I',value&0xffffffff))
        cache=row['gp'][1];code=0x61000e00;ray=0x61000f00;sp=row['gp'][4]
        program=read(0x61000100,64);put(code,bytes(16));word(code+12,code_scale);put(code+0x30,program)
        collection=int.from_bytes(read(cache+0x64,4),'little')
        context=int.from_bytes(read(cache+0x60,4),'little')
        collector=int.from_bytes(read(cache+0x5c,4),'little')
        word(ray+0x20,0xaabbccdd);word(ray+0x24,0)
        for index,value in enumerate((0x63000000,collection,code,ray,context,collector)):word(sp+index*4,value)
        row.update(entry=0x30b890,matrix=collection,source=code,setup_caller=True,
                   name=row['name']+'_actual_setup',max_steps=40000)
    return rows


def group_filters(image,rows):
    """Use the authored group-filter and stack-query virtual interfaces."""
    assert int.from_bytes(image.read(0x41c654,4),'little')==0x2e9920
    assert int.from_bytes(image.read(0x466284+0x34,4),'little')==0x24dec0
    assert int.from_bytes(image.read(0x466284+0x38,4),'little')==0x212330
    for index,row in enumerate(rows):
        def word(at,value):
            bank=next(b for b in row['banks']if b['address']<=at and at+4<=b['address']+len(b['before'])//2)
            data=bytearray.fromhex(bank['before']);start=at-bank['address'];data[start:start+4]=(value&0xffffffff).to_bytes(4,'little');bank['before']=data.hex()
        filter=0x61000a00;cache=row['gp'][1]
        word(0x61000f24,filter);word(filter,0x41c654)
        # Field/mask words are authored inputs, not supplied child responses.
        # Collection filter info is computed by its genuine212330 method.
        enabled=index%7!=0
        for lane in range(32):word(filter+12+4*lane,((0xcccc0000|(lane<<4))|1)if enabled else 0)
        word(0x61000f20,(0,0xaabb0000,0xffff0000)[index%3]|(index%32))
        word(0x61000b00,0x466284)
        row.update(group_filter=True,name=row['name']+('_filter_enabled'if enabled else'_filter_disabled'))
    return rows


def array_collections(image,rows):
    """Use the real list collection's child-pointer/filter-word row methods."""
    assert int.from_bytes(image.read(0x461f98+0x34,4),'little')==0x306220
    assert int.from_bytes(image.read(0x461f98+0x38,4),'little')==0x306230
    for index,row in enumerate(rows):
        if index&1:continue # Keep the genuine zero-info collection in scope.
        def word(at,value=None):
            bank=next(b for b in row['banks']if b['address']<=at and at+4<=b['address']+len(b['before'])//2)
            data=bytearray.fromhex(bank['before']);start=at-bank['address']
            if value is None:return int.from_bytes(data[start:start+4],'little')
            data[start:start+4]=(value&0xffffffff).to_bytes(4,'little');bank['before']=data.hex()
        datum=word(0x61000b0c);group=word(0x61000f20);kind=(index//6)%3;key=1+(index//2)%3
        info=group if kind==0 else 0x12340000|(index%32)if kind==1 else 0
        word(0x61000b00,0x461f98);word(0x61000b0c,0x61000780)
        word(0x61000b08,8)
        for lane in range(8):
            word(0x61000780+8*lane,0x61000700+8*lane)
            word(0x61000784+8*lane,info if lane==key else 0x87650000|lane)
            # Actual nested-query read DATA, normally published by24DEC0.
            word(0x61000700+8*lane,0x46625c);word(0x61000704+8*lane,datum)
        header=4 if row['program_opcode']<0x20 else 3 if row['program_opcode']<0x23 else 7
        word(0x61000e30+header,0x30+key);word(0x61000e30+header+16,0x30+key)
        enabled=index%7!=0
        for lane in range(32):word(0x61000a0c+4*lane,0xffffffff if enabled else 0)
        row.update(array_collection=True,array_key=key,name=row['name']+f'_array_collection_key{key}')
    return rows
