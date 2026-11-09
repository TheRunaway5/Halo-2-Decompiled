#!/usr/bin/env python3
"""BSP bootstrap models with actual iterator coverage in the positive gate."""
import validate_native_bsp_bootstrap_host_tu15 as model
model.STAGE=model.ROOT/'analysis/physics/native-bsp-object-association-host-stage-v3-20261103/staged'
if __name__=='__main__':model.main()
