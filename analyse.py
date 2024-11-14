#!/usr/bin/python

import statistics
import sys

def print_help():
    print("Usage: analyse.py FILE")

def analyse_wireshark(fname):
    f = open(fname, "r")
    lines = f.readlines()

    data = []
    stime = None
    etime = None
    ex_t = None

    for i in range(len(lines)):
        t, syn = tuple(lines[i].strip().split(","))
        if i == 0:
            assert syn == "False"
            stime = float(t)
            continue
        if i == len(lines)-1:
            etime = float(t)
            data.append(etime - stime)
            continue
        if syn == "False":
            etime = float(ex_t)
            data.append(etime - stime)
            stime = float(t)
        ex_t = t

    f.close()

    res = {}
    res["file"] = fname
    res["samples"] = len(data)
    res["mean"] = statistics.mean(data)
    if len(data) > 1:
        res["sd"] = statistics.variance(data)
    return res

def analyse_times(fname):
    f = open(fname, "r")
    lines = f.readlines()
    
    data = [float(line.strip()) for line in lines]

    f.close()
    
    res = {}
    res["file"] = fname
    res["samples"] = len(data)
    res["mean"] = statistics.mean(data)
    if len(data) > 1:
        res["sd"] = statistics.variance(data)
    return res

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print_help()
        sys.exit(1)
    fnames = sys.argv[1:]
    for fname in fnames:
        an = None
        if fname[-3:] == "csv":
            an = analyse_wireshark(fname)
        elif fname[-5:] == "times":
            an = analyse_times(fname)
        else:
            print("File {} not supported.".format(fname))
            continue
        print(an)
    sys.exit(0)
