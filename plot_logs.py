#!/usr/bin/env python

import datetime
import numpy
import os
import matplotlib.dates
import matplotlib.pyplot


diodev = numpy.array([1.420, 1.214, 1.107, 1.088, 1.071, 1.053,
    1.034, 1.015, 0.996,  0.976, 0.955, 0.934,
    0.912, 0.891, 0.869, 0.847, 0.824, 0.801,
    0.779, 0.756, 0.732, 0.709, 0.686, 0.662,
    0.638, 0.615, 0.591, 0.567, 0.543, 0.519,
    0.495, 0.471, 0.446, 0.422, 0.398])
diodet = numpy.array([10,20,30,40,50,60,
    70,80,90,100,110,120,
    130,140,150,160,170,180,
    190,200,210,220,230,240,
    250,260,270,280,290,300,
    310,320,330,340,350])
def volt_lookup(val):
    if numpy.isnan(val):
        return val

    try:
        #too low diode temp
        cmin = numpy.where(diodev < val)[0][0]
    except IndexError:
        return diodet[-1]

    if cmin == 0:
        return diodet[0]

    m = (diodet[cmin] - diodet[cmin-1])/(diodev[cmin]-diodev[cmin-1])
    b = diodet[cmin] - m * diodev[cmin]

    return m * val +b 


def convert_volt_to_temp(values):
    rval = list()
    for val in values:
        rval.append(volt_lookup(val))
    return rval

def get_merged_data(data,indexes,tag):
    currlist = list()
    for ii in indexes:
        currlist.extend(data[ii][tag])
    return currlist

def plot_log_data(data,flags,title=''):
    timestarts = list()
    for cdat in data:
        timestarts.append(data[cdat]['time'][0])

    #sorting time order of log files
    indexes = sorted(range(len(timestarts)), key=timestarts.__getitem__)
    #getting time ticks for all values
    timevals = get_merged_data(data,indexes,'time')
    dates = matplotlib.dates.date2num(timevals)

    if flags['gauge']:
        val1 = get_merged_data(data,indexes,'gauge')
        if all(numpy.isnan(val1)):
            print("can't plot gauge: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1, fmt='.-')
            matplotlib.pyplot.title(title + ' gauge')
            matplotlib.pyplot.show()

    if flags['power']:
        val1 = get_merged_data(data,indexes,'vpow')
        if all(numpy.isnan(val1)):
            print("can't plot vpow: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' vac power')
            matplotlib.pyplot.show()

    if flags['rpm']:
        val1 = get_merged_data(data,indexes,'vrpm')
        if all(numpy.isnan(val1)):
            print("can't plot vrpm: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' vac rpm')
            matplotlib.pyplot.show()

    if flags['motor_temp']:
        val1 = get_merged_data(data,indexes,'vmtemp')
        if all(numpy.isnan(val1)):
            print("can't plot vmtepm: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' vac motor temp')
            matplotlib.pyplot.show()

    if flags['ictemp']:
        val0 = get_merged_data(data,indexes,'a0')
        val1 = get_merged_data(data,indexes,'a1')
        val2 = get_merged_data(data,indexes,'a2')
        val3 = get_merged_data(data,indexes,'a3')
        if all(numpy.isnan(val1)):
            print("can't plot a0: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, numpy.transpose(numpy.array([val0,val1,val2,val3])),fmt='.-')
            matplotlib.pyplot.title(title + ' I2C temp sensors')
            matplotlib.pyplot.legend(['a0','a1','a2','a3'])
            matplotlib.pyplot.show()

    if flags['cryo_temp'] and not flags['diode']:
        val1 = get_merged_data(data,indexes,'ctc')
        if flags['fname']:
            import pickle
            savedict={'time':dates,'tc':val1}
            pickle.dump( savedict, open( flags['fname'], "wb" ) )

        if all(numpy.isnan(val1)):
            print("can't plot ctc: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' cryo temp')
            matplotlib.pyplot.ylabel('temp [K]')
            matplotlib.pyplot.grid()
            matplotlib.pyplot.show()

    if flags['cryo_temp'] and flags['diode']:
        val1 = get_merged_data(data,indexes,'ctc')
        val0 = get_merged_data(data,indexes,'diode')
        val0v = convert_volt_to_temp(val0)
        if flags['fname']:
            import pickle
            savedict={'time':dates,'diode':val0v,'tc':val1}
            pickle.dump( savedict, open( flags['fname'], "wb" ) )

        if all(numpy.isnan(val1)):
            print("can't plot ctc/diode: all nans")
        elif all(numpy.isnan(val0)):
            print("can't plot diode/ctc: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, numpy.transpose(numpy.array([val1,val0v])),fmt='.-')
            matplotlib.pyplot.title(title + ' cryo temp')
            matplotlib.pyplot.ylabel('temp [K]')
            matplotlib.pyplot.legend(['cryo','diode'])
            matplotlib.pyplot.grid()
            matplotlib.pyplot.show()

    if flags['cryo_power']:
        val0 = get_merged_data(data,indexes,'cmaxp')
        val1 = get_merged_data(data,indexes,'cminp')
        val2 = get_merged_data(data,indexes,'ccurrp')
        if all(numpy.isnan(val0)):
            print("can't plot cryo power: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, numpy.transpose(numpy.array([val0,val1,val2])),fmt='.-')
            matplotlib.pyplot.title(title + ' cryo power')
            matplotlib.pyplot.ylabel('power [W]')
            matplotlib.pyplot.legend(['max','min','current'])
            matplotlib.pyplot.grid()
            matplotlib.pyplot.show()

    if flags['diode']:
        val1 = get_merged_data(data,indexes,'diode')
        if all(numpy.isnan(val1)):
            print("can't plot diode: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' diode voltage')
            matplotlib.pyplot.show()

    if flags['fan_pwm']:
        val1 = get_merged_data(data,indexes,'fanpwm')
        if all(numpy.isnan(val1)):
            print("can't plot fan pwm: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' fan pwm')
            matplotlib.pyplot.show()

    if flags['fan_rpm']:
        val1 = get_merged_data(data,indexes,'fanrpm')
        if all(numpy.isnan(val1)):
            print("can't plot fan rpm: all nans")
        else:
            fig, ax = matplotlib.pyplot.subplots()
            ax.plot_date(dates, val1,fmt='.-')
            matplotlib.pyplot.title(title + ' fan rpm')
            matplotlib.pyplot.show()

def get_data_from_log_file(filename):
    time = []
    gauge = []
    vpow = []
    vrpm = []
    vmtemp = []
    a0 = []
    a1 = []
    a2 = []
    a3 = []
    ctc = []
    cmaxp = []
    cminp = []
    ccurrp = []
    diode = []
    fanpwm = []
    fanrpm = []

    filebase_dates = os.path.basename( filename ).split('-')
    dtstart = datetime.datetime( int(filebase_dates[0]), int(filebase_dates[1]), int(filebase_dates[2]), 
            int(int(filebase_dates[3])), int(int(filebase_dates[4])))
    
    data_aval = 0
    with open(filename, 'r') as infile:
        fline = infile.readline()
        secondline = infile.readline()
        lineparts = secondline.split()
        expectedlines = ['Time', 'Guage', 'p316', 'p398', 'p346', 'A0', 'A1', 'A2', 'A3', 'TC', 'Max', 'Min', 'Cur', '(V)', 'PWM', 'RPM']
        assert lineparts==expectedlines, "file does not seem to contain rimbox log file"
        dtlast = dtstart
        for line in infile:
            lineparts = line.split()
            assert len(lineparts) == 16, "unable to parse line, quitting"
            dtnow = dtlast
            ho,mi=lineparts[0].split(':')
            dtnow = dtnow.replace(hour=int(ho),minute=int(mi))
            #we substituted minutes and hours. if next line has lower minutes and hours than last, 
            #that means we hit next day
            if(dtnow < dtlast):
                dtnow += datetime.timedelta(days=1)
            dtlast = dtnow
            time.append(dtnow)

            try:
                cgauge = float(lineparts[1])
            except ValueError:
                cgauge = numpy.nan
            gauge.append(cgauge)

            try:
                cvpow = float(lineparts[2])
            except ValueError:
                cvpow = numpy.nan
            vpow.append(cvpow)

            try:
                cvrpm = float(lineparts[3])
            except ValueError:
                cvrpm = numpy.nan
            vrpm.append(cvrpm)

            try:
                cvmtemp = float(lineparts[4])
            except ValueError:
                cvmtemp = numpy.nan
            vmtemp.append(cvmtemp)

            try:
                ca0 = float(lineparts[5])
            except ValueError:
                ca0 = numpy.nan
            a0.append(ca0)

            try:
                ca1 = float(lineparts[6])
            except ValueError:
                ca1 = numpy.nan
            a1.append(ca1)

            try:
                ca2 = float(lineparts[7])
            except ValueError:
                ca2 = numpy.nan
            a2.append(ca2)

            try:
                ca3 = float(lineparts[8])
            except ValueError:
                ca3 = numpy.nan
            a3.append(ca3)

            try:
                cctc = float(lineparts[9])
            except ValueError:
                cctc = numpy.nan
            ctc.append(cctc)

            try:
                ccmaxp = float(lineparts[10])
            except ValueError:
                ccmaxp = numpy.nan
            cmaxp.append(ccmaxp)

            try:
                ccminp = float(lineparts[11])
            except ValueError:
                ccminp = numpy.nan
            cminp.append(ccminp)

            try:
                cccurrp = float(lineparts[12])
            except ValueError:
                cccurrp = numpy.nan
            ccurrp.append(cccurrp)

            try:
                cdiode = float(lineparts[13])
            except ValueError:
                cdiode = numpy.nan
            diode.append(cdiode)

            try:
                cfanpwm = float(lineparts[14])
            except ValueError:
                cfanpwm = numpy.nan
            fanpwm.append(cfanpwm)

            try:
                cfanrpm = float(lineparts[15])
            except ValueError:
                cfanrpm = numpy.nan
            fanrpm.append(cfanrpm)
            
            data_aval = 1

    assert data_aval, "file {} contains no data".format(filename)
    return {'time':time, 'gauge':gauge, 'vpow':vpow, 'vrpm':vrpm, 'vmtemp':vmtemp,
            'a0':a0,'a1':a1,'a2':a2,'a3':a3, 'ctc':ctc, 'cmaxp':cmaxp, 'cminp':cminp,
            'ccurrp':ccurrp, 'diode':diode, 'fanpwm':fanpwm, 'fanrpm':fanrpm }

def parse_and_plot_logs(filelist,flags,title):
    #testin 
    for ff in filelist:
        if not os.path.isfile(ff):
            raise RuntimeError("file {} does not exist or is not a regular file".format(ff))

    data = dict()
    for ii in range(len(filelist)):
        data[ii] = get_data_from_log_file(filelist[ii])

    plot_log_data(data,flags,title)
    

def main():
    from optparse import OptionParser
    parser = OptionParser(usage= 'Usage %prog [options] logfiles',
        description='Open, parse an plot data from log files')

    parser.add_option('-a','--all', dest='do_all', action="store_true", default=False,
        help ='plot all series')
    parser.add_option('-g','--gauge', dest='do_gauge', action="store_true", default=False,
        help ='plot gauge')
    parser.add_option('-p','--power', dest='do_power', action="store_true", default=False,
        help ='plot vac power')
    parser.add_option('-r','--rpm', dest='do_rpm', action="store_true", default=False,
        help ='plot vac rpm')
    parser.add_option('-m','--motort', dest='do_mt', action="store_true", default=False,
        help ='plot vac motor temperature')
    parser.add_option('-t','--temp', dest='do_temp', action="store_true", default=False,
        help ='plot I2C temperatures')
    parser.add_option('-c','--cryopow', dest='do_cpow', action="store_true", default=False,
        help ='plot cryo powers')
    parser.add_option('-d','--diode', dest='do_diode', action="store_true", default=False,
        help ='plot diode voltage')
    parser.add_option('-F','--fan-pwm', dest='do_fanpwm', action="store_true", default=False,
        help ='plot fan pwm')
    parser.add_option('-f','--fan', dest='do_fan', action="store_true", default=False,
        help ='plot fan rpm')
    parser.add_option('-s','--sign', type=str, dest='title_string', action="store", default='',
        help ='plot fan rpm')
    parser.add_option('--save', type=str, dest='filename', action="store", default=None,
        help ='save temperature plot to file')

    (options,args) = parser.parse_args()
    
    flags = dict()
    if options.do_all:
        flags['gauge'] = True
        flags['power'] = True
        flags['rpm'] = True
        flags['motor_temp'] = True
        flags['ictemp'] = True
        flags['cryo_temp'] = True
        flags['cryo_power'] = True
        flags['diode'] = True
        flags['fan_pwm'] = True
        flags['fan_rpm'] = True
    else:
        flags['gauge'] = options.do_gauge
        flags['power'] = options.do_power
        flags['rpm'] = options.do_rpm
        flags['motor_temp'] = options.do_mt
        flags['ictemp'] = options.do_temp
        flags['cryo_temp'] = True
        flags['cryo_power'] = options.do_cpow
        flags['diode'] = options.do_diode
        flags['fan_pwm'] = options.do_fanpwm
        flags['fan_rpm'] = options.do_fan

    flags['fname'] = options.filename

    parse_and_plot_logs(args,flags,options.title_string)

if __name__== "__main__":
    main()
