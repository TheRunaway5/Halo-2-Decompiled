"""Source-independent, deterministic surface grain for authored menu art.

This is an ordinary SVG procedural material, evaluated by the standard SVG
renderer at the requested output resolution. Only low-dimensional scale and
contrast measurements guide it. It contains no reference image, sampled grid,
or copied source noise. Major wear/reflection contours remain authored paths.
"""
from vista_prompt_svg import element


def material_grain(d,key,frequency=(.48,.63),contrast=.75,octaves=3,seed=11,
                   secondary_frequency=None,secondary_contrast=.45,
                   secondary_octaves=2,secondary_seed=29):
    if not 0<contrast<=2:raise ValueError('Material contrast outside authored range')
    if not 1<=octaves<=4:raise ValueError('Material octave count outside authored range')
    f=element(d.defs,'filter',id=key,filterUnits='userSpaceOnUse',
              primitiveUnits='userSpaceOnUse',x=0,y=0,width=d.width,height=d.height,
              color_interpolation_filters='sRGB')
    element(f,'feTurbulence',type='fractalNoise',baseFrequency=f'{frequency[0]} {frequency[1]}',
            numOctaves=octaves,seed=seed,stitchTiles='stitch',result='independent-grain')
    element(f,'feColorMatrix',type='saturate',values=0,result='neutral-grain',**{'in':'independent-grain'})
    transfer=element(f,'feComponentTransfer',result='opaque-grain',**{'in':'neutral-grain'})
    element(transfer,'feFuncA',type='linear',slope=0,intercept=1)
    element(f,'feComposite',in2='opaque-grain',operator='arithmetic',
            k1=contrast,k2=1-contrast/2,k3=0,k4=0,result='lit-surface',**{'in':'SourceGraphic'})
    surface='lit-surface'
    if secondary_frequency is not None:
        if len(secondary_frequency)!=2 or min(secondary_frequency)<=0:
            raise ValueError('Invalid secondary material frequency')
        if not 0<secondary_contrast<=2 or not 1<=secondary_octaves<=4:
            raise ValueError('Secondary material contrast or octave count outside authored range')
        element(f,'feTurbulence',type='fractalNoise',
                baseFrequency=f'{secondary_frequency[0]} {secondary_frequency[1]}',
                numOctaves=secondary_octaves,seed=secondary_seed,stitchTiles='stitch',result='medium-grain')
        element(f,'feColorMatrix',type='saturate',values=0,result='neutral-medium-grain',**{'in':'medium-grain'})
        transfer=element(f,'feComponentTransfer',result='opaque-medium-grain',**{'in':'neutral-medium-grain'})
        element(transfer,'feFuncA',type='linear',slope=0,intercept=1)
        element(f,'feComposite',in2='opaque-medium-grain',operator='arithmetic',
                k1=secondary_contrast,k2=1-secondary_contrast/2,k3=0,k4=0,
                result='layered-surface',**{'in':surface})
        surface='layered-surface'
    # Restore the source geometry's alpha rather than allowing the generated
    # field to paint the transparent margins of the original logical canvas.
    element(f,'feColorMatrix',type='matrix',
            values='1 0 0 0 0  0 1 0 0 0  0 0 1 0 0  0 0 0 0 1',result='opaque-surface',
            **{'in':surface})
    element(f,'feComposite',in2='SourceGraphic',operator='in',**{'in':'opaque-surface'})
    return f'url(#{key})'
