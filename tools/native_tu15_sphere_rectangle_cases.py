"""Declared input cases for unchanged30270 and its whole2CBD0 model caller."""
import copy
import struct

def bits(v):return v if isinstance(v,int)else struct.unpack('<I',struct.pack('<f',v))[0]

def append_cases(new,center=0x1009000,output=0x1009100):
    camera=0x4c65c0;projection=0x4c6634
    identity=[1.,0.,0.,0.,1.,0.,0.,0.,1.]
    def world(put,eye=(1.,2.,3.),scale=1.,basis=identity,proj=(.9,-1.6,.05,-.02),vp=(0,0,720,1280),v=(1.,.5,-10.),point=None):
        for i,value in enumerate(eye):put(camera+4*i,bits(value))
        for i,value in enumerate(vp):put(camera+0x30+2*i,value,2)
        put(projection,bits(scale))
        for i,value in enumerate(basis):put(projection+4+4*i,bits(value))
        for offset,value in zip((0x78,0x8c,0x98,0x9c),proj,strict=True):put(projection+offset,bits(value))
        if point is None:point=[float(e)+float(d)for e,d in zip(eye,v,strict=True)]
        for i,value in enumerate(point):put(center+4*i,bits(value))
        for i in range(4):put(output+4*i,0x5a5a0000+i)
    qnan=0x7fc00000;snan=0x7f800001;denormal=1;inf=0x7f800000
    profiles=[
        ('bounded',{}),('behind',dict(v=(0.,0.,10.))),
        ('near-equality',dict(radius=0.,eye=(0.,0.,0.),point=[0,0,bits(-.01)])),
        ('center-qnan',dict(point=[qnan,bits(2.5),bits(-7.)])),('inside',dict(v=(0.,0.,-.5))),
        ('x-degenerate',dict(v=(0.,5.,-1.))),('y-degenerate',dict(v=(5.,0.,-1.))),
        ('x-near',dict(v=(5.,0.,-.5))),('y-near',dict(v=(0.,5.,-.5))),('x-far',dict(v=(-5.,0.,-.5))),
        ('scale-half',dict(scale=.5,v=(2.,1.,-20.))),('scale-qnan',dict(scale=qnan)),
        ('scale-snan',dict(scale=snan)),('scale-denormal',dict(scale=denormal,radius=.25)),
        ('radius-inf',dict(radius=inf)),('radius-qnan',dict(radius=qnan)),('radius-negative',dict(radius=-1.)),
        ('radius-denormal',dict(radius=denormal)),('depth-overflow',dict(v=(0.,0.,-1e20),eye=(0.,0.,0.))),
        ('lateral-overflow',dict(v=(3e19,1.,-10.),eye=(0.,0.,0.))),('signed-viewport',dict(vp=(-32768,-3,32767,-1))),
        ('rotated',dict(basis=[.36,.48,-.8,-.8,.6,0.,.48,.64,.6],v=(3.,-2.,-7.),radius=.75))]
    for radius in (.3,.7,1.1,1.3,2.9,7.293):profiles.append(('c1-survey-'+str(radius),dict(radius=radius,v=(1.7,-.9,-6.3))))
    profiles.extend([
        ('pc64',dict(fp=[0x37f,0,0x1f80])),('pc24',dict(fp=[0x7f,0,0x1f80])),
        ('condition-carry',dict(fp=[0x27f,0x4500,0x1f80])),('c1-carry',dict(fp=[0x27f,0x200,0x1f80])),
        ('no-root-carry',dict(fp=[0x27f,0x4700,0x1f80],v=(0.,0.,-.5))),
        ('early-carry',dict(fp=[0x27f,0x4700,0x1f80],v=(0.,0.,10.))),('df-carry',dict(eflags=0xed7))])
    for name,fp in [('c1-pc24',[0x7f,0,0x1f80]),('c1-pc64',[0x37f,0,0x1f80]),
            ('x87-round-up',[0xa7f,0,0x1f80]),('sse-round-up',[0x27f,0,0x5f80])]:
        profiles.append((name,dict(fp=fp,radius=.3,v=(1.7,-.9,-6.3))))
    profiles.extend([
        ('daz-ftz',dict(fp=[0x27f,0,0x9fc0],radius=denormal)),
        ('x-nan-payload-order',dict(proj=(0x7fc0aaaa,-1.6,0x7fc0bbbb,-.02))),
        ('y-nan-payload-order',dict(proj=(.9,0x7f80cccc,.05,0xffc0dddd))),
        ('eye-snan',dict(eye=(snan,2.,3.),point=[bits(2.),bits(2.5),bits(-7.)])),('zero-viewport',dict(vp=(0,0,0,0)))])
    for observe in (False,True):
        for name,profile in profiles:
            p=copy.deepcopy(profile);radius=p.pop('radius',1.);fp=p.pop('fp',[0x27f,0,0x1f80]);eflags=p.pop('eflags',0x202)
            row,put,finish=new(0x30270);row.update(kind='rectangle-observer'if observe else'rectangle-leaf',
                name='rectangle-'+name+('-observer'if observe else''),matrix=bits(radius),before_fp=fp,eflags=eflags,observe=observe)
            row['gp']=[center,projection,camera,0x0bbb0bbb,row['gp'][4],0x0eee0eee,output,0x0ddd0ddd]
            world(put,**p);finish()
    for default in (False,True):
        for name,options in [('reached',{}),('outer',dict(outer=1)),('inner',dict(inner=1)),
                ('no-section',dict(section=31)),('no-override',dict(override=0xffffffff))]:
            params=dict(section=5,override=0x77);params.update(options)
            row,put,finish=new(**params);row['kind']='rectangle-parent';row['name']='rectangle-parent-'+name+('-default'if default else'')
            row['native_rectangle_default']=default
            world(put);row['arguments'][20]=bits(1.);put(row['gp'][4]+4+4*20,bits(1.));finish()
