#!/usr/bin/env python3
"""Same native-only production body helper on actual typed BEFORE inputs."""
import ast,inspect
import validate_native_contact_body_update_actual_tu15 as parent
import validate_native_contact_body_update_binding_tu15 as binding
def main():
 source=inspect.getsource(parent.main);tree=ast.parse(inspect.getsource(binding.main));values={}
 for node in ast.walk(tree):
  if isinstance(node,ast.Assign)and len(node.targets)==1 and isinstance(node.targets[0],ast.Name)and node.targets[0].id in('old','new'):
   values[node.targets[0].id]=ast.literal_eval(node.value)
 assert set(values)=={'old','new'}
 changes=[('#include "halo2/native_tu15_contact_body_update.hpp"','#include "halo2/native_tu15_contact_body_update_binding.hpp"'),(values['old'],values['new'])]
 transform=''.join('assert driver.count('+repr(a)+')==1;driver=driver.replace('+repr(a)+','+repr(b)+');'for a,b in changes)
 anchor=' source=source.replace(needle,extra+needle)'
 patch='source=source.replace('+repr(anchor)+','+repr(' extra+='+repr(transform)+'\n'+anchor)+');'
 needle='source=inspect.getsource(parent.main);';assert source.count(needle)==1;source=source.replace(needle,needle+patch)
 environment=dict(parent.__dict__);exec(source,environment);return environment['main']()
if __name__=='__main__':raise SystemExit(main())
