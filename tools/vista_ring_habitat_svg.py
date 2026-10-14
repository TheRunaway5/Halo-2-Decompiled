"""Manually outlined habitat features from Halo 2's original inner-ring map.

Coordinates below are observations in the 1024 x 64 texture, not an automatic
bitmap trace. The alignment places the principal coast at Vista's warm bend;
the rank render's original texture phase has not been recovered. The artwork
contains only newly drawn curves and Vista-coloured paint, with no source image
dependency or embedded texture pixels.
"""
import math

from vista_prompt_svg import element


HABITAT_REFERENCE = dict(
    cache='00a_introduction.map', resource_file='textures.dat',
    name=r'scenarios\objects\forerunner\industrial\halo\bitmaps\halo_inner_ring',
    dimensions=[1024, 64],
    payload_sha256='5d7a8bfb3abaff2c0c2de5eeac0e88926ec57d1dda18636bf3817133694094ed',
    purpose='Manually outlined habitat coasts and cloud regions; aligned to Vista lighting, not recovered rank UVs')

# The first coast is partly obscured by clouds. Its visible edge and the small
# western island are kept separate. The other two masses surround the narrow
# ocean channel visible between source V=32 and V=45.
COASTS = (
    ('western-island', (
        (97,18),(99,16),(102,16),(104,17),(104,20),(102,21),
        (104,23),(100,22),(99,20),(96,20))),
    ('northern-coast', (
        (112,3),(116,7),(122,8),(127,7),(132,11),(134,14),
        (131,17),(127,17),(124,20),(121,21),(125,23),(131,23),
        (129,27),(134,29),(139,28),(142,25),(146,26),(148,23),
        (152,23),(154,20),(157,18),(160,14),(165,14),(170,12),
        (173,9),(179,11),(184,9),(190,8),(193,5),(198,3))),
    ('channel-island', (
        (207,15),(211,12),(215,14),(219,13),(223,17),(227,18),
        (231,21),(226,23),(220,23),(217,22),(213,23),(210,20),(207,19))),
    ('channel-north-shore', (
        (220,3),(222,9),(228,10),(231,13),(234,14),(233,18),
        (238,21),(234,24),(235,29),(240,32),(248,34),(253,32),
        (258,31),(264,32),(268,34),(273,32),(277,33),(281,29),
        (284,31),(288,28),(291,23),(294,25),(299,27),(300,31),
        (304,33),(307,33),(308,36),(314,36),(318,33),(322,34),
        (325,29),(325,25),(330,24),(330,19),(334,17),(334,13),
        (330,12),(327,14),(323,12),(319,11),(317,15),(313,15),
        (311,12),(307,12),(306,10),(309,8),(315,8),(318,6),
        (322,7),(326,5),(331,6),(335,3))),
    ('channel-south-shore', (
        (244,57),(244,54),(247,50),(253,50),(257,47),(263,47),
        (265,44),(270,45),(271,42),(277,42),(281,39),(286,39),
        (289,37),(293,39),(297,36),(301,38),(304,36),(308,39),
        (313,39),(316,37),(320,36),(324,37),(327,34),(332,35),
        (336,32),(342,34),(348,32),(353,35),(356,40),(354,46),
        (358,49),(357,57))),
)

# Visible colour boundaries within the land, drawn independently of its coast.
INLAND = (
    ('northern-lowland', ((133,9),(140,10),(144,14),(142,19),
        (145,22),(140,26),(135,27),(132,24),(135,20),(133,16)), '#81968c', .32),
    ('western-upland', ((237,7),(247,8),(252,12),(247,15),
        (250,20),(245,24),(240,23),(240,18),(236,16)), '#e0d7bd', .5),
    ('central-lowland', ((258,17),(264,16),(270,19),(278,18),
        (282,21),(280,26),(275,28),(270,26),(265,28),(261,24)), '#7c998f', .42),
    ('eastern-upland', ((286,8),(292,10),(299,9),(303,12),
        (300,16),(296,15),(292,18),(288,16),(284,17)), '#e4ddc8', .46),
    ('eastern-lowland', ((298,23),(305,22),(311,24),(314,28),
        (320,28),(320,31),(315,33),(309,32),(306,29),(301,29)), '#8a9a89', .36),
    ('southern-upland', ((271,48),(278,45),(283,46),(286,42),
        (292,43),(295,46),(293,50),(288,52),(282,50),(277,52)), '#ddd3b7', .42),
    ('southern-lowland', ((304,45),(311,43),(318,46),(324,45),
        (330,49),(327,53),(320,55),(314,52),(307,53)), '#83988c', .3),
)

# Cloud margins follow the white swaths in the same source texture. They are
# translucent, so the habitat's newly drawn coast remains legible underneath.
CLOUDS = (
    ('western-cloud-bank', ((111,39),(119,35),(125,37),(132,33),
        (139,35),(144,31),(151,33),(158,29),(166,32),(174,29),
        (181,33),(184,38),(179,43),(173,44),(169,49),(161,47),
        (156,51),(148,49),(143,54),(136,51),(129,54),(123,51),
        (117,53),(112,49),(106,48),(108,44)), .24),
    ('northern-cloud-filament', ((143,11),(149,10),(153,7),(161,8),
        (164,11),(160,14),(155,15),(153,19),(148,22),(143,22),
        (146,18),(142,16)), .21),
    ('channel-cloud-bank', ((290,31),(295,30),(298,32),(302,31),
        (308,34),(312,32),(316,35),(321,34),(324,37),(322,41),
        (317,41),(314,44),(309,43),(306,40),(301,41),(298,38),
        (292,39),(290,36)), .22),
    ('southern-cloud-fringe', ((331,44),(335,40),(341,42),(344,39),
        (350,42),(353,47),(351,50),(346,48),(342,51),(339,49),
        (334,51),(330,48)), .18),
)


def habitat(d, parent, rx, ry, height):
    # Preserve the accepted cylindrical face and Vista's warm/light placement.
    # These are presentation coordinates, not a claim about the original UVs.
    def project(uv):
        u,v=uv
        theta=math.radians(47.109375+u*360/1024)
        return rx*math.cos(theta),ry*math.sin(theta)+((v-3)/55-.5)*height

    def outline(points):
        points=[project(p) for p in points]
        mid=lambda a,b:((a[0]+b[0])/2,(a[1]+b[1])/2)
        first=mid(points[-1],points[0])
        path=f'M{first[0]:.5f} {first[1]:.5f}'
        for i,p in enumerate(points):
            end=mid(p,points[(i+1)%len(points)])
            path+=f' Q{p[0]:.5f} {p[1]:.5f} {end[0]:.5f} {end[1]:.5f}'
        return path+' Z'

    land=d.linear('habitat-lit-land',[
        (0,(65,84,83)),(.065,(174,188,173)),(.145,(235,211,172)),
        (.25,(193,199,176)),(.5,(169,189,200)),(.7,(72,91,111)),(1,(22,27,34))],
        x1=-rx,y1=0,x2=rx,y2=0,calibrate=False)
    coast=element(parent,'g',id='source-outlined-habitat-coasts')
    for name,points in COASTS:
        element(coast,'path',id=name,d=outline(points),fill=land,opacity=.82)
    inland=element(parent,'g',id='source-outlined-inland-regions')
    for name,points,colour,alpha in INLAND:
        element(inland,'path',id=name,d=outline(points),fill=colour,opacity=alpha)
    cloud=element(parent,'g',id='source-outlined-cloud-regions',fill='#edf1e8')
    for name,points,alpha in CLOUDS:
        element(cloud,'path',id=name,d=outline(points),opacity=alpha)
