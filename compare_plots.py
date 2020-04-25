#!/usr/bin/env python

import datetime
import numpy
import os
import matplotlib.dates
import matplotlib.pyplot
import pickle

def extractAll(args):
    retlist = list()
    for arg in args:
        ldict = pickle.load( open( arg, "rb" ) )
        retlist.append(ldict)

    return retlist

def alignAll(inputlist):
    for ldict in inputlist:
        tdstart = ldict['time'][0] - inputlist[0]['time'][0]
        ldict['time'] = ldict['time'] - tdstart
    return inputlist

def plotAll(datalist):

    #years = matplotlib.dates.DayLocator()   # every year
    #months = matplotlib.dates.HourLocator()  # every month
    #years_fmt = matplotlib.dates.DateFormatter('%M')

    #i have no better idea right now
    #
    assert len(datalist) == 2, "only works with 2 imputs now"
    fig, ax = matplotlib.pyplot.subplots()
    ax.plot_date(datalist[0]['time'], numpy.transpose(numpy.array([datalist[0]['tc'],datalist[0]['diode']])),
            'o-')
    ax.hold(True)
    ax.plot_date(datalist[1]['time'], numpy.transpose(numpy.array([datalist[1]['tc'],datalist[1]['diode']])),
            '.-')
    #ax.xaxis.set_major_locator(years)
    #ax.xaxis.set_major_formatter(years_fmt)
    #ax.xaxis.set_minor_locator(months)
    #ax.format_xdata = matplotlib.dates.DateFormatter('%h:%M')
    fig.autofmt_xdate()
    matplotlib.pyplot.ylabel('temp [K]')
    matplotlib.pyplot.legend(['cryo(1)','diode(1)','cryo(2)','diode(2)'])
    matplotlib.pyplot.grid()
    matplotlib.pyplot.show()
    import pdb
    pdb.set_trace()


def main():
    from optparse import OptionParser
    parser = OptionParser(usage= 'Usage %prog [options] logfiles',
        description='Open, parse an plot data from log files')

    (options,args) = parser.parse_args()


    datalist = extractAll(args)
    datalist = alignAll(datalist)
    plotAll(datalist)


if __name__== "__main__":
    main()


