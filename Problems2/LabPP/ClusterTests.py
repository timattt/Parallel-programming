import subprocess
import matplotlib.pyplot as plt
import numpy as np

threads = np.array(range(1, 10))
#dts = [0.179677, 0.092115, 0.075130, 0.057838]
dts = [0.052934, 0.024854, 0.024114, 0.018091, 0.016458, 0.015037,0.011857, 0.014354,0.009032]

dts = np.array(dts)
dts *= 1000
dts = 1/dts

z = np.polyfit(threads, dts, 1)
f = np.poly1d(z)

dts = 1/dts

thrCont = np.linspace(threads[0], threads[-1], 100)

plt.plot(thrCont, 1/f(thrCont))
plt.scatter(threads, dts, label="parallel", color="red")

plt.xlabel("number thread")
plt.ylabel("miliseconds")
plt.legend()
plt.show()
