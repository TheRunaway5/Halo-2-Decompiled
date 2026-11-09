#!/usr/bin/env python3
"""BSP bootstrap exact models through the sole strict-FP static target."""
import validate_native_bsp_bootstrap_host_tu15 as model
model.STAGE=model.ROOT/'analysis/physics/native-bsp-object-association-host-stage-v4-20261103/staged'
if __name__=='__main__':model.main()
