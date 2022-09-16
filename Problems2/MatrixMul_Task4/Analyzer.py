import subprocess
import matplotlib.pyplot as plt

MIN_N = 100
MAX_N = 1000

ths = [i for i in range(1, 7)]

Ns = [i for i in range(MIN_N, MAX_N, 50)]
times = [[0 for _ in Ns] for _ in ths]

for j, th in enumerate(ths):
    print("threads: " + str(th))
    nxt = 0
    for i, N in enumerate(Ns):
        time = float(subprocess.Popen(["build/Task.exe", str(N), str(th)], stdout=subprocess.PIPE).communicate()[0])
        times[j][i] = time
        
        cur = i / len(Ns)
        
        if cur > nxt:
            nxt += 0.1
            print(str(int(cur * 100.0)) + "%")
    
    plt.plot(Ns, times[j], label = "threads: " + str(th))
    
plt.legend()
plt.xlabel("N - matrix size")
plt.ylabel("T - ellapsed mul time")
plt.show()
