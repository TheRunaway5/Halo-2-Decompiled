"""Vista lobby row branding, drawn as paths and alpha engraving.

The two native row sizes have independent placement and repeat spacing. All
letter outlines below were measured from the row artwork, including the wide
space before 'net'; no system font or raster contour conversion is used. The
Seventh Column is drawn here only as row branding, with the larger original
Vista symbol resolving its rounded corners. Paused emblem bank drafts are not
read, rewritten, or admitted by this module.
"""
import math

import numpy as np

from vista_prompt_svg import element, rgb


LOBBY='ui\\screens\\game_shell\\pregame_lobby\\'

# Design space: ascending stems at y=2, x-height 8..28, g descender at 36.
# Counters are separate subpaths with even-odd winding.
LOWER={
 'b': 'M.5 2 H4.5 V10 C6.3 8.6 8.5 8 11.1 8 '
      'C17 8 20 12.1 20 18 C20 24.1 16.8 28 11.1 28 H.5 Z '
      'M4.5 18 V24.6 H10.3 C14 24.6 16 22.1 16 18 '
      'C16 13.9 14 11.4 10.3 11.4 C6.5 11.4 4.5 13.9 4.5 18 Z',
 'u': 'M1 8 H5 V20.8 C5 23.4 6.7 24.6 9.6 24.6 H15.7 V8 H19.7 V28 '
      'H9.4 C3.9 28 1 25.5 1 20.9 Z',
 'n': 'M0 8 H10.1 C16.6 8 19.6 10.6 19.6 16 V28 H15.6 V16.1 '
      'C15.6 12.8 13.8 11.5 10.2 11.5 H4 V28 H0 Z',
 'g': 'M16 8 H20 V26.8 C20 32.7 16.7 36 10.2 36 '
      'C5.3 36 2 33.9 1.1 30.2 H5.2 C6.1 32 7.8 32.7 10.2 32.7 '
      'C14.2 32.7 16 31 16 27 V25.7 C14.2 27.2 12.2 28 9.8 28 '
      'C3.9 28 0 24.1 0 18 C0 11.9 3.9 8 9.8 8 C12.4 8 14.5 8.8 16 10.3 Z '
      'M4 18 C4 22.2 6.3 24.6 10 24.6 C13.9 24.6 16 22.2 16 18 '
      'C16 13.7 13.9 11.4 10 11.4 C6.3 11.4 4 13.7 4 18 Z',
 'i': 'M0 2 H4 V6 H0 Z M0 8 H4 V28 H0 Z',
 'e': 'M19.8 19 H4.1 C4.5 22.6 6.6 24.8 10.2 24.8 '
      'C12.8 24.8 14.6 23.7 15.5 22 H19.5 C18.1 26.1 15 28.2 10.1 28.2 '
      'C3.8 28.2 0 24.3 0 18.1 C0 11.9 3.8 8 10 8 '
      'C16.2 8 19.8 12 19.8 18.1 Z M4.3 15.8 H15.6 '
      'C14.9 12.7 13 11.3 10 11.3 C7 11.3 5 12.9 4.3 15.8 Z',
 '.': 'M0 22 H5.5 V28 H0 Z',
 't': 'M2 2 H6 V8 H12 V11.5 H6 V21.7 C6 24.3 7 24.8 9.3 24.8 '
      'H12 V28 H8.7 C4.2 28 2 26.1 2 22 V11.5 H0 V8 H2 Z',
 's': 'M17.7 13.6 H13.8 C13.3 11.8 11.8 11.1 9.5 11.1 '
      'C6 11.1 4.1 12.1 4.1 13.9 C4.1 15.3 5.4 15.8 9.6 16.4 '
      'C15.9 17.1 18.2 18.5 18.2 22 C18.2 25.9 14.8 28.2 9.1 28.2 '
      'C3.8 28.2 .6 25.9 0 22.1 H4.1 C4.5 24.1 6.3 25 9.3 25 '
      'C12.5 25 14.3 24.1 14.3 22.3 C14.3 20.7 13 20.2 8.7 19.6 '
      'C2.8 18.9 .2 17.4 .2 14 C.2 10.2 3.7 8 9.5 8 '
      'C14.5 8 17.3 9.9 17.7 13.6 Z',
 'a': 'M16 8 H20 V28 H16 V25.7 C14.3 27.3 12.3 28 9.7 28 '
      'C3.8 28 0 24.1 0 18 C0 11.9 3.8 8 9.7 8 C12.2 8 14.4 8.8 16 10.3 Z '
      'M4 18 C4 22.2 6.2 24.6 10 24.6 C13.9 24.6 16 22.2 16 18 '
      'C16 13.8 13.9 11.4 10 11.4 C6.2 11.4 4 13.8 4 18 Z',
 'f': 'M2 28 V11.5 H0 V8 H2 V7.1 C2 3.6 4.1 2 8.3 2 H12 V5.4 H8.8 '
      'C6.8 5.4 6 6 6 7.7 V8 H12 V11.5 H6 V28 Z',
}
WORD=(('b',0),('u',20),('n',42),('g',62),('i',84),('e',90),
      ('.',112),('n',134),('e',154),('t',174))
STAFF=(('s',196),('t',216),('a',226),('f',248),('f',260))

UPPER={
 'B': 'M0 0 H9.1 C13.3 0 15.6 1.8 15.6 5.2 C15.6 7.3 14.4 8.7 12.7 9.3 '
      'C15 10 16.1 11.6 16.1 14.3 C16.1 18 13.7 20 9.3 20 H0 Z '
      'M2 2 V8.5 H8.6 C12.1 8.5 13.5 7.3 13.5 5.2 C13.5 3 12 2 8.8 2 Z '
      'M2 10.5 V18 H9 C12.4 18 14 16.8 14 14.3 C14 11.8 12.4 10.5 9 10.5 Z',
 'U': 'M0 0 H2 V12.1 C2 16.4 4.2 18.2 8.1 18.2 C12 18.2 14.2 16.4 14.2 12.1 '
      'V0 H16.2 V12.2 C16.2 17.6 13.2 20.3 8.1 20.3 C3 20.3 0 17.6 0 12.2 Z',
 'N': 'M0 20 V0 H2.7 L16 16.8 V0 H18 V20 H15.4 L2 3.2 V20 Z',
 'G': 'M19.6 5.7 H17.2 C16.1 3.1 13.6 1.7 10.3 1.7 C5.1 1.7 2.1 4.7 2.1 10 '
      'C2.1 15.2 5.2 18.2 10.4 18.2 C13.7 18.2 16 16.9 17.2 14.8 V11.5 H10.7 '
      'V9.5 H19.4 V15.5 C17.6 18.5 14.6 20.3 10.3 20.3 C3.8 20.3 0 16.4 0 10 '
      'C0 3.6 3.8 -.3 10.3 -.3 C14.8 -.3 18.3 1.7 19.6 5.7 Z',
 'I': 'M0 0 H2.2 V20 H0 Z',
 'E': 'M0 0 H15 V2 H2 V8.7 H14 V10.7 H2 V18 H15.2 V20 H0 Z',
}


def lettering(parent,x,y=0,scale=1,staff=False):
    group=element(parent,'g',id='bungie-net-staff' if staff else 'bungie-net',
                  transform=f'translate({x} {y}) scale({scale})')
    for glyph,offset in WORD+(STAFF if staff else ()):
        element(group,'path',d=LOWER[glyph],fill_rule='evenodd',transform=f'translate({offset} 0)')


def wordmark(parent,x=182,y=0,scale=1):
    group=element(parent,'g',id='bungie-wordmark',transform=f'translate({x} {y}) scale({scale})')
    for glyph,offset in (('B',0),('U',18),('N',36),('G',56),('I',79),('E',85)):
        element(group,'path',d=UPPER[glyph],fill_rule='evenodd',transform=f'translate({offset} 10)')
    element(group,'path',id='wordmark-arc',d='M13 37 C16 19 31 4 49 2 '
        'C66 0 79 4 86 12 L86 18 C78 8 64 3 50 5 C32 8 19 23 13 37 Z')


def star(parent,cx,cy,radius):
    # The five-point source silhouette has a 0.382 inner/outer radius ratio.
    points=[]
    for i in range(10):
        angle=math.radians(-90+i*36);r=radius if i%2==0 else radius*.382
        points.append(f'{cx+r*math.cos(angle):.6f} {cy+r*math.sin(angle):.6f}')
    element(parent,'path',d='M'+' L'.join(points)+' Z')


def column(parent,cx,scale=1):
    group=element(parent,'g',transform=f'translate({cx-18*scale} {1 if scale==1 else .5}) scale({scale*36/256})')
    element(group,'path',d='M108 11 H148 Q155 11 155 18 V145 Q155 153 148 153 '
        'H108 Q101 153 101 145 V18 Q101 11 108 11 Z')
    element(group,'path',d='M61 47 L85 67 C89 71 89 75 89 81 V103 L81 139 '
        'C80 144 81 148 84 152 L102 176 C105 180 108 182 112 182 '
        'H144 C148 182 151 180 154 176 L172 152 C175 148 176 144 175 139 '
        'L167 103 V81 C167 75 167 71 171 67 L195 47 Q200 43 204 49 '
        'L228 80 Q232 85 227 89 L193 115 L238 126 Q243 128 241 134 '
        'L232 177 Q230 182 224 180 L183 170 Q179 168 181 174 L197 209 '
        'Q200 215 195 218 L159 241 Q154 244 151 238 L131 202 Q128 197 125 202 '
        'L105 238 Q102 244 97 241 L61 218 Q56 215 59 209 L75 174 Q77 168 73 170 '
        'L32 180 Q26 182 24 177 L15 134 Q13 128 18 126 L63 115 L29 89 '
        'Q24 85 28 80 L52 49 Q56 43 61 47 Z')


def branded_row(d,name):
    if name not in (LOBBY+'item_background',LOBBY+'item_background_lobby'):return False
    if name.endswith('item_background') and d.frame==0:return False  # Existing uniform master.
    d.family='lobby-branded-row'
    small=name.endswith('_lobby')
    values,counts=np.unique(d.source[:,:,3],return_counts=True)
    background=float(values[np.argmax(counts)])
    dark=d.source[:,:,3][d.source[:,:,3]<background-20]
    ink=float(np.percentile(dark,5)) if len(dark) else background
    mask=element(d.defs,'mask',id='row-engraving',maskUnits='userSpaceOnUse',
                 x=0,y=0,width=d.width,height=d.height)
    element(mask,'rect',width=d.width,height=d.height,fill='white')
    shapes=element(mask,'g',fill=rgb([ink/background*255]*3))
    if d.frame in (1,2):
        x=(279 if d.frame==1 else 225) if small else (110 if d.frame==1 else 26)
        lettering(shapes,x,-.5 if small else 0,.79 if small else 1,d.frame==2)
    elif d.frame==3:
        d.art_references=[dict(name='Original Vista Seventh Column, 256-square reference',
            page='d854f5f8c7dc076777aa628b5286cef8edfcc27304494cce6eca5fb4fccdcafa',
            png_sha256='c807ba3758826b967e61524ae81da78b579e7bbcb79acd4cea8e915dca89c45a',
            purpose='Rounded symbol corners for row branding only; customization emblem drafts remain paused')]
        for i in range(14 if small else 9):column(shapes,(30+31*i) if small else (20+37*i),.8 if small else 1)
    elif d.frame==4:
        for i in range(13 if small else 8):star(shapes,(29.25+33.5*i) if small else (27+40*i),17.1 if small else 20,14.1 if small else 18)
    elif d.frame in (5,6):
        wordmark(shapes,294 if small else 182,-.6 if small else 0,.8 if small else 1)
        if d.frame==6:
            for cx in ((29,62.5,96,129.5,163,196.5,230,263.5,402.5,436) if small else (37,77,117,157,307)):
                star(shapes,cx,17.1 if small else 20,14.1 if small else 18)
    element(d.root,'rect',id='row-material',width=d.width,height=d.height,fill='white',
            fill_opacity=background/255,mask='url(#row-engraving)')
    return True
