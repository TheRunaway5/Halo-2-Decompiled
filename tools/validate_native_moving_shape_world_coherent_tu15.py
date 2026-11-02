#!/usr/bin/env python3
"""Exact frozen1480+moving stage: deduplicated native World default/prefix models."""
import inspect,hashlib
import validate_native_world_island_merge_stage_tu15 as parent

def main():
 source=inspect.getsource(parent.main)
 # The source-only runtime checkpoint deliberately excludes Python tools.
 # Reuse the immutable178-CPP list that qualified this exact1480 base, then
 # retain the parent's production-CMake additions and de-duplication.
 proof=parent.ROOT/'analysis/physics/native-broad-phase-edges-world-model-20261102-first/gcc/report.json'
 assert hashlib.sha256(proof.read_bytes()).hexdigest()=='326d299ae77935ce2355208f92f5f767ba6326aafbaf1d0eb052068c1db6db6c'
 start=source.index('  tree=ast.parse(');end=source.index('  cmake=',start)
 source=source[:start]+"  files=json.loads(BASE_CLOSURE.read_text())['compiled_cpp_closure']\n"+source[end:]
 # Production links the existing feature archive. This standalone direct-CPP
 # model needs only its demanded arithmetic object, never the unused handoff.
 old="  files=list(dict.fromkeys(files));";assert source.count(old)==1
 source=source.replace(old,"  files+=['src/physics/native_tu15_contact_features.cpp']\n"+old)
 env=dict(parent.__dict__,BASE_CLOSURE=proof);exec(source,env);return env['main']()
if __name__=='__main__':raise SystemExit(main())
