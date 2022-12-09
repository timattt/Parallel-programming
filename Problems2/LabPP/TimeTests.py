import subprocess
import matplotlib.pyplot as plt
import numpy as np

threads = np.array(range(1, 5))
dts = []

for th in threads:
    process = subprocess.Popen(["mpirun", "-n", str(th), "./LAB", "2000", "2520"], stdout=subprocess.PIPE)
    output = process.stdout.readline()
    dts.append(float(output))

print(dts)

dts = np.array(dts)

dts = 1/dts

z = np.polyfit(threads, dts, 1)
f = np.poly1d(z)

dts = 1/dts

thrCont = np.linspace(threads[0], threads[-1], 100)

plt.plot(thrCont, 1/f(thrCont))
plt.scatter(threads, dts, label="parallel", color="red")

plt.xlabel("number thread")
plt.ylabel("seconds")
plt.legend()
plt.show()
