#!/usr/bin/env python3
"""Export actual biped velocity dispatch, island admission and angular delivery.

Inputs and ordered retained stores are taken from unchanged natural admission
and world insertion. No native result, substituted island, force admission or
motion class is used. Caller stack identities are retained solely as ABI scratch.
"""
from export_mp_biped_body_placement import main

if __name__ == '__main__':
    main({'001c8430', '001d5600', '001d4840'}, __doc__, __file__)
