"""Manually drawn Vista title lettering in the original 1024 by 128 canvas.

The open A, asymmetric O, stepped serifs and detached O centre are individual
outlines. The paint anchors describe the shared metallic material; no source
pixels, substituted font or raster contour conversion enter the SVG.
"""
from vista_prompt_svg import element


LETTERING={
 'H':('M50 17 H90 V58 H169 V31 '
      'C181 31 189 36 195 43 V99 C195 102 200 103 206 103 V105 H195 V108 '
      'H180 V104 H169 V66 H101 V93 C101 102 110 105 119 106 V110 '
      'C113 108 110 108 109 112 H79 L80 107 H54 V105 H60 '
      'C65 105 66 101 66 95 V34 C66 23 62 20 50 20 Z'),
 'A':('M315 21 L390 101 H407 V105 H387 V109 H369 L366 105 H352 V102 H363 '
      'C369 102 366 99 364 97 L337 73 H295 L278 94 C272 101 273 102 282 102 '
      'H290 V105 H261 L259 109 H231 V103 H241 L271 67 H281 L288 61 H326 '
      'L305 39 L299 46 H290 L306 29 L308 31 Z'),
 'L':('M425 24 H463 V28 H489 V31 C477 31 474 34 474 41 V91 Q474 93 477 93 H537 '
      'C547 93 552 88 552 83 H555 V103 H550 C547 103 545 105 545 109 '
      'H456 L457 101 H429 V98 C443 98 446 93 446 85 V40 '
      'C446 29 442 26 425 26 Z'),
 'O':('M565 57 H576 C583 41 602 27 629 21 L626 17 '
      'C687 7 747 23 754 58 C755 62 761 64 774 64 V66 H762 '
      'C756 66 755 68 754 72 C747 99 702 115 657 115 L656 110 '
      'C636 110 622 107 609 102 L616 95 C639 103 675 105 703 91 '
      'C724 80 734 64 728 52 C718 28 674 22 648 27 '
      'C618 28 596 39 584 57 L583 65 H574 Z'),
 '2':('M870 28 L868.5 21 C896 7 938 10 958 29 C969 42 968 58 956 72 H932 '
      'L882 98 H905 L913 94 H962 C969 94 972 91 972 85 H975 V105 H971 '
      'C967 105 965 107 965 110 H848 L846 106 H829 V103 H840 '
      'L884 81 L888 85 C911 74 933 63 939 53 C948 36 931 29 919 28 '
      'C895 24 872 29 865 41 V48 H849 L845 42 H856 C861 35 866 31 870 28 Z'),
}

# Paint measurements from clear face areas of each original letter. The
# gradients intentionally differ: a shared ramp visibly changes the logo.
FACE={
 'H':((17,(255,255,255)),(42,(255,255,255)),(52,(253,253,254)),(62,(244,245,252)),
      (72,(225,227,246)),(82,(190,196,235)),(92,(134,150,218)),(100,(83,102,198)),(112,(26,45,165))),
 'A':((21,(255,255,255)),(42,(255,255,255)),(52,(253,253,254)),(62,(244,245,252)),
      (72,(221,225,245)),(82,(179,188,232)),(92,(111,130,209)),(100,(51,83,187)),(109,(26,45,165))),
 'L':((24,(255,255,255)),(40,(255,255,255)),(50,(254,254,254)),(60,(248,249,253)),
      (70,(233,236,248)),(80,(199,205,238)),(90,(140,154,219)),(98,(81,101,197)),(107,(26,45,165))),
 'O':((14,(255,255,255)),(35,(255,255,255)),(45,(254,254,254)),(55,(250,250,253)),
      (65,(241,242,251)),(75,(220,224,244)),(85,(184,192,233)),(95,(119,137,219)),(106,(60,88,191)),(115,(26,45,165))),
 '2':((14,(255,255,255)),(35,(255,255,255)),(45,(254,254,254)),(55,(248,249,253)),
      (65,(236,238,249)),(75,(208,213,242)),(83,(174,183,231)),(93,(116,134,212)),(103,(50,81,187)),(110,(26,45,165))),
}

# The polished seams follow the authored bevel, which changes width around
# serifs and corners. A constant-width outline cannot reproduce these seams.
SEAMS={
 'H':('M50 18 C65 18 68 24 68 34 L69 88 C69 102 65 104 54 105',
      'M89 20 C86 22 86 26 86 34 V59 H170 V32 C181 32 189 38 194 43 V102 H205',
      'M170 66 V102 H182 L181 106 H194',
      'M101 66 V93 C101 102 109 106 119 107',
      'M54 105 H81 Q83 105 81 110 H108 Q109 107 117 108'),
 'A':('M291 45 H299 L305 38 L328 62 H289 L282 68 H272 L242 104 H232',
      'M307 31 L315 22 L389 102 H406',
      'M289 103 H280 C272 103 274 99 278 94 L296 72 H337 L366 99 Q369 103 364 103 H353',
      'M232 106 H259 L261 103',
      'M368 105 L370 107 H386 L387 104'),
 'L':('M426 25 H462 L461 29 H488',
      'M430 99 C443 99 448 94 448 84 V39 C448 29 442 25 426 25',
      'M488 30 C476 31 473 35 473 43 V93.5 H537 C547 93.5 553 89 553.5 84',
      'M430 99 H459 Q461 99 459 106 H545.5 Q546 102 553.5 102 V85'),
 'O':('M566 58 H578 C585 40 603 28 630 22 L629 18 C689 10 745 25 753 59 '
      'C755 64 763 65 773 65',
      'M773 65 H762 C756 65 754 68 753 73 C746 98 704 113 658 113 V108 '
      'C637 109 623 106 611 102',
      'M616 96 C639 104 676 106 704 92 C725 80 735 64 729 51 '
      'C719 27 676 22 648 26 C618 27 594 40 583 58 L582 64 H575'),
 '2':('M846 43 H858 C863 34 868 30 872 28 L870.5 22 C899 10 937 13 958 30 '
      'C968 42 967 58 955 71 H932 L881 98',
      'M866 47 V42 C873 29 896 25 920 28 C934 29 948 37 940 53 '
      'C934 64 911 76 888 87 L884 83 L841 104 H830',
      'M881 98 H906 L914 95 H962 C969 95 973 91 973 86',
      'M831 104 H847 L849 108 H963 Q965 103 973 103 V86'),
}


def l_bevel(d,group,clip):
    """Separate stem, foot and hooked-tip surfaces measured on Vista's L.

    Its foot has a white upper bevel, a saturated blue face and a subdued
    lower ridge. Treating all three as one bright outline erases the step.
    """
    inside=element(group,'g',clip_path=clip)
    lower_clip=element(d.defs,'clipPath',id='L-foot-region')
    element(lower_clip,'rect',x=425,y=83,width=132,height=30)
    g=element(inside,'g',id='L-authored-bevel',clip_path='url(#L-foot-region)')
    foot=d.linear('L-foot-metal',[(0,(67,93,214)),(.3,(66,91,192)),
                 (.7,(30,62,187)),(1,(26,44,164))],y1=96,y2=109,calibrate=False)
    element(g,'path',d='M474 91 Q474 93 477 93 H537 C547 93 552 88 552 83 H555 '
            'V103 H550 C547 103 545 105 545 109 H456 L457 101 H474 Z',fill=foot)
    # The two edges of the foot have different reflectivity. Their editable
    # paths follow the long upper sweep and the short stepped lower cut.
    top='M474 92.8 H537 C547 92.8 552.4 88.6 552.7 83.5'
    element(g,'path',d=top,fill='none',stroke='#cbd5f5',stroke_width=4.4,
            filter=d.glow('L-upper-bevel-softness',.48))
    element(g,'path',d=top,fill='none',stroke='#f6f8ff',stroke_width=1.55,
            filter=d.glow('L-upper-polished-edge',.32))
    tip=d.linear('L-hook-reflection',[(0,(242,244,253)),(.6,(215,220,243)),
                 (1,(139,156,214))],y1=84,y2=105,calibrate=False)
    element(g,'path',d='M551 88 L552 83.5 H554 V102 H550 '
            'C546 102 544.5 104 544.5 107 H542.5 C542.5 102 546 99.5 550.5 99.5 Z',fill=tip,
            filter=d.glow('L-hook-bevel-softness',.45))
    element(g,'path',d='M554 84 V103 H550 C547 103 545 105 545 108.5',
            fill='none',stroke='#3e50b8',stroke_width=1.2)
    lower='M429 99 H458.5 Q461 99 458.5 106 H542.8 C543 102.2 546 100.8 550.5 100.8'
    element(g,'path',d=lower,fill='none',stroke='#839bd9',stroke_width=1.7,
            filter=d.glow('L-lower-ridge-softness',.55))
    element(g,'path',d='M456 108 H544',fill='none',stroke='#1a2ca4',stroke_width=1.65)
    element(g,'path',d='M426 25 H462 L461.5 29 H488',fill='none',stroke='#fbfcff',
            stroke_width=1.1,filter=d.glow('L-serif-polished-edge',.35))


def wordmark(d):
    d.family='halo2-title-wordmark'
    # Stable face samples, away from the bevel, establish a smooth material
    # ramp. These are a few paint parameters, never a per-pixel reconstruction.
    # The original has a narrow polished ridge surrounded by a darker cut,
    # rather than a single outline. Each band remains editable vector paint.
    cut=d.linear('wordmark-cut',[(0,(194,199,225)),(.5,(175,186,227)),(1,(44,66,164))],y1=17,y2=115)
    ridge=d.linear('wordmark-polished-ridge',[(0,(239,241,255)),(.5,(238,242,255)),(1,(151,177,250))],y1=17,y2=115)
    edge=d.linear('wordmark-outer-edge',[(0,(218,225,248)),(.5,(180,196,242)),(1,(41,61,165))],
                  y1=17,y2=115,calibrate=False)
    softened=d.glow('wordmark-bevel-softness',.65)
    seam=d.linear('wordmark-seam-light',[(0,(255,255,255)),(.55,(242,245,255)),
                  (1,(145,168,228))],y1=17,y2=115,calibrate=False)
    group=element(d.root,'g',id='halo2-wordmark',stroke_linejoin='round')
    for name,path in LETTERING.items():
        element(group,'path',id=name+'-outer-shade',d=path,fill='#292929',stroke='#292929',
                stroke_width=8,opacity=.85,filter=d.glow(name+'-outer-shade-blur',3.5))
        element(group,'path',id=name+'-shadow',d=path,fill='#161616',stroke='#161616',stroke_width=2,
                transform='translate(0 2.2)',opacity=.5,filter=d.glow(name+'-edge-softening',1.5))
        anchors=FACE[name];top=anchors[0][0];bottom=anchors[-1][0]
        paint=d.linear(name+'-metal-face',[((y-top)/(bottom-top),c) for y,c in anchors],
                       y1=top,y2=bottom,calibrate=False)
        clip=element(d.defs,'clipPath',id=name+'-inside');element(clip,'path',d=path,fill_rule='evenodd')
        element(group,'path',id=name,d=path,fill=paint,fill_rule='evenodd')
        bevel=element(group,'g',clip_path='url(#'+name+'-inside)',filter=softened)
        for width,ink,offset in ((7,cut,0),(3,ridge,.6),(3,edge,-.6)):
            element(bevel,'path',d=path,fill='none',stroke=ink,stroke_width=width,
                    transform=f'translate({offset} {offset})')
        seams=element(group,'g',id=name+'-polished-seams',fill='none',stroke_linecap='round',
                      stroke_linejoin='round')
        for line in SEAMS[name]:
            element(seams,'path',d=line,stroke='#4b5ba8',stroke_width=1.8,opacity=.5,
                    transform='translate(.65 .8)',filter=d.glow(name+'-seam-cavity-'+str(len(seams)),.4))
            element(seams,'path',d=line,stroke=seam,stroke_width=1.7,opacity=.4,
                    filter=d.glow(name+'-seam-bloom-'+str(len(seams)),.65))
            element(seams,'path',d=line,stroke=seam,stroke_width=1.2,opacity=1,
                    filter=d.glow(name+'-seam-light-'+str(len(seams)),.48))
        if name=='L':l_bevel(d,group,'url(#L-inside)')
    # Independent asymmetric oval and pointed extension inside the O.
    oval_anchors=((56,(255,255,255)),(61,(255,255,255)),(64,(248,248,253)),
                  (66,(231,233,248)),(68,(197,203,238)),(70,(153,166,223)),
                  (71,(157,167,223)),(72,(121,132,205)),(73.5,(26,44,164)))
    oval=d.linear('wordmark-centre-metal',[((y-56)/17.5,c) for y,c in oval_anchors],
                  y1=56,y2=73.5,calibrate=False)
    dot=('M642.8 64.5 C642.8 60.1 650 55.7 658 55.7 C666.1 55.7 671.7 60.1 674.4 63.5 '
         'H684.5 V65 H674.5 C672 69.2 665.6 73.5 657.9 73.5 '
         'C649.4 73.5 642.8 69.8 642.8 64.5 Z')
    element(group,'path',d=dot,fill='#292929',stroke='#292929',stroke_width=4,
            opacity=.65,filter=d.glow('oval-outer-shade',2.5))
    element(group,'path',id='central-oval',d=dot,fill=oval)
    element(group,'path',d='M643.2 64.2 C644 59.5 650.2 56.1 658 56.1 '
            'C665.5 56.1 671.4 60 674.2 64 H684',fill='none',stroke='#ffffff',stroke_width=.45,
            opacity=.65,filter=d.glow('oval-polished-edge',.25))
