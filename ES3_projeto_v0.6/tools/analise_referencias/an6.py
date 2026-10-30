import numpy as np, scipy.io.wavfile as w
def loadLR(n):
    sr,x=w.read(n+".wav"); return sr,x.astype(np.float64)/32768.0
for n in ["Sine","Triangle","Saw","Square","Pulse"]:
    sr,x=loadLR(n); m=(x[:,0]+x[:,1])/2
    nz=np.abs(m)>1e-6   # "silêncio digital" = exatamente zero
    # transições
    ons=[];offs=[]
    on=False; run=0
    # silêncio longo (>=20 ms de zeros exatos) separa notas
    z=~nz; i=0; N=len(m); gap=int(0.02*sr)
    # encontre blocos de zeros >= gap
    edges=np.flatnonzero(np.diff(np.concatenate([[0],z.astype(int),[0]])))
    starts=edges[0::2]; ends=edges[1::2]
    big=[(s,e) for s,e in zip(starts,ends) if e-s>=gap]
    print("==",n,"blocos de silêncio digital >=20ms:",[(round(s/sr,3),round(e/sr,3)) for s,e in big])
    for s,e in big:
        # onset logo após e
        if e<N-int(0.05*sr):
            seg=m[e:e+int(0.012*sr)]
            hop=int(0.0005*sr)
            env=[np.max(np.abs(seg[i:i+hop])) for i in range(0,len(seg)-hop,hop)]
            print("  ONSET @%.3fs  primeiras amostras: %s"%(e/sr," ".join("%.4f"%v for v in m[e:e+6])))
            print("    pico por 0.5 ms (12 ms):"," ".join("%.3f"%v for v in env))
        if s>int(0.05*sr):
            seg=m[s-int(0.060*sr):s]
            hop=int(0.002*sr)
            env=[np.max(np.abs(seg[i:i+hop])) for i in range(0,len(seg)-hop,hop)]
            print("  OFFSET @%.3fs últimos 60ms pico/2ms:"%(s/sr)," ".join("%.4f"%v for v in env))
            print("    últimas amostras:"," ".join("%.5f"%v for v in m[s-6:s]))
