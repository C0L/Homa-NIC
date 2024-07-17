import numpy as np

slotstats_t = np.dtype(
    [
        ('occupied', np.uint64),
        ('arrivals', np.uint64),
        ('arrivalbacklog', np.uint64)
     ]
)

queuestats_t = np.dtype(
    [
        ('highwater', np.uint64),
        ('lowwater', np.uint64),
        ('maxsize', np.uint64)
    ]
)

simstats_t = np.dtype(
    [
        ('packets', np.uint64),
        ('comps', np.uint64),
        ('comptimes', np.uint64),
        ('slowdowns', np.uint64),
        ('violations', np.double),
        ('underflow', np.uint64),
        ('cycles', np.uint64),
        ('msginacc', np.uint64),
        ('pktinacc', np.uint64)
    ]
)

def parse_stats(fn):
    queuestats = np.fromfile(fn, dtype=queuestats_t, count=1)
    simstats   = np.fromfile(fn, dtype=simstats_t, count=1, offset=queuestats.nbytes)
    slotstats  = np.fromfile(fn, dtype=slotstats_t, count=-1, offset=(queuestats.nbytes + simstats.nbytes))

    return queuestats, simstats, slotstats

def add_subplot_axes(ax,rect,facecolor='w'): # matplotlib 2.0+
    fig = plt.gcf()
    box = ax.get_position()
    width = box.width
    height = box.height
    inax_position  = ax.transAxes.transform(rect[0:2])
    transFigure = fig.transFigure.inverted()
    infig_position = transFigure.transform(inax_position)    
    x = infig_position[0]
    y = infig_position[1]
    width *= rect[2]
    height *= rect[3]  # <= Typo was here
    subax = fig.add_axes([x,y,width,height],facecolor=facecolor)  # matplotlib 2.0+
    x_labelsize = subax.get_xticklabels()[0].get_size()
    y_labelsize = subax.get_yticklabels()[0].get_size()
    x_labelsize *= rect[2]**0.5
    y_labelsize *= rect[3]**0.5
    subax.xaxis.set_tick_params(labelsize=x_labelsize)
    subax.yaxis.set_tick_params(labelsize=y_labelsize)
    return subax
