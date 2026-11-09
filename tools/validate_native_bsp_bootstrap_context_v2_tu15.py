#!/usr/bin/env python3
"""Rebased BSP Host contexts, preserving the canonical Gauss additions."""
import validate_native_bsp_bootstrap_context_tu15 as context
context.STAGE=context.ROOT/'analysis/physics/native-bsp-object-association-host-stage-v2-20261103/staged'
if __name__=='__main__':context.main()
