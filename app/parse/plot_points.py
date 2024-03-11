import numpy as np
import matplotlib.pyplot as plt

# x = [1,2,3,4]
# y = [3,5,7,10] # 10, not 9, so the fit isn't perfect
x = [1024, 2*1024, 3*1024, 4*1024, 5*1024, 6*1024, 7*1024, 8*1024, 9*1024, 10*1024]
y = [2779 ,2976, 3688, 3527, 3709, 3957, 4275, 4541, 4709, 4958]

coef = np.polyfit(x,y,1)
poly1d_fn = np.poly1d(coef) 
# poly1d_fn is now a function which takes in x and returns an estimate for y

plt.plot(x, y, 'yo', x, poly1d_fn(x), '--k') #'--k'=black dashed line, 'yo' = yellow circle marker

plt.xlim(0, 5)
plt.ylim(0, 12)
plt.savefig('out.png')
