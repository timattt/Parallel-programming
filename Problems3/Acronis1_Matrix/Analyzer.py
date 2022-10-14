import subprocess
import matplotlib.pyplot as plt
import time

figure, axis = plt.subplots(2, 2)

# no cache one thread
print("no cache one thread")
Ns = [128, 128*3, 128*5, 128*7, 128*9, 128*11]#[i for i in range(MIN_N, MAX_N, 100)]
times = []

for i, N in enumerate(Ns):
    print(N)
    start = time.time()
    subprocess.run(["build/Task.exe", str(2), str(N)])
    end = time.time()
    print(float(end - start))
    times.append(float(end - start))

axis[0, 0].set_xlabel("matrix size")
axis[0, 0].grid()
axis[0, 0].plot(Ns, times)
axis[0, 0].set_title("no cache, one thread")

# cache one thread
print("cache one thread")
times = []

for i, N in enumerate(Ns):
    print(N)
    start = time.time()
    subprocess.run(["build/Task.exe", str(2), str(N)])
    end = time.time()
    print(float(end - start))
    times.append(float(end - start))

axis[0, 1].set_xlabel("matrix size")
axis[0, 1].grid()
axis[0, 1].plot(Ns, times)
axis[0, 1].set_title("cache, one thread")

# no cache multi thread
print("no cache multi thread")
ths = [2, 4, 8, 16, 32]
times = []

for i, th in enumerate(ths):
    print(th)
    start = time.time()
    subprocess.run(["build/Task.exe", str(3), str(th)])
    end = time.time()
    print(float(end - start))
    times.append(float(end - start))

axis[1, 0].set_xlabel("threads num")
axis[1, 0].grid()
axis[1, 0].plot(ths, times)
axis[1, 0].set_title("no cache, multi thread")

# cache multi thread
print("cache multi thread")
times = []

for i, th in enumerate(ths):
    print(th)
    start = time.time()
    subprocess.run(["build/Task.exe", str(4), str(th)])
    end = time.time()
    print(float(end - start))
    times.append(float(end - start))

axis[1, 1].set_xlabel("threads num")
axis[1, 1].grid()
axis[1, 1].plot(ths, times)
axis[1, 1].set_title("cache, one thread")

plt.show()
    
