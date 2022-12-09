import subprocess
import matplotlib.pyplot as plt
import numpy as np

threads = np.array(range(1, 9))
dts =np.array([1.577,0.818,0.534,0.412,0.35,0.293,0.252,0.223])

dts /= dts[0]

dts = 1/dts

z = np.polyfit(threads, dts, 1)
f = np.poly1d(z)

thrCont = np.linspace(threads[0], threads[-1], 100)

plt.plot(thrCont, f(thrCont))
plt.scatter(threads, dts, label="parallel", color="red")

plt.xlabel("number thread")
plt.ylabel("эффективность")
plt.legend()
plt.show()
