import pdb
import numpy as np
import matplotlib.pyplot as plt
from math import factorial


def smooth(y, box_pts):
    box = np.ones(box_pts) / box_pts
    y_smooth = np.convolve(y, box, mode='same')

    return y_smooth


def control(number):
    # do some control here
    number = number + 1
    # debug purpose
    pdb.set_trace()

    return number


def myplot(a):
    t = np.arange(1.0, 501.0, 0.1)
    plt.plot(t, a, 'b-', lw=2)
    plt.plot(t, smooth(a, 100), 'r-', lw=2)
    plt.xlabel('Time[s]')
    plt.ylabel('Sinr[linear]')
    plt.show()
    pdb.set_trace()

    return 0
