from __future__ import with_statement
import sys
import pylab as p
from numpy import zeros


#infile = sys.argv[1]
#if len(sys.argv) == 3:
    #outfile = sys.argv[2]
#else:
    #outfile = None

def plot_q(file):
    xs = []
    ys = []
    print "Reading from", file
    with open(file, "r") as f:
        for lineno,line in enumerate(f):
            line = line.split()
            xs.append(int(line[0]))
            ys.append(int(line[1]))
    print "Done."

    img = zeros((max(xs)+1, max(ys)+1), 'float')

    print max(xs), max(ys)
    assert(len(xs) == len(ys))

    for i in range(len(xs)):
        img[xs[i], ys[i]] = (float(i)+1) / len(xs)

    print "Creating image"
    p.figure()
    #p.title(file)
    #p.jet()
    #p.spectral()

    #p.figimage(img[0:1000,0:1000])
    #p.gca().set_xlim([0,max(xs)])
    #p.gca().set_ylim([0,max(ys)])

    p.imshow(img[0:7000,0:7000])
    print "Done."

    #if outfile is None:
    #p.show()
    ##else:
    print "Saving image."
    #p.savefig("dark_prio_queue.png");
    print "Done."


for f in sys.argv[1:]:
    plot_q(f)

p.show()

