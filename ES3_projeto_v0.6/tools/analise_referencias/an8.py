import numpy as np, scipy.signal as sg
from an6 import loadLR
sr,x=loadLR("Sine"); m=(x[:,0]+x[:,1])/2
# banda larga: remover componentes fora da faixa do seno para limpar a envolvente
def env_around(t0,pre=0.004,post=0.012,step=0.00025):
    a=int((t0-0.05)*sr); b=int((t0+0.05)*sr)
    seg=m[a:b]; an=np.abs(sg.hilbert(seg)); 
    out=[]
    for t in np.arange(-pre,post,step):
        i=int((t0+t)*sr)-a; out.append(an[i])
    return np.array(out)
for label,t0 in [("ON  587Hz 0.296s",0.2955),("ON  587Hz 1.442s",1.4415),("ON 130Hz 2.419s",2.4185),("ON 155Hz 7.588s",7.5875),("OFF 155Hz 7.567s",7.5635),("OFF 130Hz 2.09s",2.0855)]:
    # refinar: procurar o instante do salto máximo de envolvente em ±10 ms
    a=int((t0-0.01)*sr); b=int((t0+0.01)*sr); seg=m[a-2000:b+2000]; an=np.abs(sg.hilbert(seg))[2000:-2000]
    d=np.diff(20*np.log10(an+1e-9)); k=np.argmax(np.abs(d)); t1=(a+k)/sr
    e=env_around(t1)
    print(label,"salto em %.5fs"%t1,"\n   env (dB) a cada 0.25ms de -4ms a +12ms:"," ".join("%d"%round(20*np.log10(v+1e-9)) for v in e))
