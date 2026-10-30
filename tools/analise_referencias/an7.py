import numpy as np
from an6 import loadLR
def events(n,show=True):
    sr,x=loadLR(n); m=(x[:,0]+x[:,1])/2
    hop=int(0.0005*sr); K=len(m)//hop
    env=np.array([np.max(np.abs(m[i*hop:(i+1)*hop])) for i in range(K)])
    db=20*np.log10(env+1e-7)
    # lisse por máximo em janelas de 5 ms para ignorar cruzamentos por zero
    sm=np.array([db[max(0,i-5):i+1].max() for i in range(K)])
    ev=[]
    for i in range(10,K-10):
        if sm[i]-sm[i-8]>30 and sm[i-8]<-45 and sm[i]>-30: ev.append(("on",i))
        if sm[i-8]-sm[i]>30 and sm[i]<-45 and sm[i-8]>-30: ev.append(("off",i))
    # dedup
    out=[];last=-100
    for t,i in ev:
        if i-last>40: out.append((t,i)); last=i
    return sr,m,db,out,hop
if __name__=="__main__":
  for n in ["Sine","Triangle","Saw","Square","Pulse"]:
    sr,m,db,out,hop=events(n)
    print("==",n)
    for t,i in out:
        a=max(0,i-30 if t=="off" else i-14); 
        print("  %s ~%.3fs  dB/0.5ms:"%(t.upper(),i*hop/sr)," ".join("%d"%round(v) for v in db[a:a+44]))
