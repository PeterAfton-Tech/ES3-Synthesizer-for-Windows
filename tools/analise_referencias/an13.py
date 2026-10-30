import numpy as np
from an6 import loadLR
def run(n):
    sr,x=loadLR(n); m=(x[:,0]+x[:,1])/2
    h=int(0.00025*sr); K=len(m)//h
    env=np.array([np.max(np.abs(m[i*h:(i+1)*h])) for i in range(K)]); 
    # envolvente "de ciclo": máximo móvel de 4 ms (cobre meio ciclo de 130 Hz ≈ 3.8 ms)
    w=16; cyc=np.array([env[max(0,i-w+1):i+1].max() for i in range(K)])
    db=20*np.log10(cyc+1e-7)
    res=[]
    for i in range(w+2,K-w-2):
        if db[i-w]>-32 and db[i+4]<db[i-w]-25 and (not res or i-res[-1][1]>60): res.append(("OFF",i))
    for i in range(w+2,K-w-2):
        if db[i-4]<-45 and db[i+w]>-30 and (not res or i-res[-1][1]>60): res.append(("ON",i))
    return sr,m,h,db,sorted(res,key=lambda r:r[1])
for n in ["Sine","Triangle","Saw","Square","Pulse"]:
    sr,m,h,db,res=run(n); print("==",n)
    for t,i in res: print("  %s %.3fs"%(t,i*h/sr),"(dB, janela móvel 4ms, passo 0.25ms):"," ".join("%d"%round(v) for v in db[i-8:i+24]))
