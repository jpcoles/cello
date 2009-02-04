from __future__ import with_statement
import sys
import pylab as p
from numpy import zeros, array, log2, arange, unique, append, fromfile
from math import ceil, floor

def transfer_time(win_sizes):
    bytes_per_node = 232
    bus_speed = 2e9
    return bytes_per_node * win_sizes / bus_speed

def compute_time(n_interactions):
    MOMR_op = 800
    gpu_speed = 3.45e11
    return n_interactions * MOMR_op / gpu_speed
    #return (win_sizes**2) * MOMR_op / gpu_speed

def plot_q(file, color, coords, slide):

    windows = [[2**x, set(), 0] for x in range(8,20)]
    bins    = array([[0,0,1e10,-1e10,0] for i in range(len(windows))], 'float')

    n_coords = len(coords)
    for i,xy in enumerate(coords):

        if i % 100000 == 0:
            print "%.2f" % (float(i)/n_coords)

        for bin,win in enumerate(windows):

            win[1].update(xy)
            win[2] += 1             # lines used by this window

            size = len(win[1])

            if size >= win[0] or i+1 == n_coords:
                #print i, len(coords)

                yval = float(win[2]) / size
                bins[bin][0] += yval
                bins[bin][1] += 1
                bins[bin][2] = min(bins[bin][2], yval)
                bins[bin][3] = max(bins[bin][3], yval)
                #bins[bin][4] += win[2]

                win[1].clear()
                win[2] = 0



#    for bin,win_size in enumerate(win_sizes):
#        print win_size,
#        sys.stdout.flush()
#
#        lines = 0
#
#        win.clear()
#        for i,xy in enumerate(coords):
#
#            win.update(xy)
#
#            #win.add(y)
#            #winlist.append(x)
#            #winlist.append(y)
#
##           win = append(win, xy)
##           if (len(win) >= win_size):
##               #print len(win),
##               win = unique(win)
##           #win = unique(append(win,xy))
##               #print win, len(win)
##           #print len(win>0)
#
#            lines += 1
#
#            if len(win) >= win_size or i+1 == len(coords):
#                #print i, len(coords)
#                size = len(win)
#
#                yval = float(lines) / size
#                bins[bin][0] += yval
#                bins[bin][1] += 1
#                bins[bin][2] = min(bins[bin][2], yval)
#                bins[bin][3] = max(bins[bin][3], yval)
#                bins[bin][4] += lines
#
#                win.clear()
#
##               #winlist = winlist[ int(ceil(len(winlist) * slide)) : ]
##               #win = set(winlist)
##               win = array([], 'int') #zeros(1)
#                lines = 0

    print bins

    bin_plot = bins[:,0] / bins[:,1]
    #avg_lines = bins[:,4] / bins[:,1]

    yerr0 = bin_plot - bins[:,2]
    yerr1 = bins[:,3] - bin_plot

    #ax = p.gca()
    xs = log2(win_sizes).astype(int)
    p.plot(xs, bin_plot, label=file, color=color)
    #p.errorbar(xs, bin_plot, yerr=[yerr0, yerr1], label=file)
    #p.semilogy(xs,  compute_time(avg_lines) / transfer_time(win_sizes) * 1e1, 'm--', label='compute/transfer')
    #p.plot(xs, transfer_time(win_sizes)*1e4, 'c--', xs, compute_time(avg_lines)*1e4, 'm--')
    #p.axhline(y=1, ls=':')

    #p.savefig("dark_prio_queue.png");
    print

    #for s,c,t in zip(win_sizes, compute_time(avg_lines), transfer_time(win_sizes)):
    #    print "(%i %e %e) " % (s,c,t)


slide_percents = [1.0] #, .75, .50, .25]

#for file in sys.argv[1:]:
#    c = []
#    print "Reading from %s..." % file,
#    sys.stdout.flush()
#    with open(file, "r") as f:
#        for lineno,line in enumerate(f):
#            line = line.split()
#            c.append([int(line[0]), int(line[1])])
#    coords.append(c)
#    print "Done."


coords = []
for file in sys.argv[1:]:
    print "Reading from %s..." % file, 
    sys.stdout.flush()
    c = fromfile(file, sep=' ', dtype=int)
    c = c.reshape(-1,2)
    coords.append(c)
    print "Done."

p.figure()
for i,slide in enumerate(slide_percents):
    ax = p.subplot(2,int(ceil(len(slide_percents)/2.0)), i+1)
    #p.subplot(2,len(slide_percents)/2, i+1)
    #p.set_ylim(ymin=0, ymax=50)
    colors = map(str, 1.0/len(sys.argv[1:]) * arange(len(sys.argv[1:])))
    print colors
    for file,color,cs in zip(sys.argv[1:], colors, coords):
    #for file,color,cs in zip(sys.argv[1:], ['r', 'g', 'b', 'o'], coords):
        print "%0.2f] %s:" % (slide,file),
        sys.stdout.flush()
        plot_q(file,color,cs,slide)
        #p.axis(ymin=0, ymax=50)
    p.text(.1, .6, "slide=%.2f" % slide, family='monospace', transform=ax.transAxes)

    p.legend(loc='upper left')

p.show()

