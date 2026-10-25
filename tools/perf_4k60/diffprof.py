import collections, sys, runpy, io, contextlib
def load(prefix, binary):
    sys.argv=[sys.argv[0],prefix,binary,'0','12']
    with contextlib.redirect_stdout(io.StringIO()):
        g=runpy.run_path(__file__.replace('diffprof.py','analyze.py'),run_name='analyze')
    inc=collections.Counter()
    for t,pcs in g['samples']:
        for n in set(g['sym'](pc) for pc in pcs): inc[n[:140]]+=1
    return inc,len(g['samples']),g['samples']
pa,pb,binary=sys.argv[1],sys.argv[2],sys.argv[3]
a,na,_=load(pa,binary); b,nb,_=load(pb,binary)
rows=[(b[k]/nb-a[k]/na,k) for k in set(a)|set(b)]
rows.sort(reverse=True)
print('samples',na,nb)
for d,k in (rows[:25] if len(sys.argv)<5 else rows[-25:]): print(f'{100*d:+6.2f}%  A={100*a[k]/na:5.2f} B={100*b[k]/nb:5.2f} {k}')
