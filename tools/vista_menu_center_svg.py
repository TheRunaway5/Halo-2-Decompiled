"""The Vista centre menu frame: stepped bands and traced circuit decoration.

This recipe uses the 2048x1303 source's native design coordinates. Repeated
engraving is an ordinary SVG pattern with an authored cadence, not pixel
regions. The window, exterior field and blue bands have independent alpha.
"""
from vista_prompt_svg import element
from vista_menu_materials_svg import linear


WINDOW='M0 122 H1107 L1144 84 H2048 V1060 H1010 L977 1092.67 H0 Z'
TOP='M0 70 H1055 L1080 95 H1132 L1107 122 H0 Z'
BOTTOM='M548 1093 H977 L1010 1060 H2048 V1134 H1250 L1228 1112 H566 Z'

# Thin circuit centre lines traced against native-sized bend closeups. The
# footer's second line includes the full long horizontal before its ascent.
TOP_CIRCUITS=(
    'M507.5 122 L512 117.5 H870.5 L883.5 104.5 H1106.5 L1124 122',
    'M908.3 70 L938.5 100.2 H1135',
)
BOTTOM_CIRCUITS=(
    'M689 1093 L702.2 1106.2 H1082.7 L1093.2 1095.7 H1157.5 L1174 1112',
    'M548 1101.5 H1001.5 L1037.5 1065.5 H1185.5 L1191 1060',
)


def clip(d,name,path):
    result=element(d.defs,'clipPath',id=name)
    element(result,'path',d=path,fill_rule='evenodd',clip_rule='evenodd')
    return f'url(#{name})'


def tick_material(d,group,name,x,y,width,height,period=10.5,bar=5.3,opacity=.065):
    pattern=element(d.defs,'pattern',id=name,patternUnits='userSpaceOnUse',
                    x=x,y=y,width=period,height=height)
    element(pattern,'rect',width=bar,height=height,fill='#193d64',opacity=opacity)
    fade=linear(d,name+'-fade',[(0,'white',0),(.015,'white',1),(.985,'white',1),(1,'white',0)],
                x1=x,x2=x+width,y2=0)
    mask=element(d.defs,'mask',id=name+'-envelope',maskUnits='userSpaceOnUse',
                 x=x,y=y,width=width,height=height)
    element(mask,'rect',x=x,y=y,width=width,height=height,fill=fade)
    element(group,'rect',x=x,y=y,width=width,height=height,fill=f'url(#{name})',
            mask=f'url(#{name}-envelope)')


def centre_frame(d,name):
    if name!='ui\\global_bitmaps\\framing_center':return False
    d.family='central-circuit-frame'
    exterior='M0 0 H2048 V1303 H0 Z '+WINDOW+' '+TOP+' '+BOTTOM
    element(d.root,'path',id='outer-field',d=exterior,fill='#09152a',fill_opacity=109/255,fill_rule='evenodd')
    for key,path,stops in (
        ('upper',TOP,[(0,'#054f96',1),(1,'#2f7cb8',1)]),
        ('lower',BOTTOM,[(0,'#2166a6',1),(1,'#024d94',1)])):
        group=element(d.root,'g',id=key+'-band',clip_path=clip(d,key+'-band-bounds',path),opacity=191/255)
        paint=linear(d,key+'-blue',stops,x1=0 if key=='upper' else 1010,
                     x2=1132 if key=='upper' else 2048,y2=0)
        element(group,'path',d=path,fill=paint)
        if key=='upper':
            for facet in (
                'M294 122 L326 88 H632 L652 108 L640 122 Z',
                'M640 122 L688 70 H900 L884 88 H696 L664 122 Z',
                'M856 122 L900 70 H976 L1020 122 Z'):
                element(group,'path',d=facet,fill='#174878',opacity=.13)
            tick_material(d,group,'upper-short-engraving',394,112,714,6)
            tick_material(d,group,'upper-tall-engraving',782,95,326,22,24,14,.08)
            lines=TOP_CIRCUITS
            edge='M0 70 H1055 L1080 95 H1132'
        else:
            element(group,'path',d='M694 1093 H730 L744 1112 H708 Z',fill='#4293c0',opacity=.24)
            # These are three overlapping metal facets, not one zigzag.
            # Their native diagonal boundaries and stable interior palette
            # anchors resolve the source's short horizontal shelves.
            faces=(
                ('lower-wide-face','M1152 1060 H1412 L1346 1126 H1086 Z',
                 ((1150,1090),(1300,1110))),
                ('lower-right-face','M1199 1060 H1619 L1563.25 1115.75 H1427.75 L1405 1093 H1232 Z',
                 ((1440,1100),(1560,1100))),
                ('lower-overlap-face','M1199 1060 H1412 L1379 1093 H1232 Z',
                 ((1240,1090),(1360,1090))))
            for facekey,facepath,anchors in faces:
                (x0,y0),(x1,y1)=anchors
                colours=[d.source[y0,x0,:3],d.source[y1,x1,:3]]
                colour=d.linear(facekey,[(0,colours[0]),(1,colours[1])],
                                x1=x0,x2=x1,y1=0,y2=0,calibrate=False)
                element(group,'path',d=facepath,fill=colour)
            tick_material(d,group,'lower-short-engraving',710,1094,292,11)
            tick_material(d,group,'lower-tall-engraving',981,1072,137,33)
            lines=BOTTOM_CIRCUITS
            edge='M548 1093 L566 1112 H1228 L1250 1134 H2048'
        for i,pathline in enumerate(lines):
            # The original's rails have different faintness through the lit
            # band. Four small paint envelopes retain that local contrast.
            anchors=((104.5,.20),(117.5,.235)) if key=='upper' and i==0 else \
                    ((1095.7,.10),(1106.2,.163)) if key=='lower' and i==0 else None
            if anchors:
                (y0,a0),(y1,a1)=anchors
                ink=linear(d,f'{key}-circuit-{i}',[(0,'#b0e2fa',a0),(1,'#b0e2fa',a1)],y1=y0,y2=y1)
                opacity=1
            else:ink='#b0e2fa';opacity=.24 if key=='upper' else .185
            element(group,'path',d=pathline,fill='none',stroke=ink,
                    stroke_width=1.4,stroke_opacity=opacity)
        element(group,'path',d=edge,fill='none',stroke='#6cafe9' if key=='upper' else '#4087cb',
                stroke_width=2,stroke_opacity=.8)
    outside=clip(d,'outside-central-paint','M0 0 H2048 V1303 H0 Z '+WINDOW+' '+TOP+' '+BOTTOM)
    glow=element(d.root,'g',clip_path=outside)
    softness=d.glow('central-edge-light',13)
    element(glow,'path',d=WINDOW,fill='none',stroke='#2e567c',stroke_width=20,
            stroke_opacity=.52,filter=softness)
    bandlight=element(d.root,'g',clip_path=clip(d,'central-band-light',TOP+' '+BOTTOM))
    paint=linear(d,'central-band-reflection',[(0,'#2e84c4',1),(.54,'#4a98cf',1),(1,'#2e84c4',1)],x2=2048,y2=0)
    element(bandlight,'path',d=WINDOW,fill='none',stroke=paint,stroke_width=20,
            stroke_opacity=.6,filter=softness)
    face=element(d.root,'g',id='central-window',opacity=242/255)
    element(face,'path',d=WINDOW,fill='#09152a')
    element(face,'path',d='M0 122 H1107 L1144 84 H2048 M977 1092.67 L1010 1060 H2048',
            fill='none',stroke='#3e9ffa',stroke_width=2)
    element(face,'path',d='M0 1092.67 H977',fill='none',stroke='#3e9ffa',stroke_width=2.67)
    return True
