import numpy as np
from an2 import load
from an3 import harm
from scipy.optimize import minimize_scalar
sr,m=load("Pulse"); sp=m[int(0.7*sr):int(2.7*sr)]
sr,q=load("Square"); sq=q[int(6.6*sr):int(8.8*sr)]
f0=130.8134
ap,_,_=harm(sp,sr,f0,nh=30); aq,_,_=harm(sq,sr,f0,nh=30)
n=np.arange(1,len(ap)+1)
relp=20*np.log10(ap/ap[0]); relq=20*np.log10(aq/aq[0])
ideal_sq=np.where(n%2==1,-20*np.log10(n),-300)
H=np.where(n%2==1,relq-ideal_sq,np.nan)  # resposta do caminho (relativa ao fundamental), só nas ímpares
# interpola H para pares (em dB, vs. frequência) 
odd=n[n%2==1]; Hi=np.interp(n,odd,H[odd-1])
def model(d):
    A=np.abs(np.sin(n*np.pi*d))/n
    return 20*np.log10(A/A[0]+1e-9)+Hi
def cost(d):
    mm=model(d); ok=(relp>-45)  # ignora nulos
    return np.mean((relp[ok]-mm[ok])**2)
best=min(((cost(d),d) for d in np.linspace(0.05,0.5,901)))
print("melhor d = %.4f  (erro médio quadrático %.1f dB²)"%(best[1],best[0]))
d=best[1]; mm=model(d)
print("n:   obs / modelo(d)")
for k in range(0,24): print("%2d: %6.1f / %6.1f"%(n[k],relp[k],mm[k]))
# nível da fundamental de Pulse relativa ao 3º
print("H estimado (resposta do caminho, dB rel. fundamental) ímpares:"," ".join("%d:%.1f"%(k,v) for k,v in zip(odd[:12],H[odd[:12]-1])))
