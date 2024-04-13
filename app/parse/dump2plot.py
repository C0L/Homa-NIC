delays = []
with open('parsed') as meds:
    for line in meds:
        split = line.split()
        if len(split) < 3:
            continue
        if split[2] == 'response':
            delays.append(int(split[3]))

sizes = [(x+1) * 1000 for x in range(200)]
           
# print(len(delays))
# print(len(sizes))

print(delays)

import numpy as np
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

coef = np.polyfit(sizes, delays, 1)
poly1d_fn = np.poly1d(coef)

fig, axs = plt.subplots(1)
# fig, axs = plt.subplots(1, figsize = (50,20))
# fig.tight_layout()

# plt.plot(sizes, delays, 'yo', sizes, poly1d_fn(sizes), '--k') #'--k'=black dashed line, 'yo' = yellow circle marker
plt.plot(sizes, delays, sizes, poly1d_fn(sizes), '--k') #'--k'=black dashed line, 'yo' = yellow circle marker
plt.xlabel('bytes')
plt.ylabel('nanoseconds')
plt.title('HW-Loopback Payload Round Trip Times (128 byte RW)')
plt.text(50000, 5000, f'{str(((1/coef[0]) / (10**-9) * 8 / (10**9)))} Gbps', fontsize='x-large')

plt.savefig('pcie_bandwidth.png')
