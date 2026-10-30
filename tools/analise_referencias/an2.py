import numpy as np, scipy.io.wavfile as w
def load(n):
    sr,x=w.read(n+".wav"); x=x.astype(np.float64)/32768.0
    return sr,(x[:,0]+x[:,1])/2
def f0_est(seg,sr,fmin=30,fmax=2000):
    seg=seg-seg.mean()
    n=len(seg); 
    X=np.fft.rfft(seg*np.hanning(n),4*n); ac=np.fft.irfft(np.abs(X)**2)[:n]
    lo=int(sr/fmax); hi=int(sr/fmin)
    k=np.argmax(ac[lo:hi])+lo
    # parabolic
    a,b,c=ac[k-1],ac[k],ac[k+1]; d=(a-c)/(2*(a-2*b+c)+1e-12)
    return sr/(k+d), ac[k]/ac[0]
