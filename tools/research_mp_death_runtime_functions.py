#!/usr/bin/env python3
"""Complete death runtime owner correspondence from authenticated images."""
import research_mp_sandbox_update_functions as matcher
matcher.ENTRIES = (0x170a30,0x170a70,0x170b60,0x23fb00,0x23ff50,0x23fea0,0x23fd40,
                   0x1732b0,0x173780,0x1d7310,0x1d7710,0x143580,0x1441d0,0x142d00,0x143ad0)
if __name__ == '__main__':
    matcher.main()
