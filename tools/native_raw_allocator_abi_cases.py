"""Actual raw allocation inputs; all release pointers come from original code."""
def extend(reference,rows,selected_rows):
    for entry,receiver in ((0x149bf0,0x486080),(0x233160,0x10076040),(0x232aa0,0x10061000)):
        for size in (0,1,15,16,32,128,496,512,1024,8176,8192,8193,65536,0xffffffff):
            rows.append(dict(name=f'actual_raw_allocate_{entry:x}_{size:x}',entry=entry,arguments=[size,0x12],
                receiver=receiver,header=reference.HEADER,full_abi=True,raw_owner_transport=True))
    for entry in (0x149c40,0x2331a0):
        for size in (0,1,15,16,32,128,496,512,1024,8176):
            rows.append(dict(name=f'actual_raw_release_{entry:x}_{size}',entry=entry,arguments=[0],
                receiver=0x486080 if entry==0x149c40 else 0x10076040,header=reference.HEADER,
                full_abi=True,raw_owner_transport=True,prepare_raw_bytes=size,prepare_raw_argument=True))
    for size in (8192,8193,65536):
        rows.append(dict(name=f'actual_raw_secondary_release_{size}',entry=0x149c40,arguments=[0],
            receiver=0x486080,header=reference.HEADER,full_abi=True,raw_owner_transport=True,
            prepare_raw_bytes=size,prepare_raw_argument=True))
    rows.append(dict(name='actual_raw_release_null',entry=0x149c40,arguments=[0],receiver=0x486080,
        header=reference.HEADER,full_abi=True,raw_owner_transport=True))
    for selected in selected_rows:
        for entry,receiver in ((0x149c40,0x486080),(0x232ac0,0x10061000)):
            rows.append(dict(name=f'actual_raw_fixed_release_{entry:x}_{selected["index"]}',entry=entry,
                arguments=[selected['data']],receiver=receiver,header=reference.HEADER,
                full_abi=True,raw_owner_transport=True))
    for alignment in range(16):
        rows.append(dict(name=f'actual_raw_fixed_mode_alignment_{alignment}',entry=0x149bf0,
            arguments=[16+alignment,0x12],receiver=0x486080,header=reference.HEADER,
            full_abi=True,raw_owner_transport=True,stack_pointer=reference.SP+alignment,
            input_words=[(0x486064,1)],direction_flag=bool(alignment&1)))
        rows.append(dict(name=f'actual_raw_primary_release_alignment_{alignment}',entry=0x149c40,
            arguments=[0],receiver=0x486080,header=reference.HEADER,full_abi=True,raw_owner_transport=True,
            stack_pointer=reference.SP+alignment,prepare_raw_bytes=31+alignment,prepare_raw_argument=True,
            direction_flag=bool(alignment&1)))
    rows.append(dict(name='actual_raw_exhausted_default_entry',entry=0x149bf0,arguments=[0x80000,0x12],
        receiver=0x486080,header=reference.HEADER,full_abi=True,raw_owner_transport=True,required_entries=[0x149930]))
    rows.append(dict(name='actual_raw_default_release_entry',entry=0x149c40,arguments=[reference.DATA],
        receiver=0x486080,header=reference.HEADER,full_abi=True,raw_owner_transport=True,required_entries=[0x149930]))
    rows.append(dict(name='actual_raw_tagged_oversize_crt_entry',entry=0x2331a0,arguments=[reference.DATA],
        receiver=0x10076040,header=reference.HEADER,full_abi=True,raw_owner_transport=True,
        input_words=[(reference.DATA-16,0x02345656),(reference.DATA-12,8193),(reference.DATA-8,0x12)],
        required_entries=[0x1cb560]))
    rows.append(dict(name='actual_crt_tail_entry',entry=0x2e1030,arguments=[reference.DATA],receiver=0x10076040,
        header=reference.HEADER,full_abi=True,raw_owner_transport=True,required_entries=[0x1cb560]))
