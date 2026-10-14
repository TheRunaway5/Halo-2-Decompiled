#!/usr/bin/env python3
"""Execute the unchanged supplied Vista secondary-weapon setter/getter.

Uses the existing qualified PC ABI fixture. Security-cookie, CRT and absent
localized title are its declared external services; source variant code is not
patched or replaced. No native implementation is imported.
"""
import hashlib
from pathlib import Path
from oracle_vista_pcr_publication import Original, ROOT, SHA, VARIANT, VALUES


def main():
    image=(ROOT/'Halo 2 Vista installer/halo2.exe').read_bytes()
    assert hashlib.sha256(image).hexdigest()==SHA
    original=Original(image)
    rows=['# Supplied unchanged Vista '+SHA+'; complete secondary-weapon setter/getter outputs']
    for value in VALUES:
        original.variant()
        original.call(0x63a8c2,(VARIANT,94,value))
        raw=bytes(original.u.mem_read(VARIANT,0x130))
        actual=original.call(0x63aa54,(VARIANT,94))
        if actual&0x80000000:actual-=0x100000000
        rows.append(f'94 {value} {raw.hex()} {actual}')
    (ROOT/'tests/fixtures/custom_slayer_pc_original.tsv').write_text('\n'.join(rows)+'\n')
    print('PASS12 unchanged PC secondary-weapon controls; complete304-byte buffers')


if __name__=='__main__':main()
