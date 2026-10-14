"""Editable celestial ranks, measured on the original 28 x 26 Vista pages.

The lunar basins, highland ridges, phase boundaries, flare silhouettes, comet
streams and ring faces are individually drawn curves. No raster embedding,
autotrace, pixel cells, generated noise, or invented small craters are used.
These are reconstructions of the visible reference features, not recovered
higher-resolution original artwork.
"""
import math
from vista_prompt_svg import element
from vista_ring_habitat_svg import HABITAT_REFERENCE, habitat


# Identity-only provenance for the separately extracted, larger MCC references.
# Replacement rendering does not load or embed their pixels.
MCC_RANK_PAYLOADS = {
    44: '8c6ca2d9a305876104abaafe55029216e92d5946166e7a342e80017d3113294d',
    45: '6edd2a79d91f8638308606eb47773342a5290634ef1066dc4a0049e0a6183733',
    46: '446b8bafaf794bd516b535feeeff91587e5aac796b3a12ddf63e0d5a786e76b4',
    47: 'c7186cae779a5216012a70af9d22e789f261be8b921a4c482e2e26b4cf839e73',
    48: 'a13f517af74ffae5a926e5d5f3a0520447c41a2da4175d9b399773a907bc5511',
    49: '76c56f7a120eef86927a74c8cc5278afde60d261cea606d9a63ea5c2f80bc2ec',
    50: '4456a0984d72e26f46e625f7732f607d1451c846195b26f682836c7abb1b9f1e',
}
MCC_RANK_PACK = dict(pack='mainmenuandcampaigntexturepack',dimensions=[152,152],
    perm_sha256='8fca3bcf6a8bd799573b0bd62de5eef5509036a9b1e9c37289493e735ed3f690',
    temp_sha256='3f46d95fe6b81828bb724b0e9f9a4de9d628c907809a01c4eca036aee504fb03')


# Broad dark regions and the light corridors between them, plotted against
# rank_icons page 45. Coordinates refer to the full native canvas, including
# its transparent border. The three phases share this surface drawing.
BASINS = (
    ('M7.6 7.1 C9 6.2 9.6 7.4 10.5 7.8 L11.5 7.2 '
     'C12.8 7.1 14.4 7.7 14.1 9.2 L13.3 10.2 L12.8 11.8 '
     'L11 12.4 L9.2 11.8 L9.7 10.5 L8.4 10.1 Z', '#565d67', .85),
    ('M16.2 6.5 L18.5 6.6 L19.8 7.6 L19.2 8.9 L20.2 9.6 '
     'L19.5 11 L17.9 11.5 L16.8 10.4 L16.7 8.6 L15.9 8 Z', '#626973', .9),
    ('M20 9.2 C22.1 9.4 21.7 11.3 21.7 12.2 L22.4 13.7 '
     'L21.5 15 L21.9 16.4 L20.6 17.7 L18.9 17.5 L18.7 15.9 '
     'L17.6 14.9 L18.4 13.7 L17.8 12.3 Z', '#555b65', .88),
    ('M6.2 10.3 L8.2 10.5 L8.7 11.8 L8 13.1 L8.7 14.3 '
     'L8.3 16.9 L7 17.8 L5.8 16.8 L6.1 15.3 L4.8 13.8 L5.1 12 Z', '#5d6674', .85),
    ('M10.5 13.4 L12 14 L13 13.6 L14.1 14.6 L14.1 15.8 '
     'L15.1 16.9 L14.5 18.5 L12.7 19.6 L11.5 18.8 L10 19.4 '
     'L8.5 18.2 L9.2 16.7 L8.9 15.2 Z', '#4f5764', .92),
    ('M16.4 15 L17.9 16 L17.6 17.7 L18.5 18.5 L18 19.6 '
     'L16.3 20.3 L14.6 19.7 L15.4 18.3 L15.2 16.6 Z', '#59616c', .9),
    ('M9.2 19 L10.6 19.4 L11.1 20.5 L10.4 21.4 L8.7 20.9 Z', '#69717e', .8),
    ('M12 5.2 L13.3 4.4 L14.7 5.2 L14.4 6.3 L13.1 6.4 Z', '#7e8490', .7),
    ('M7.6 4.4 L8.6 3.8 L10.2 4.3 L10.5 5.2 L8.9 5.9 Z', '#9298a3', .8),
    ('M19.5 4.9 L20.3 5 L21.3 6.8 L20.5 7.7 L19.2 6.8 Z', '#8a919c', .65),
    ('M5 17.2 L6.6 17.9 L7.5 19.6 L6.8 20.4 L5.3 19.4 Z', '#737e8d', .7),
    ('M19.2 19.1 L20.9 18.5 L21.3 20 L20.2 21.3 L18.8 21.5 L18.3 20.4 Z', '#7d8692', .75),
)

# Irregular, connected highland contours visible in the reference. These are
# not circles stamped onto a generic moon: their positions follow its light
# ridges and small bright groups. Soft strokes preserve the source's material.
RIDGES = (
    ('M7.5 4.9 L8.5 5.2 L9.1 4.9 L10 5.9 L11.3 6.2 L11.7 7.3', .7, .48),
    ('M5.1 6.8 L6.3 6.5 L6.9 7.4 L6.1 8.4 L7 9.1 L8.3 8.9', .6, .55),
    ('M4 10.2 L5 9.8 L5.5 10.9 L6.7 11.1 L7.1 10.7', .55, .38),
    ('M3.5 13.3 L4.5 12.5 L5.2 13.6 L4.5 14.6 L5.1 15.3', .6, .45),
    ('M5 16.4 L5.1 17.5 L6.4 17.1 L7.1 18.3 L7.4 19.4', .55, .5),
    ('M7.1 20 L8.5 19.4 L9.3 20.3 L9.1 21.3 L10.4 21.5', .7, .55),
    ('M10.9 22.1 L12 21.4 L13.5 22 L14.4 21 L15 21.8 L16.8 21.8', .6, .5),
    ('M14 23.3 L15.4 22.9 L16.2 23.4 L17.2 22.8 L18.3 22.9', .65, .5),
    ('M18.5 21.5 L19.4 22 L20 20.9 L21.4 20.4 L21.8 19.2', .6, .55),
    ('M23.1 18.4 L22.4 17.6 L23.5 16.7 L22.9 15.5 L23.7 14.1', .6, .45),
    ('M24 12.5 L23.1 12.1 L23.4 10.7 L22.5 10.2 L22.8 8.8', .7, .6),
    ('M21.9 7.2 L20.8 7.4 L20.9 8.8 L19.6 9.2', .6, .45),
    ('M18.8 3.3 L18 4.1 L18.9 5 L18.3 5.8 L17.2 5.4 L16.2 6.3', .7, .55),
    ('M13.8 3.1 L14.5 4.2 L15.5 4 L15.7 5.2 L16.2 5.7', .55, .5),
    ('M10.6 2.8 L11.7 3.7 L12.4 3.2 L13 4.1 L12.4 4.7', .6, .5),
    ('M9.3 6.7 L10.3 7.2 L10.1 8.3 L11 8.8', .5, .32),
    ('M14.6 7.6 L15.4 8.5 L15.1 10.1 L14.2 11.2 L14.8 12.3', .6, .42),
    ('M10.4 11.9 L11.1 12.8 L12.6 12 L13.2 12.7 L12.8 13.6', .65, .58),
    ('M14.9 12.7 L16 13.2 L15.6 14.3 L16.5 15.3', .6, .45),
    ('M8.3 13.7 L9.3 14.1 L9.6 15 L10.4 14.6 L11 15.3', .45, .33),
    ('M11 16.1 L12.1 16.8 L11.8 17.8 L12.8 18.2', .45, .3),
    ('M17.1 11.2 L16.6 12.1 L17.4 13.4 L17 14.6', .55, .38),
    ('M18.8 17.4 L19.6 17.9 L19 18.7 L19.7 19.5', .55, .4),
    ('M10.7 19.4 L11.5 20.1 L12.6 19.7 L13.4 20.6', .5, .4),
)


def lunar_surface(d, parent):
    clip=element(d.defs,'clipPath',id='lunar-disc')
    element(clip,'circle',cx=14,cy=13,r=11.65)
    surface=element(parent,'g',id='lunar-surface',clip_path='url(#lunar-disc)')
    base=element(d.defs,'radialGradient',id='lunar-body',gradientUnits='userSpaceOnUse',cx=14,cy=12,r=12)
    for offset,colour in ((0,'#6d7480'),(.7,'#89929e'),(.87,'#a8b0bc'),(1,'#aeb6c1')):
        element(base,'stop',offset=offset,stop_color=colour)
    element(surface,'circle',cx=14,cy=13,r=11.65,fill='url(#lunar-body)')
    element(surface,'path',d='M4.8 8 C6 4 10 1.8 14 1.6 C19 1.7 22 4.3 24 7.6 '
            'M25 10.3 C26.2 14 24.1 19.3 21 21.9 M17 24.1 C12 25.5 7.6 23.1 5.2 20.1',
            fill='none',stroke='#d5d8dc',stroke_width=1.3,opacity=.8,
            filter=d.glow('lunar-limb',.5))
    element(surface,'path',d='M5 5 C9 0 19 0 23 5 L21 7 L18.5 6.5 L17 7.3 '
            'L15 6.5 L13.5 7 L12 6.5 L9 7 L7 6.3 Z',fill='#d1d4d8',opacity=.45,
            filter=d.glow('northern-highlands',.6))
    basin_softness=d.glow('basin-softness',.65)
    for path,colour,alpha in BASINS:
        element(surface,'path',d=path,fill=colour,opacity=alpha,filter=basin_softness)
    ridges=element(surface,'g',id='highland-ridges',fill='none',stroke='#d1d4d9',
                   stroke_linecap='round',stroke_linejoin='round',filter=d.glow('ridge-softness',.23))
    for path,width,alpha in RIDGES:
        element(ridges,'path',d=path,stroke_width=width*.62,opacity=alpha*.7)
    # The handful of distinct bright knots, rather than a synthetic crater field.
    knot_softness=d.glow('highland-knot',.25)
    for x,y,rx,ry,alpha in ((5.9,5.3,.65,.5,.6),(8.5,6.4,.5,.7,.42),
                          (11.3,4,.6,.4,.45),(17.9,3,.8,.35,.6),
                          (21,4.5,.5,.4,.65),(22.5,7,.45,.7,.65),
                          (24.8,11,.4,.7,.6),(24,14.2,.5,.8,.65),
                          (21.6,19.1,.55,.5,.45),(17.7,22.5,.7,.45,.5),
                          (11.6,22.9,.55,.55,.5),(8,21,.5,.45,.5),
                          (4,17.7,.4,.7,.45),(3.9,11.2,.45,.65,.4),
                          (12.3,12.8,.6,.4,.4),(15.7,15,.35,.55,.35)):
        element(surface,'ellipse',cx=x,cy=y,rx=rx,ry=ry,fill='#d8dbe0',opacity=alpha,
                filter=knot_softness)
    element(surface,'path',d='M13.1 11.5 L14.1 11.9 L14.4 13.3 L13.6 14.2 '
            'L12.7 13.5 L13 12.6 Z',fill='#aeb7c4',opacity=.5,
            filter=d.glow('central-highland',.35))
    lunar_craters(d,surface)
    if d.frame==44:half_moon_material(d,surface)
    return surface


# Independently outlined crater groups from the unobscured MCC 152px moon
# (LARGERANKICON_046). The middle is occluded by its numerals, so Vista remains
# the authority there. No synthetic random field fills that missing evidence.
MCC_CRATERS = (
    (7.4,6.7,.75,.48), (8.7,7.3,.3,.38), (6.4,8.3,.4,.5),
    (5.6,9.8,.45,.6), (4.5,11.5,.32,.6), (4.4,14.6,.35,.55),
    (4.9,17.4,.48,.6), (5.7,19.3,.45,.4), (6.9,20.9,.45,.36),
    (8.3,21.5,.35,.5), (9.8,22.4,.4,.36), (11,21.5,.5,.45),
    (12.4,19.5,.5,.5), (13.8,20.5,.3,.45), (14.7,21.7,.5,.35),
    (16,19.3,.6,.4), (17.1,20.6,.5,.5), (18.5,19.2,.4,.6),
    (19.4,20.8,.4,.38), (21,19.1,.45,.65), (22.1,17.7,.45,.55),
    (23.4,15.3,.4,.5), (23.3,12.8,.35,.6), (22.8,10.4,.55,.6),
    (22,8.4,.45,.65), (20.7,6.5,.48,.55), (19.1,5.1,.35,.4),
    (17.8,6.4,.55,.5), (16.5,4.8,.45,.45), (15.3,6.5,.35,.5),
    (14.1,4.6,.4,.5), (12.9,5.7,.5,.4), (11.4,5.4,.35,.5),
    (10.2,4.2,.4,.5), (9,5.4,.4,.35), (7.8,5.1,.35,.45),
)


def lunar_craters(d,parent):
    # Small grey rim, recessed floor, and a narrow sun-facing glint. Their
    # reflected paint is deliberately subdued to retain Vista's blue-grey tone.
    rim=d.linear('crater-rim',[(0,(166,175,185)),(.45,(109,119,133)),(1,(151,161,176))],
                 y1=-1,y2=1,calibrate=False)
    recess=d.linear('crater-recess',[(0,(73,81,94)),(.65,(112,123,137)),(1,(135,145,158))],
                    y1=-1,y2=1,calibrate=False)
    group=element(parent,'g',id='mcc-observed-crater-groups',opacity=.32,
                  filter=d.glow('crater-softness',.08))
    for x,y,rx,ry in MCC_CRATERS:
        g=element(group,'g',transform=f'translate({x} {y}) scale({rx} {ry})')
        path='M-.9 -.35 C-.6 -.9 .2 -1  .65 -.65 C1 -.4 1 .25 .6 .65 C.1 1 -.6 .8 -.9 .35 Z'
        element(g,'path',d=path,fill=rim)
        element(g,'ellipse',cx=.02,cy=-.06,rx=.65,ry=.6,fill=recess)
        element(g,'path',d='M-.75 .1 C-.7 .6 -.1 .8 .4 .6',fill='none',
                stroke='#c3cbd4',stroke_width=.12,opacity=.4)


def half_moon_material(d,parent):
    # An editable stipple brush, not a random crater generator. Its small
    # irregular flecks describe the fine, granular material visible in MCC.
    # The individually placed basins and crater groups supply the geography.
    pattern=element(d.defs,'pattern',id='half-moon-granulation',patternUnits='userSpaceOnUse',
                    width=3.7,height=3.1,patternTransform='rotate(19)')
    light=(
        'M.1 .2 L.32 .12 L.48 .28 L.35 .46 L.16 .4 Z',
        'M.85 .08 L1.1 .2 L1.06 .39 L.8 .36 L.72 .2 Z',
        'M1.5 .2 L1.82 .1 L2 .32 L1.8 .5 L1.59 .38 Z',
        'M2.5 .1 L2.7 .18 L2.8 .49 L2.6 .53 L2.43 .34 Z',
        'M3.16 .32 L3.38 .13 L3.58 .36 L3.46 .61 L3.21 .55 Z',
        'M.35 .83 L.62 .69 L.76 .91 L.52 1.1 L.31 1.02 Z',
        'M1.12 .78 L1.3 .69 L1.5 .93 L1.34 1.2 L1.08 1.05 Z',
        'M2.08 .79 L2.23 .58 L2.47 .8 L2.35 1.08 L2.09 1.12 Z',
        'M3.02 1.08 L3.16 .86 L3.4 .93 L3.49 1.19 L3.24 1.36 Z',
        'M.02 1.63 L.29 1.39 L.43 1.67 L.3 1.91 L.08 1.87 Z',
        'M.76 1.69 L.92 1.49 L1.18 1.64 L1.06 1.91 L.83 1.97 Z',
        'M1.62 1.51 L1.87 1.63 L1.94 1.92 L1.71 2.05 L1.5 1.85 Z',
        'M2.63 1.48 L2.83 1.72 L2.69 2.01 L2.46 1.9 L2.47 1.64 Z',
        'M3.32 1.83 L3.51 1.62 L3.69 1.89 L3.54 2.16 L3.31 2.1 Z',
        'M.32 2.35 L.6 2.19 L.74 2.47 L.55 2.68 L.3 2.57 Z',
        'M1.18 2.36 L1.4 2.19 L1.58 2.43 L1.48 2.69 L1.21 2.66 Z',
        'M2.05 2.43 L2.23 2.28 L2.49 2.45 L2.36 2.73 L2.11 2.77 Z',
        'M2.96 2.53 L3.17 2.33 L3.35 2.63 L3.14 2.84 L2.97 2.77 Z',
    )
    for path in light:element(pattern,'path',d=path,fill='#d6dbe3',opacity=.16)
    for path in ('M.4 .45 L.8 .52 L.69 .7 L.39 .64 Z',
                 'M1.32 .44 L1.6 .54 L1.47 .79 L1.23 .68 Z',
                 'M2.82 .6 L3.11 .68 L2.98 .95 L2.73 .86 Z',
                 'M.5 1.13 L.79 1.24 L.62 1.5 L.42 1.36 Z',
                 'M1.73 1.09 L1.98 1.18 L1.89 1.49 L1.62 1.35 Z',
                 'M2.54 1.12 L2.84 1.26 L2.69 1.47 L2.44 1.39 Z',
                 'M.23 2.02 L.51 2.09 L.43 2.3 L.14 2.23 Z',
                 'M1.13 2.02 L1.39 2.1 L1.3 2.33 L1.02 2.25 Z',
                 'M2.05 2.09 L2.38 2.16 L2.2 2.41 L1.98 2.33 Z',
                 'M3.32 2.19 L3.6 2.27 L3.49 2.51 L3.21 2.43 Z'):
        element(pattern,'path',d=path,fill='#3e4858',opacity=.17)
    material=element(parent,'g',id='half-moon-fine-material',filter=d.glow('half-moon-grain-softness',.06))
    element(material,'circle',cx=14,cy=13,r=11.65,fill='url(#half-moon-granulation)',opacity=.7)
    detail=element(d.defs,'pattern',id='half-moon-secondary-granulation',patternUnits='userSpaceOnUse',
                   width=3.7,height=3.1,patternTransform='rotate(-31) scale(.63 .71)')
    for node in pattern:
        element(detail,'path',**node.attrib)
    element(material,'circle',cx=14,cy=13,r=11.65,fill='url(#half-moon-secondary-granulation)',opacity=.5)


# Source flare tips, drawn asymmetrically instead of displacing a perfect ring.
CORONA = ('M13.1 1.5 L15.6 2.2 L17.3 1.9 L19.2 3.5 L22.8 2.9 '
          'L22.2 5.4 L25.5 6.2 L24.4 9.7 L26.6 12.7 L24.8 14.7 '
          'L25.7 18.9 L23.2 19.1 L24.6 23.9 L21.1 22.5 L19.7 25 '
          'L17.1 23.8 L14.9 25.6 L11.5 23.7 L7.7 25 L8.1 22.7 '
          'L3.4 24.1 L4.3 20.7 L1.8 19.7 L3 16.1 L1.4 13.1 '
          'L3.1 10.5 L2.5 6.7 L5.2 7.1 L6.6 3.4 L9.1 4 L11.2 1.8 Z')


def corona(d,group,solar):
    grad=element(d.defs,'radialGradient',id='corona',gradientUnits='userSpaceOnUse',cx=14,cy=13,r=13)
    stops=((0,'#ffd92b',1),(.68,'#d8ad23',1),(.83,'#977117',.8),(1,'#745019',0)) if solar else (
        (0,'#000000',1),(.65,'#c9bf91',1),(.76,'#bdb18a',1),(.88,'#77683b',.65),(1,'#5b4e27',0))
    for offset,colour,alpha in stops:
        element(grad,'stop',offset=offset,stop_color=colour,stop_opacity=alpha)
    element(group,'path',d=CORONA,fill='#a97a15' if solar else '#8e7d46',opacity=.65,
            filter=d.glow('flare-tips',.75))
    element(group,'path',d=CORONA,fill='url(#corona)',filter=d.glow('corona-softness',.5))
    # Narrow continuous bright rim inside the individually positioned flare tips.
    element(group,'ellipse',cx=14,cy=13,rx=9.8,ry=10.2,fill='none',
            stroke='#e4bc2e' if solar else '#c7bf9b',stroke_width=1.3,
            opacity=.8,filter=d.glow('corona-rim',.5))
    if solar:
        disc=element(d.defs,'radialGradient',id='solar-disc',gradientUnits='userSpaceOnUse',
                     cx=11.3,cy=9,r=15.5)
        for offset,colour in ((0,'#ffffc7'),(.35,'#ffffa3'),(.65,'#fff576'),(.85,'#ffdf2c'),(1,'#e8ad0d')):
            element(disc,'stop',offset=offset,stop_color=colour)
        element(group,'ellipse',cx=14,cy=13,rx=9.5,ry=10.1,fill='url(#solar-disc)',
                filter=d.glow('solar-edge',.38))
    else:
        element(group,'ellipse',cx=14,cy=13,rx=9.15,ry=9.65,fill='black',
                filter=d.glow('eclipse-edge',.2))


def comet(d,group):
    grad=element(d.defs,'linearGradient',id='comet-stream',gradientUnits='userSpaceOnUse',
                 x1=3,y1=1,x2=24,y2=23)
    for offset,colour,alpha in ((0,'#359bff',.3),(.23,'#42a2ff',.85),(.5,'#86c6ff',.98),
                               (.7,'#cfebff',1),(.88,'#f5fbff',1),(1,'#ffffff',1)):
        element(grad,'stop',offset=offset,stop_color=colour,stop_opacity=alpha)
    element(group,'path',d='M.5 .5 C8 -.7 17.5 6.9 22.2 13.8 L24.8 20.7 '
            'L23.2 23.6 C20.4 24.4 18.5 20.4 14 19.2 C8.7 18.4 4.2 16.2 .8 14.5 Z',
            fill='url(#comet-stream)',opacity=.83,filter=d.glow('comet-envelope',.95))
    # Overlapping streams join the head, rather than ending as separate petals.
    strands=(
        ('M1.5 .6 C6.2 4.2 11.6 9.4 16.6 14.5 L22.5 21.5 '
         'C20 20.6 15.6 16.6 11.2 12.1 C7.5 8.4 3.9 3.8 1.5 .6 Z',.95),
        ('M5 .3 C10 4.6 15.3 10.3 19.6 16 L23.5 21 '
         'C20.7 19.1 15.3 12.8 11.8 9.1 C8.4 5.6 6.1 1.9 5 .3 Z',.92),
        ('M10 1 C14.1 5.6 18.2 10.6 22 16.8 L23.7 21.4 '
         'C20.6 17.4 16.9 10.1 10 1 Z',.88),
        ('M1 5.5 C5.4 9.1 13.2 13.1 18.5 17.3 L22.3 21.5 '
         'C18.4 20.1 11.3 15 7.8 12.7 C4.6 10.6 2.9 7.7 1 5.5 Z',.95),
        ('M1.4 10 C7.9 13.4 12.4 16.7 19.3 19.9 L22.2 22 '
         'C17 21.1 9.8 18.4 5.9 15.3 C3.9 13.6 2.5 11.5 1.4 10 Z',.76),
    )
    stream_softness=d.glow('stream-softness',.28)
    for path,alpha in strands:
        element(group,'path',d=path,fill='url(#comet-stream)',opacity=alpha,filter=stream_softness)
    core=element(d.defs,'linearGradient',id='comet-continuous-core',gradientUnits='userSpaceOnUse',
                 x1=8,y1=5,x2=23,y2=22)
    for offset,alpha in ((0,0),(.4,.12),(.7,.7),(1,1)):
        element(core,'stop',offset=offset,stop_color='#f1faff',stop_opacity=alpha)
    element(group,'path',d='M6 3 C10 5 16 11 20 15 C21.5 17 23.5 20 22.8 22.4 L20.5 21.5 '
            'C18 17 14 13 10 9 Z',fill='url(#comet-continuous-core)',filter=d.glow('comet-core-softness',.45))
    head='M19.3 14.8 C21.1 14.7 22.5 15.7 23.8 17.5 C25 19.3 25.4 21.1 24.4 22.5 '
    head+='C23.5 24 21.8 24.2 20.4 22.6 C18.9 21.2 17.6 19.4 17.2 17.8 '
    head+='C17.1 16.5 18 15.4 19.3 14.8 Z'
    element(group,'path',d=head,fill='#5eb7ff',opacity=.65,filter=d.glow('comet-head-halo',.75))
    paint=d.linear('comet-head-paint',[(0,(242,250,255)),(.35,(252,254,255)),(1,(255,255,255))],
                   x1=18,y1=15,x2=25,y2=24,calibrate=False)
    element(group,'path',d=head,fill=paint,filter=d.glow('comet-head',.28))


# Centreline ellipse fitted to Vista page 49's alpha support (soft-L1 fit),
# not guessed from the enlarged preview. The previous ring was too long and
# narrow. The larger MCC reference confirms a broad band with an inset face.
RING_CENTRE=(14.3908,13.0357)
RING_RADII=(12.6986,7.5427)
RING_ANGLE=-55.3129
RING_PAINT=(
    (0,(17,18,19)),(30,(40,42,46)),(60,(94,102,120)),
    (90,(185,201,227)),(120,(157,187,226)),(135,(192,190,174)),
    (150,(178,195,187)),(180,(100,115,129)),(210,(22,24,25)),
    (240,(29,32,33)),(270,(99,106,108)),(300,(194,202,204)),
    (330,(147,155,156)),(360,(17,18,19)),
)


def ring_point(angle,rx,ry):
    theta=math.radians(angle)
    return rx*math.cos(theta),ry*math.sin(theta)


def ring_arc(start,end,rx,ry):
    a=ring_point(start,rx,ry);b=ring_point(end,rx,ry)
    return f'M{a[0]:.5f} {a[1]:.5f} A{rx} {ry} 0 0 1 {b[0]:.5f} {b[1]:.5f}'


def ring_wall_path(start,end,rx,ry,height):
    p0=ring_point(start,rx,ry);p1=ring_point(end,rx,ry)
    # Projected upper and lower edges of a cylindrical band, not an ellipse
    # stroke. The separation is axial thickness in camera space.
    return (f'M{p0[0]:.5f} {p0[1]-height/2:.5f} '
            f'A{rx} {ry} 0 0 1 {p1[0]:.5f} {p1[1]-height/2:.5f} '
            f'L{p1[0]:.5f} {p1[1]+height/2:.5f} '
            f'A{rx} {ry} 0 0 0 {p0[0]:.5f} {p0[1]+height/2:.5f} Z')


def ring_face_path(start,end,outer_a,outer_b,inner_a,inner_b):
    p0=ring_point(start,outer_a,outer_b);p1=ring_point(end,outer_a,outer_b)
    q0=ring_point(start,inner_a,inner_b);q1=ring_point(end,inner_a,inner_b)
    return (f'M{p0[0]:.5f} {p0[1]:.5f} '
            f'A{outer_a} {outer_b} 0 0 1 {p1[0]:.5f} {p1[1]:.5f} '
            f'L{q1[0]:.5f} {q1[1]:.5f} '
            f'A{inner_a} {inner_b} 0 0 0 {q0[0]:.5f} {q0[1]:.5f} Z')


def ring(d,group):
    cx,cy=RING_CENTRE;a,b=RING_RADII
    height=1.9;inner_a=a-.36;inner_b=b-.22
    face=element(group,'g',id='halo-ring-cross-section',
                 transform=f'translate({cx} {cy}) rotate({RING_ANGLE})',
                 filter=d.glow('ring-face-softness',.12))
    paints=list(zip(RING_PAINT,RING_PAINT[1:]))
    # Camera basis: screen minor = Y*cos(i)+Z*sin(i), depth = -Y*sin(i)+Z*cos(i).
    # An inward normal faces the camera for angles 0..180; an outward
    # normal faces it for 180..360. The end rim projects along positive minor.
    # Camera looks into the lower/right inside surface. The upper/left half
    # is the nearer exterior. The old surface ownership was reversed.
    for side,start_angle,end_angle,rx,ry in (
        ('far-interior',0,180,inner_a,inner_b),
        ('near-exterior',180,360,a,b)):
        wall=element(face,'g',id=side+'-wall')
        for i,((start,c0),(end,c1)) in enumerate(paints):
            if start<start_angle or end>end_angle:continue
            p0=ring_point(start,rx,ry);p1=ring_point(end,rx,ry)
            paint=d.linear(f'ring-{side}-{i}',[(0,c0),(1,c1)],
                           x1=p0[0],y1=p0[1],x2=p1[0],y2=p1[1],calibrate=False)
            element(wall,'path',d=ring_wall_path(start-.12,end+.12,rx,ry,height),fill=paint)
    # The original habitat texture supplies manually drawn coastline detail.
    # Its placement follows the existing Vista light/warm regions, while the
    # accepted camera, facing and cross-section remain fixed.
    inside=element(face,'g',id='vista-interior-surface-detail')
    clip=element(d.defs,'clipPath',id='visible-ring-interior')
    element(clip,'path',d=ring_wall_path(0,180,inner_a,inner_b,height))
    inside.set('clip-path','url(#visible-ring-interior)')
    habitat(d,inside,inner_a,inner_b,height)
    inward_edge=element(face,'g',id='interior-opening-edge',transform=f'translate(0 {-height/2})')
    inward_paint=d.linear('inward-structural-edge',[(0,(20,28,35)),(.5,(69,87,106)),(1,(46,62,65))],
                         x1=a*.87,y1=b*.5,x2=-a*.87,y2=b*.5,calibrate=False)
    element(inward_edge,'path',d=ring_arc(12,174,inner_a,inner_b),fill='none',
            stroke=inward_paint,stroke_width=.18,opacity=.6)
    # The top rim projects toward lower/right in this camera orientation.
    # Its modest radial thickness joins the two broad cylindrical surfaces;
    # it is not the main light face.
    top=element(face,'g',id='annular-end-rim',transform=f'translate(0 {height/2})')
    for i,((start,c0),(end,c1)) in enumerate(paints):
        p0=ring_point(start,a,b);p1=ring_point(end,a,b)
        paint=d.linear(f'ring-end-rim-{i}',
                       [(0,tuple(round(c*.34) for c in c0)),
                        (1,tuple(round(c*.34) for c in c1))],
                       x1=p0[0],y1=p0[1],x2=p1[0],y2=p1[1],calibrate=False)
        element(top,'path',d=ring_face_path(start-.12,end+.12,a,b,inner_a,inner_b),fill=paint)
    # A single subdued edge on the inside distinguishes the habitat face
    # from its narrow structural rim, without drawing parallel chrome lines.
    edge=d.linear('interior-rim-reflection',[(0,(65,79,103)),(.5,(160,175,180)),(1,(100,120,112))],
                  x1=a*.5,y1=b*.87,x2=-a*.87,y2=b*.5,calibrate=False)
    element(top,'path',d=ring_arc(65,165,inner_a,inner_b),fill='none',stroke=edge,
            stroke_width=.12,opacity=.5)


def celestial(d):
    if d.frame not in range(43,50):return False
    d.family='celestial-rank-illustration'
    reference_ranks=sorted({d.frame+1,46} if d.frame in (43,44,45) else {d.frame+1})
    d.art_references=[MCC_RANK_PACK|dict(name=f'LARGERANKICON_{rank:03}',
        payload_sha256=MCC_RANK_PAYLOADS[rank],purpose='Manually redrawn compatible detail; Vista governs final presentation')
        for rank in reference_ranks]
    unit=min(d.width/28,d.height/26)
    group=element(d.root,'g',id='celestial-rank',
                  transform=f'translate({(d.width-28*unit)/2} {(d.height-26*unit)/2}) scale({unit})')
    if d.frame in (43,44,45):
        moon=element(group,'g',filter=d.glow('lunar-outline-softness',.23))
        lunar_surface(d,moon)
        if d.frame!=45:
            shadow=element(d.defs,'mask',id='lunar-shadow',maskUnits='userSpaceOnUse',x=0,y=0,width=28,height=26)
            element(shadow,'rect',width=28,height=26,fill='white')
            if d.frame==44:
                element(shadow,'path',d='M13.7 1 C14.6 4 15.4 7.5 15.4 13 '
                        'C15.3 18.8 14.6 22 13.7 25 L0 26 V0 Z',fill='black',
                        filter=d.glow('curved-half-terminator',.3))
            else:
                element(shadow,'path',d='M9.8 2 C5.1 4.2 2.2 8.2 2.3 13.1 '
                        'C2.2 19.8 7.7 25.1 14.1 24.8 C17.1 24.8 19.4 24.1 21.3 23 '
                        'C17.1 23.9 12.5 22.4 9.1 18.8 C5.5 15 5.4 9.4 7.4 5.4 Z',fill='black')
            # Keep visible surface detail in the unlit hemisphere. It is not
            # an opaque flat circle placed over the moon.
            element(moon,'circle',cx=14,cy=13,r=11.65,fill='#1b1e20',opacity=.85,
                    mask='url(#lunar-shadow)',filter=d.glow('lunar-terminator',.25))
    elif d.frame in (46,47):corona(d,group,d.frame==47)
    elif d.frame==48:comet(d,group)
    elif d.frame==49:
        d.art_references.append(HABITAT_REFERENCE)
        ring(d,group)
    return True
