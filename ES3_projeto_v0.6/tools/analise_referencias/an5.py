import numpy as np
from an2 import load
from scipy.optimize import minimize_scalar
def fold(seg,sr,f0,nb=256):
    t=np.arange(len(seg))/sr
    def score(f):
        ph=(t*f)%1.0
        z=np.sum(seg*np.exp(-2j*np.pi*ph)); return -abs(z)
    r=minimize_scalar(score,bounds=(f0*0.999,f0*1.001),method='bounded',options={'xatol':1e-5})
    f=r.x; ph=(t*f)%1.0
    idx=(ph*nb).astype(int); cyc=np.zeros(nb); cnt=np.zeros(nb)
    np.add.at(cyc,idx,seg); np.add.at(cnt,idx,1)
    cyc/=np.maximum(cnt,1)
    resid=seg-cyc[idx]
    return f,cyc,np.sqrt(np.mean(resid**2))/np.sqrt(np.mean(seg**2))
cases=[("Sine",2.5,4.8,130.88),("Triangle",0.5,2.4,130.87),("Saw",4.0,4.8,261.64),("Saw",2.9,3.6,155.56),("Square",6.6,8.8,130.83),("Square",2.8,3.7,261.64),("Pulse",0.7,2.7,130.82)]
for n,a,b,f0 in cases:
    sr,m=load(n); seg=m[int(a*sr):int(b*sr)]
    f,cyc,res=fold(seg,sr,f0)
    pk=cyc.max(); tr=cyc.min(); dc=cyc.mean()
    print("== %s %.2f-%.2fs f=%.4f Hz  (nota ~ %.1f cents vs A440 equal temp.)  max %.3f min %.3f DC %.4f  resíduo não-periódico %.1f dB"%(n,a,b,f,1200*np.log2(f/440)%100 if (1200*np.log2(f/440)%100)<50 else (1200*np.log2(f/440)%100)-100,pk,tr,dc,20*np.log10(res+1e-12)))
    c=cyc-dc; 
    pos=np.mean(c>0); 
    print("   fração positiva (duty ref. à média): %.3f"%pos)
    print("   ciclo (64 pontos):"," ".join("%.2f"%v for v in cyc[::4]/max(abs(pk),abs(tr))))
