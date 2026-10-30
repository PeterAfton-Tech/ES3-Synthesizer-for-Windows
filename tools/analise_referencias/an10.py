import numpy as np
from an6 import loadLR
from an2 import f0_est
sr,x=loadLR("Melody"); m=(x[:,0]+x[:,1])/2
print("Melody pico %.3f; exatamente-zero (<1e-6) em %.2f%% das amostras"%(np.abs(m).max(),100*np.mean(np.abs(m)<1e-6)))
# f0 por janelas de 50 ms
win=int(0.05*sr); fs=[]
for i in range(0,len(m)-win,win):
    s=m[i:i+win]; r=np.sqrt(np.mean(s**2))
    if r<1e-3: fs.append(0); continue
    f,c=f0_est(s,sr,60,1500); fs.append(f if c>0.9 else -1)
names=["C","C#","D","D#","E","F","F#","G","G#","A","A#","B"]
def nm(f):
    if f<=0: return "."
    n=69+12*np.log2(f/440); r=int(round(n)); return "%s%d"%(names[r%12],r//12-1)
seq=[];prev=None
for k,f in enumerate(fs):
    s=nm(f)
    if s!=prev: seq.append((k*0.05,s)); prev=s
print("notas (início s):"," ".join("%.2f:%s"%(t,s) for t,s in seq))
# eventos de nota: quedas bruscas de envolvente (janela 0.5 ms)
hop=int(0.0005*sr); K=len(m)//hop
env=np.array([np.max(np.abs(m[i*hop:(i+1)*hop])) for i in range(K)]); db=20*np.log10(env+1e-7)
sm=np.array([db[max(0,i-3):i+1].max() for i in range(K)])
ev=[]
for i in range(8,K-8):
    if sm[i]-sm[i-4]>12 : ev.append(("ON",i))
    if sm[i-4]-sm[i]>12 : ev.append(("OFF",i))
last=-99;out=[]
for t,i in ev:
    if i-last>20: out.append((t,i)); last=i
print("%d eventos; primeiros:"%len(out))
for t,i in out[:10]:
    print("  %s %.3fs dB/0.5ms:"%(t,i*hop/sr)," ".join("%d"%round(v) for v in db[i-10:i+16]))
