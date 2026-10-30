import numpy as np, scipy.signal as sg
from an6 import loadLR
from an2 import load
from an3 import harm
sr,x=loadLR("Noise"); L,R=x[:,0],x[:,1]; m=(L+R)/2
seg=m[int(0.25*sr):int(0.95*sr)]
f,P=sg.welch(seg,sr,nperseg=4096)
db=10*np.log10(P+1e-20)
def band(a,b): k=(f>=a)&(f<b); return db[k].mean()
cent=[100,200,400,800,1600,3200,6400,12800]
print("Ruído: densidade espectral média por oitava (dB):"," ".join("%d:%.1f"%(c,band(c/1.414,c*1.414)) for c in cent))
k=(f>100)&(f<12000); sl=np.polyfit(np.log2(f[k]),db[k],1)[0]
print("inclinação 100 Hz-12 kHz: %.2f dB/oitava (branco=0, rosa=-3)"%sl)
print("acima de 16 kHz vs 1-4 kHz: %.1f dB"%(band(16000,20000)-band(1000,4000)))
s=seg/np.std(seg); print("curtose %.2f (gaussiano=3, uniforme=1.8)  pico/RMS %.2f"%(np.mean(s**4),np.abs(s).max()))
Ls=L[int(0.25*sr):int(0.95*sr)]; Rs=R[int(0.25*sr):int(0.95*sr)]
print("correlação L/R: %.3f ; energia lado/meio %.1f dB"%(np.corrcoef(Ls,Rs)[0,1],10*np.log10(np.mean(((Ls-Rs)/2)**2)/np.mean(((Ls+Rs)/2)**2))))
# THD do seno vs nível
print("\nSine: harmônicas 3 e 5 (dB re fundamental) por trecho:")
sr,m=load("Sine")
for a,b,f0,lab in [(0.30,0.80,586.8,"587 Hz"),(2.5,4.8,130.88,"131 Hz (-8 dBFS)"),(7.7,8.4,155.6,"156 Hz"),(5.9,6.2,185.0,"185 Hz"),(6.35,6.55,311.2,"311 Hz")]:
    seg=m[int(a*sr):int(b*sr)]; am,_,_=harm(seg,sr,f0,nh=7)
    if len(am)<5: continue
    print("  %-18s rms %5.1f dBFS  h2 %.0f  h3 %.0f  h5 %.0f"%(lab,20*np.log10(np.sqrt(np.mean(seg**2))),20*np.log10(am[1]/am[0]),20*np.log10(am[2]/am[0]),20*np.log10(am[4]/am[0])))
