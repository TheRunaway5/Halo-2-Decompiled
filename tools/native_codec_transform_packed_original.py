"""Exact 283E10 packed-phase EMMS observation using unchanged i386 text."""
import inspect

import native_packed_rotation_original as packed

ENTRIES = (0x283e10,0x120570,0x120740,0x1201e0)


def physical_original_packed(*args,opaque_input=False,**kwargs):
    if tuple(kwargs.get('entries',()))!=ENTRIES:
        raise ValueError('Wrong original codec packed-phase owner set')
    source=inspect.getsource(packed.physical_original_rotation)
    changes=(
        ('!=(0x296180,)',f'!={ENTRIES!r}'),
        ('pc<=0x2961a5u','!executed.contains(0x283f1bu)'),
        ('std::uint32_t(regs.rip)<=0x2961a5u','!executed.contains(0x283f1bu)'),
        ('Original packed rotation did not complete its EMMS interval',
         'Original 283E10 did not retire its own EMMS interval'),
    )
    for old,new in changes:
        if source.count(old)!=1:
            raise ValueError('Original packed-phase observer seam drift: '+old)
        source=source.replace(old,new,1)
    namespace=dict(vars(packed))
    namespace['__file__']=__file__
    exec(compile(source,__file__,'exec'),namespace)
    result,proof=namespace['physical_original_rotation'](*args,opaque_input=opaque_input,**kwargs)
    proof['codec_packed_scope']='Actual 283E10 EMMS retirement; only genuine selected rows'
    return result,proof
