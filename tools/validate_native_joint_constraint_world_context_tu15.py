#!/usr/bin/env python3
"""Both full actual Host/Live contexts for the staged joint counter route."""
import inspect
import validate_native_look_pipeline_context_tu15 as parent
def main():
 source=inspect.getsource(parent.main).replace("'include/halo2/native_tu15_look_pipeline_binding.hpp'","'include/halo2/native_tu15_constraint_step_residuals.hpp'")
 environment=dict(parent.__dict__,__doc__=__doc__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
