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
