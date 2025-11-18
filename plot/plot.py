import numpy as np
import matplotlib.pyplot as plt
import scipy.odr as odr
import numba
import math
from scipy.stats import chi2
    
#@numba.jit(nopython=True)
def poisson_interval(k, alpha=0.33): 
    """
    uses chisquared info to get the poisson interval. Uses scipy.stats 
    (imports in function). 
    """
    a = alpha
    low, high = (chi2.ppf(a/2, 2*k) / 2, chi2.ppf(1-a/2, 2*k + 2) / 2)
    if k == 0: 
        low = 0.0
#    return low, high
    return high - k
 


@numba.jit(nopython=True)
def fitfun(A, x):
    return A[2] + np.exp(A[0] - x*A[1])

data = []

with open("deltat.txt") as f:
    for line in f:
        try:
            data.append(int(line))
        except:
            pass
data = np.array(data)*6.25

bins = np.linspace(0, 18000, 100)

hist = np.histogram(data, bins=bins)

err_func = np.vectorize(poisson_interval, excluded=["alpha"])
errors = err_func(hist[0])
errors[errors < 1] = 1.0

x = (hist[1][:-1] + hist[1][1:])/2

model = odr.Model(fitfun)
cut = slice(1,None)
fitdata = odr.RealData(x[cut], hist[0][cut], sy=errors[cut])
myodr = odr.ODR(fitdata, model, beta0=[1, 1/2200, 1])
fitoutput = myodr.run()
#help(fitoutput)
fitoutput.pprint()

beta = fitoutput.beta
err = fitoutput.sd_beta
print("lifetime = %0.3f +/- %0.3f us"%(0.001/beta[1], 0.001*err[1]/beta[1]**2))
y = fitfun(beta, bins)

plt.step(bins, np.r_[hist[0], hist[0][-1]], where="post", c="blue")
plt.errorbar(x, hist[0], yerr=errors, fmt='none', c="blue")
plt.plot(bins, y, c="orange")
plt.yscale('log')
plt.tight_layout()
plt.ylabel("Events")
plt.xlabel("time [ns]")
plt.axvline(x[cut][0], c="red")
plt.tight_layout()
plt.savefig("hist.png")

