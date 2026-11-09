#!/usr/bin/env python3
"""Compatibility name for the canonical MCC authored-field contact audit.

The former Xbox/Vista investigation route is retired. Use --runtime, --maps,
--case-json and --report-output; run this command through tools/run_test.py.
Every requested case is reported, including failures and negative controls.
"""
import sys
from test_mcc_gravlift_contact_audit_tu15 import main

if __name__ == '__main__':
    if any(argument in ('--xbe', '--ui', '--visible-mirror-display') for argument in sys.argv[1:]):
        raise SystemExit('The obsolete Xbox/Vista lift wrapper is retired. Use tools/test_mcc_gravlift_contact_audit_tu15.py --help for the MCC runtime/census audit.')
    raise SystemExit(main())
