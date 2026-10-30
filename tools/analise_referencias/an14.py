import numpy as np
from an2 import load
sr=48000
def valley_peak(seg,f0,bands=((1000,4000),(4000,8000),(8000,12000),(12000,16000))):
    N=len(seg); w=np.blackman(N); X=np.abs(np.fft.rfft(seg*w,4*N)); fr=np.fft.rfftfreq(4*N,1/sr)
    out=[]
    for lo,hi in bands:
        pk=[];vl=[]
        k0=int(np.ceil(lo/f0)); k1=int(hi/f0)
        for k in range(k0,k1):
            a=(fr>k*f0-6)&(fr<k*f0+6); b=(fr>(k+0.5)*f0-6)&(fr<(k+0.5)*f0+6)
            if k*f0 >= 15900 : continue
            pk.append(X[a].max()**2); vl.append(X[b].max()**2)
        out.append(10*np.log10(np.mean(vl)/np.mean(pk)))
    return out
class Osc:
    pass
def synth(kind,f0,dur,scale):
    import subprocess
    return None
# sintetizador de referência em Python (PolyBLEP, mesma fórmula do ES3)
def polyblep(t,dt):
    if t<dt: t/=dt; return t+t-t*t-1
    if t>1-dt: t=(t-1)/dt; return t*t+t+t+1
    return 0.0
def gen(kind,f0,n,scale):
    out=np.zeros(n); ph=0.3; dt=f0/sr
    for i in range(n):
        t=ph
        if kind=="saw": v=2*t-1-scale*polyblep(t,dt)
        elif kind=="square":
            t2=(t+0.5)%1; v=(1 if t<0.5 else -1)+scale*(polyblep(t,dt)-polyblep(t2,dt))
        out[i]=v; ph+=dt; ph-=1 if ph>=1 else 0
    return out
refs=[("Saw",3.95,4.85,261.6251,"saw"),("Square",2.75,3.7,261.6231,"square")]
print("Razão vale/pico (dB) por banda  [1-4k, 4-8k, 8-12k, 12-16k] — mais negativo = mais limpo")
for n,a,b,f0,kind in refs:
    sr_,m=load(n); seg=m[int(a*sr):int(b*sr)]
    print("%-6s %.1f Hz GRAVAÇÃO:"%(n,f0)," ".join("%6.1f"%v for v in valley_peak(seg,f0)))
    for sc,lab in [(1.0,"PolyBLEP completo"),(0.5,"aliasing=0.5 (ES3 atual)"),(0.0,"ingênua (sem BLEP)")]:
        s=gen(kind,f0,len(seg),sc)
        print("       modelo %-26s"%lab," ".join("%6.1f"%v for v in valley_peak(s,f0)))
