import subprocess
import matplotlib.pyplot as plt

totalThreads = 7

ts = []
dts = []

index = 0

names = ["balanced", "unbalanced", "locked"]

process = subprocess.Popen('Debug/LockFreeStack.exe', stdout=subprocess.PIPE)
while True:
    output = process.stdout.readline()

    if output == '' or process.poll() is not None:
        break
    if output:
        if index % totalThreads == 0 and index > 0:
            print(ts)
            print(dts)
            plt.plot(ts, dts, label = names[int(index / totalThreads) - 1])
            ts = []
            dts = []
    
        output = output.decode("utf-8")
        thrds = int(output.split(' ')[0])
        dt = int(output.split(' ')[1])
        
        dts.append(dt/1000000)
        ts.append(thrds)
        
        print("threads={} time={}".format(thrds, dt))
        index += 1

        
rc = process.poll()

if index % totalThreads == 0 and index > 0:
    print(ts)
    print(dts)
    plt.plot(ts, dts, label = names[int(index / totalThreads) - 1])
    ts = []
    dts = []
plt.xlabel("number thread")
plt.ylabel("seconds")
plt.legend()
plt.show()

