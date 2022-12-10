import subprocess
import matplotlib.pyplot as plt
import numpy as np

threads = np.array(range(1, 10))

dts = [0.052934, 0.028854, 0.024114, 0.018091, 0.016458, 0.015037,0.011857, 0.010354,0.009032]

dts = np.array(dts)
dts *= 1000
dts = 1/dts

z = np.polyfit(threads, dts, 1)
f = np.poly1d(z)

dts = 1/dts

thrCont = np.linspace(threads[0], threads[-1], 100)

fig, axs = plt.subplots(3)

axs[0].plot(thrCont, 1/f(thrCont))
axs[0].scatter(threads, dts, label="parallel", color="red")
axs[0].set_xlabel("number thread")
axs[0].set_ylabel("miliseconds")
axs[0].legend()

axs[1].plot(thrCont, dts[0]*f(thrCont))
axs[1].scatter(threads, dts[0]/dts, label="parallel", color="red")
axs[1].set_xlabel("number thread")
axs[1].set_ylabel("Ускорение")
axs[1].legend()

axs[2].plot(thrCont, dts[0]*f(thrCont) / thrCont)
axs[2].scatter(threads, dts[0]/dts / threads, label="parallel", color="red")
axs[2].set_xlabel("number thread")
axs[2].set_ylabel("Эффективность")
axs[2].legend()


plt.show()