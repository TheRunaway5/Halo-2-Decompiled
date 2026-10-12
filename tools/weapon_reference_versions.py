"""Read-only, pinned original Xbox images used by weapon differential checks."""
import hashlib
from pathlib import Path
from audit_world_vertex_declarations import ROOT, Xbe


def images():
    records = (
        ('TU1.1', ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',
         'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'),
        ('retail', ROOT/'assets/disc/default.xbe',
         '03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'),
        ('TU1.5', Path.home()/'.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe',
         '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'),
    )
    result = []
    for label, path, pin in records:
        x = Xbe(path)
        if hashlib.sha256(x.data).hexdigest() != pin:
            raise ValueError(f'Unexpected original {label} XBE')
        result.append((label, x, pin))
    return result


def unique_entry(x, anchor, offset=0):
    hits = []
    for section in x.sections:
        data = x.section_bytes(section)
        start = 0
        while (found := data.find(anchor, start)) >= 0:
            hits.append(section.va+found-offset)
            start = found+1
    if len(hits) != 1:
        raise ValueError(f'Original function anchor has {len(hits)} matches')
    return hits[0]
