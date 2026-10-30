import numpy as np, scipy.io.wavfile as w
from an2 import load, f0_est
def segments(m,sr):
    win=int(0.2*sr); hop=int(0.05*sr); fr=[]
    for i in range(0,len(m)-win,hop):
        s=m[i:i+win]; r=np.sqrt(np.mean(s**2))
        if r<3e-3: fr.append((i,0,0)); continue
        f,c=f0_est(s,sr,40,1500); fr.append((i,f if c>0.95 else 0,r))
    segs=[];cur=None
    for i,f,r in fr:
        if f==0: 
            if cur: segs.append(cur); cur=None
            continue
        if cur and abs(f/cur['f']-1)<0.003 and abs(20*np.log10(r/cur['r']))<1.5: cur['end']=i+win; cur['fs'].append(f); cur['r']=r
        else:
            if cur: segs.append(cur)
            cur={'start':i,'end':i+win,'f':f,'fs':[f],'r':r}
    if cur: segs.append(cur)
    return segs
def harm(seg,sr,f0,N=None,nh=24):
    N=len(seg) if N is None else N
    x=seg[:N]*np.blackman(N); X=np.abs(np.fft.rfft(x,8*N))*2/np.sum(np.blackman(N)); fr=np.fft.rfftfreq(8*N,1/sr)
    amps=[]
    for k in range(1,nh+1):
        fk=k*f0
        if fk>sr/2-500: break
        msk=(fr>fk*0.995)&(fr<fk*1.005); amps.append(X[msk].max())
    return np.array(amps),X,fr
if __name__=="__main__":
    import sys
    for n in ["Sine","Triangle","Saw","Square","Pulse"]:
        sr,m=load(n); segs=segments(m,sr)
        print("==",n,"segmentos estáveis (>=0.5s):")
        for s in segs:
            d=(s['end']-s['start'])/sr
            if d>=0.5: print("  %.2f-%.2fs  f0=%.2f Hz  rms %.1f dB"%(s['start']/sr,s['end']/sr,np.median(s['fs']),20*np.log10(s['r'])))
