"""Additional declared whole-call inputs using genuine zero-support vtables."""
import copy
import struct

def mixed_inputs(image,polygon_rows):
    table=0x4613a0
    if struct.unpack('<I',image.read(table+0x30,4))[0]!=0x2e8d20:raise ValueError('Actual4613A0 zero support slot30 drift')
    rows=list(polygon_rows)
    for first_zero,second_zero in((True,False),(False,True),(True,True)):
        # Keep the32 polygon-only saved-BX/BP aliases in the original160 rows.
        # A zero point cannot preserve a caller RET word overlapping its store;
        # the additional mixed-family cases use genuine ordinary output aliases.
        for seed in polygon_rows[:32]:
            row=copy.deepcopy(seed);row['id']=len(rows);row['name']=f'mixed-zero-{int(first_zero)}{int(second_zero)}-{seed["id"]}'
            bank=row['banks'][0];data=bytearray.fromhex(bank['before']);base=bank['address']
            if first_zero:struct.pack_into('<I',data,0x61005000-base,table)
            if second_zero:struct.pack_into('<I',data,0x61006000-base,table)
            bank['before']=data.hex();rows.append(row)
    return rows
