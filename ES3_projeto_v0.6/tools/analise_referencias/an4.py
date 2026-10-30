import numpy as np
from an2 import load
from an3 import harm
cases=[("Sine",2.5,4.8),("Triangle",0.5,2.4),("Saw",4.0,4.8),("Saw",2.9,3.6),("Square",6.6,8.8),("Square",2.8,3.7),("Pulse",0.7,2.7)]
ideal={"Sine":lambda k:1.0 if k==1 else 0, "Triangle":lambda k:(1/k**2 if k%2 else 0),"Saw":lambda k:1/k,"Square":lambda k:(1/k if k%2 else 0)}
f0s={("Sine",2.5):130.88,("Triangle",0.5):130.87,("Saw",4.0):261.64,("Saw",2.9):155.56,("Square",6.6):130.83,("Square",2.8):261.64,("Pulse",0.7):130.82}
np.set_printoptions(linewidth=200)
for n,a,b in cases:
    sr,m=load(n); f0=f0s[(n,a)]
    seg=m[int(a*sr):int(b*sr)]
    # refine f0 using peak near
    N=len(seg); amps,X,fr=harm(seg,sr,f0,nh=30)
    db=20*np.log10(amps/amps[0]+1e-12)
    print("==",n,"%.2f-%.2f s f0 %.2f Hz"%(a,b,f0))
    print(" harm rel dB:", " ".join("%d:%.1f"%(k+1,d) for k,d in enumerate(db[:24])))
    if n in ideal:
        idl=np.array([ideal[n](k+1) for k in range(len(amps))]); 
        with np.errstate(divide='ignore'): idb=20*np.log10(idl/idl[0]+1e-12)
        print(" ideal  dB:", " ".join("%d:%.1f"%(k+1,d) for k,d in enumerate(idb[:24])))
    # residual energy outside harmonics
    tot=np.sum(X**2); mask=np.ones(len(X),bool)
    for k in range(1,int((sr/2)/f0)):
        c=k*f0; mask&=~((fr>c-8)&(fr<c+8))
    res=X**2*mask
    bands=[(0,1000),(1000,4000),(4000,8000),(8000,16000),(16000,24000)]
    print(" residual (non-harmonic) rel to total dB by band:", " ".join("%d-%dk:%.0f"%(lo//1000,hi//1000,10*np.log10(res[(fr>=lo)&(fr<hi)].sum()/tot+1e-15)) for lo,hi in bands))
    # spectral rolloff: level of highest harmonic
    print(" level of spectrum >16 kHz vs <16k (dB): %.0f"%(10*np.log10(np.sum(X[fr>16000]**2)/np.sum(X[fr<16000]**2)+1e-15)))
