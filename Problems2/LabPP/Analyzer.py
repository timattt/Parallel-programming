import subprocess
import matplotlib.pyplot as plt
import numpy as np

dtsCons = []
dtsParal = []

threads = np.array(range(1, 7))

for th in threads:
    process = subprocess.Popen(['Task.exe', '10000', '5000', str(th)], stdout=subprocess.PIPE)
    output = process.stdout.readline()
    output = output.decode("utf-8")
    dtsCons.append(float(output.split(' ')[0]))
    dtsParal.append(float(output.split(' ')[1]))

print(dtsCons)
print(dtsParal)

z = np.polyfit(threads, dtsParal, 3)
f = np.poly1d(z)

thrCont = np.linspace(threads[0], threads[-1], 100)

plt.plot(thrCont, f(thrCont))
plt.plot([threads[0], threads[-1]], [np.sum(dtsCons)/len(dtsCons) for _ in range(2)], label="consistent")
plt.scatter(threads, dtsParal, label="parallel", color="red")

plt.xlabel("number thread")
plt.ylabel("seconds")
plt.legend()
plt.show()