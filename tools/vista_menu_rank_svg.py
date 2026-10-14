"""Authored Vista numbered-rank plates and condensed numeral outlines.

The colored corner plates and four optional cardinal gaps are named design
parts. A few paint/state samples select their source-authored variants; no
source contour or pixel-color regions are converted into paths.
"""
import math
import numpy as np
from vista_prompt_svg import element,rgb
from vista_menu_celestial_svg import celestial


# Separate letterforms measured in the original eleven-unit numeral height.
NUMERALS={
 '0':'M2.75 0 C4.8 0 5.5 1.4 5.5 3 V8 C5.5 10 4.8 11 2.75 11 C.7 11 0 10 0 8 V3 C0 1.4 .7 0 2.75 0 Z M2.75 2 C1.8 2 1.8 3 1.8 4 V7 C1.8 8.5 1.8 9 2.75 9 C3.7 9 3.7 8.5 3.7 7 V4 C3.7 3 3.7 2 2.75 2 Z',
 '1':'M2.5 0 H5 V11 H2.5 V3 L0 4 V2 Z',
 '2':'M0 3 C0 1 1 0 2.8 0 C4.6 0 5.6 1 5.6 2.7 C5.6 4.9 3.6 6.5 2 8 V9 H5.6 V11 H0 V7.8 L2.8 5 C3.7 4 3.8 3.1 3.6 2.6 C3.4 1.7 2 1.5 2 3.3 H0 Z',
 '3':'M0 3 C0 1 1 0 2.8 0 C4.7 0 5.6 1.1 5.6 2.9 C5.6 4.2 5.1 5 4.3 5.4 C5.2 5.9 5.7 6.7 5.7 8.1 C5.7 10 4.7 11 2.8 11 C1 11 0 10.1 0 8 V7.5 H2 V8.1 C2 9.4 3.7 9.4 3.7 8 C3.7 6.8 3.2 6.4 1.5 6.4 V4.5 C3.2 4.5 3.6 4.2 3.6 3 C3.6 1.6 2 1.6 2 3 Z',
 '4':'M3 0 H5 V7 H6 V9 H5 V11 H3 V9 H0 V6.5 Z M3 3 L1.5 7 H3 Z',
 '5':'M0 0 H5.5 V2 H2 V4.5 H4 Q5.5 4.5 5.5 6 V9.5 Q5.5 11 4 11 H1 Q0 11 0 9.5 V7.5 H2 V9 H3.5 V6.5 H0 Z',
 '6':'M1 0 H4 Q5.5 0 5.5 1.5 V3 H3.5 V2 H2 V4.5 H4 Q5.5 4.5 5.5 6 V9.5 Q5.5 11 4 11 H1 Q0 11 0 9.5 V1.5 Q0 0 1 0 Z M2 6.5 V9 H3.5 V6.5 Z',
 '7':'M0 0 H5.5 V2.5 L2 11 H0 L3.5 2 H0 Z',
 '8':'M2.75 0 C4.6 0 5.5 1 5.5 2.9 C5.5 4.2 5.1 5 4.3 5.5 C5.2 6 5.6 6.8 5.6 8.1 C5.6 10 4.6 11 2.75 11 C.9 11 -.1 10 -.1 8.1 C-.1 6.8 .3 6 1.2 5.5 C.4 5 0 4.2 0 2.9 C0 1 .9 0 2.75 0 Z M2.75 2 C1.6 2 1.6 4.5 2.75 4.5 C3.9 4.5 3.9 2 2.75 2 Z M2.75 6.4 C1.5 6.4 1.5 9 2.75 9 C4 9 4 6.4 2.75 6.4 Z',
 '9':'M1 0 H4 Q5.5 0 5.5 1.5 V9.5 Q5.5 11 4 11 H0 V9 H3.5 V6.5 H1 Q0 6.5 0 5 V1.5 Q0 0 1 0 Z M2 2 V4.5 H3.5 V2 Z',
}


def sample(d,x,y):
    # Coordinates describe a handful of named material/state anchors in the
    # 28-by-26 design, shared by the separately padded 17-square small bank.
    px=min(d.width-1,max(0,round((x+.5)*d.width/28-.5)))
    py=min(d.height-1,max(0,round((y+.5)*d.height/26-.5)))
    return d.source[py,px]


def numbered_rank(d):
    if d.frame>=43:return celestial(d)
    d.family='numbered-rank-badge'
    unit=min(d.width/28,d.height/26)
    ox=(d.width-28*unit)/2;oy=(d.height-26*unit)/2
    group=element(d.root,'g',id='rank-badge',transform=f'translate({ox} {oy}) scale({unit})')
    plate=element(group,'g',id='rank-corner-plates-and-rim')
    # These source variants have intentional gaps through both the plate
    # and rim; the inner purple field remains visible behind their ends.
    mask=element(d.defs,'mask',id='rank-cardinal-gaps',maskUnits='userSpaceOnUse',x=0,y=0,width=28,height=26)
    element(mask,'rect',width=28,height=26,fill='white')
    for anchor,box in (((14,1),(12,0,4,3)),((25,13),(24,12,4,3)),
                       ((14,24),(12,23,4,3)),((2,13),(0,12,4,3))):
        if sample(d,*anchor)[3]<64:
            x,y,w,h=box;element(mask,'rect',x=x,y=y,width=w,height=h,fill='black')
    plate.set('mask','url(#rank-cardinal-gaps)')
    corners=(('nw',2,1,12,12,(3,3),(3,2),(3,6)),
             ('ne',14,1,12,12,(24,3),(24,2),(24,6)),
             ('se',14,13,12,12,(24,22),(25,20),(24,23)),
             ('sw',2,13,12,12,(3,22),(2,20),(3,23)))
    for name,x,y,w,h,present,top,bottom in corners:
        if sample(d,*present)[3]<128:continue
        a=sample(d,*top);b=sample(d,*bottom)
        paint=d.linear('rank-plate-'+name,[(0,a[:3]),(1,b[:3])],y1=top[1],y2=bottom[1],calibrate=False)
        element(plate,'rect',x=x,y=y,width=w,height=h,fill=paint)
        edge={'nw':'M2 13 V1 H14','ne':'M14 1 H26 V13',
              'se':'M26 13 V25 H14','sw':'M14 25 H2 V13'}[name]
        element(plate,'path',d=edge,fill='none',stroke='white',stroke_width=.6,stroke_opacity=.45)
    # Four editable quarter arcs retain the material changes around the rim.
    anchors=((14,1),(25,13),(14,24),(2,13))
    near_gaps=((17,2),(25,16),(11,24),(2,10))
    fallback=((214,224,254),(88,127,252),(88,127,252),(88,127,252))
    paints=[sample(d,*p) for p in anchors]
    paints=[a if a[3]>=64 else sample(d,*near_gaps[i]) for i,a in enumerate(paints)]
    colors=[a[:3] if a[3]>=64 else np.array(fallback[i]) for i,a in enumerate(paints)]
    circle=element(d.defs,'radialGradient',id='rank-field',gradientUnits='userSpaceOnUse',cx=14,cy=13,r=11)
    for offset,color in ((0,(51,41,98)),(.65,(45,35,88)),(1,(27,21,59))):
        element(circle,'stop',offset=offset,stop_color=rgb(color))
    element(group,'circle',cx=14,cy=13,r=10.4,fill='url(#rank-field)')
    positions=((14,1.8),(25.2,13),(14,24.2),(2.8,13))
    for i in range(4):
        a=positions[i];b=positions[(i+1)%4]
        gradient=d.linear('rank-rim-'+str(i),[(0,colors[i]),(1,colors[(i+1)%4])],
                          x1=a[0],y1=a[1],x2=b[0],y2=b[1],calibrate=False)
        path=f'M{a[0]} {a[1]} A11.2 11.2 0 0 1 {b[0]} {b[1]}'
        cut=d.linear('rank-rim-cut-'+str(i),[(0,colors[i]*.7),(1,colors[(i+1)%4]*.7)],
                     x1=a[0],y1=a[1],x2=b[0],y2=b[1],calibrate=False)
        glint=d.linear('rank-rim-glint-'+str(i),[(0,colors[i]*.8+50),(1,colors[(i+1)%4]*.8+50)],
                       x1=a[0],y1=a[1],x2=b[0],y2=b[1],calibrate=False)
        element(plate,'path',d=path,fill='none',stroke=cut,stroke_width=1.9,
                filter=d.glow('rank-rim-cut-softness-'+str(i),.25))
        element(plate,'path',d=path,fill='none',stroke=gradient,stroke_width=1.3,
                filter=d.glow('rank-rim-softness-'+str(i),.25))
        element(plate,'path',d=path,fill='none',stroke=glint,stroke_width=.5,
                transform='translate(-.15 -.2)',filter=d.glow('rank-rim-polish-'+str(i),.2))
    # Raised grey numerals, each retaining its own counters and flag shape.
    text=str(d.frame+1);width=5.5 if len(text)==1 else 12.5
    lettering=element(group,'g',id='rank-number',transform=f'translate({14-width/2} 7)')
    for i,char in enumerate(text):
        path=NUMERALS[char]
        glyph=element(lettering,'g',transform=f'translate({i*7} 0)')
        element(glyph,'path',d=path,fill='#09051c',fill_rule='evenodd',transform='translate(.8 .8)',
                stroke='#09051c',stroke_width=.35)
        element(glyph,'path',d=path,fill='#adadad',fill_rule='evenodd')
        edge=element(d.defs,'clipPath',id='rank-numeral-'+str(i));element(edge,'path',d=path,fill_rule='evenodd')
        element(glyph,'path',d=path,fill='none',stroke='#c7c7c7',stroke_width=.45,stroke_opacity=.6,
                transform='translate(-.25 -.25)',clip_path='url(#rank-numeral-'+str(i)+')')
    return True
