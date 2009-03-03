import sys
from os import system
from math import sqrt 
from itertools import izip
from pylab import show, plot, savefig, figure, hist, suptitle

ICdir   = '../ICs'
pkdgrav = './pkdgrav2m_pthread -p +std +overwrite -theta .6 -n 0 -I %s'
cello   = '../../cello --tipsy-std -i %s -o cello-%s --overwrite --theta=.6 --steps=1 --write-acc --force-n2'

ICs = [['grid512',     'grid512.std'], 
       ['plummer1000', 'plummer1000.std'],
       ['dark',        'dark.std']]

for name,ic in ICs[2:]:
    if True:
        ic = "%s/%s" % (ICdir, ic)

        cmd = pkdgrav % ic
        print cmd
        #system(cmd)
        #system("mv pkdgrav.accg pkdgrav-%s.acc.00001" % name)

        cmd = cello % (ic, name)
        print cmd
        #system(cmd)
        sys.exit(0)

    f0 = open('pkdgrav-%s.acc.00001' % name, 'r')
    f1 = open('cello-%s.acc.00001' % name, 'r')
    n0 = int(f0.readline())
    n1 = int(f1.readline())
    assert n0 == n1

    farray = open("%s.array" % name, 'w')
    data = []
    for i, (l0, l1) in enumerate(izip(f0,f1)):
        ax,ay,az = map(float, l0.split())
        ae = sqrt(ax**2 + ay**2 + az**2)

        ax,ay,az = map(float, l1.split())
        a0 = sqrt(ax**2 + ay**2 + az**2)

        e = abs(ae - a0) / ae * 100.0
        if e > 5: 
            print >>farray, 1
        else:
            print >>farray, 0
        data.append(e)
    farray.close()            

    figure()
    hist(data, bins=30)
    suptitle(name)
    savefig('%s.png' % name)

show()

