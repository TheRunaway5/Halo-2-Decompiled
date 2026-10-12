#!/usr/bin/env python3
"""Complete authenticated correspondence for constraint/world membership owners."""
import argparse,sys
from pathlib import Path
import research_mp_power_contact_batches_correspondence as owner
from oracle_mp_power_constraint_membership_1_1 import OUT,verify
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'final/later-correspondence/report.json');a=ap.parse_args();verify()
 owner.ENTRIES={0x313800,0x31d2b0,0x322320,0x31c520,0x31e7d0};owner.SHORT=set();owner.OUT=OUT;owner.__file__=__file__
 sys.argv=[sys.argv[0],'--output',str(a.output)];owner.main()
if __name__=='__main__':main()
