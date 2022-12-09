import subprocess
import random
import filecmp
import os

SZ = 100
# 16, 30, 3
for testNum in range(SZ):
	i = random.randint(2, 1000)
	n = random.randint(1, 10)
	j = random.randint(2, 1000)
	
	if  testNum % (SZ / 100) ==0:
		print("{}%".format(round(testNum / SZ * 100)))

	os.system("./RAW {} {}".format(i, j))
	os.system("mpirun -n {} ./LAB {} {}".format(n, i, j))
	
	if filecmp.cmp("LAB.txt", "RAW.txt", shallow=False) == False:
		print("ERROR. i={}, j={}, n={}".format(i, j, n))
		quit(0)
		
print("100%")
print("OK")
	
