#!/usr/bin/env python3
import sys
assert sys.version_info[0] == 3, "Python 3 required."

# Lizard
# Copyright Brad Smith 2018
# http://lizardnes.com

#
# Generates debugging symbols for FCEUX and other statistics info
#

from collections import OrderedDict

def stat_label(l):
    if (l.find("stat") != -1):
        return True
    return False

def read_labels(label_file, range_min, range_max, stats):
    labs = {}
    labels = []
    try:
        of = open(label_file, "rt")
        labels = of.readlines()
    except IOError:
        print("skipped: "+label_file)
        return labs
    for line in labels:
        words = line.split()
        if (words[0] == "al"):
            adr = int(words[1], 16)
            sym = words[2]
            sym = sym.lstrip('.')
            if (sym[0] == '_' and sym[1] == '_'):
                continue # skip compiler internals
            if stats != stat_label(sym):
                continue
            if sym.isupper():
                continue # skip allcaps labels by convention (usually exported constants)
            if (adr >= range_min and adr <= range_max):
                if (adr in labs):
                    # multiple symbol
                    text = labs[adr]
                    textsplit = text.split()
                    if (sym not in textsplit):
                        text = text + " " + sym
                        labs[adr] = text
                else:
                    labs[adr] = sym
    return labs    

def read_symbols(label_file):
    labs = {}
    labels = []
    try:
        of = open(label_file, "rt")
        labels = of.readlines()
    except IOError:
        print("skipped: "+label_file)
        return labs
    for line in labels:
        words = line.split()
        if (words[0] == "al"):
            adr = int(words[1], 16)
            sym = words[2]
            sym = sym.lstrip('.')
            if (sym[0] == '_' and sym[1] == '_'):
                continue # skip compiler internals
            labs[sym] = adr
    return labs

def label_to_nl(label_file, nl_file, range_min, range_max):
    labs = read_labels(label_file, range_min, range_max, False)
    sout = ""
    for (adr, sym) in labs.items():
        sout += ("$%04X#%s#\n" % (adr, sym))
    open(nl_file, "wt").write(sout)
    print("debug symbols: " + nl_file)

def label_to_wch(label_file, wch_file, range_min, range_max):
    labs = read_labels(label_file, range_min, range_max, False)
    labels = []
    sout = ""
    labs[range_max+1] = "END"
    sout += "\n"
    sout += "%d\n" % (1+range_max-range_min)
    last_adr = 0x0000
    last_sym = "NONE"
    count = 0
    labs = OrderedDict(sorted(labs.items(), key=lambda t: t[0]))
    for (adr, sym) in labs.items():
        # print ("> %04X = %s" % (adr,sym))
        l = adr-last_adr
        if l == 1:
            sout += "%05X\t%04X\tb\th\t0\t%s\n" % (count, last_adr, last_sym)
            count += 1
        else:
            subcount = 0
            for a in range(last_adr,adr):
                sout += "%05X\t%04X\tb\th\t0\t%s+%X\n" % (count, a, last_sym, subcount)
                subcount += 1
                count += 1
        last_sym = sym
        last_adr = adr
    open(wch_file, "wt").write(sout)
    print("watch: " + wch_file)

def label_to_stats(label_file, stat_file, range_min, range_max):
    labs = read_labels(label_file, range_min, range_max, True)
    labs[range_max+1] = "END_stat"
    labs = OrderedDict(sorted(labs.items(), key=lambda t: t[0]))
    sout = ""
    last_lab = "START"
    last_addr = range_min
    for (adr, sym) in labs.items():
        diff = adr - last_addr
        sout += ("%8d, $%04X, %s\n" % (diff,last_addr,last_lab))
        last_lab = sym
        last_addr = adr
    open(stat_file, "wt").write(sout)
    print("code stats: " + stat_file)

# returns: (segment start addr, segment end addr, space since last segment)
def map_to_segment(map_file, seg_search):
    try:
        of = open(map_file, "rt")
        lines = of.readlines()
    except IOError:
        print("skipped: "+map_file)
        return (0,0,0)
    print("segment: "+map_file+" ["+seg_search+"]")
    # find segment list
    seg_list = -1
    for i in range(0,len(lines)):
        if lines[i].startswith("Segment list:"):
            seg_list = i
            break
    if (seg_list < 0):
        print("no segment list in: "+map_file)
        return (0,0,0)
    seg_found = False
    seg_last_end = 0
    seg_start = 0
    seg_end = 0
    for i in range(seg_list+4,len(lines)):
        tokens = lines[i].split()
        if (len(tokens) != 5):
            break
        seg_start = int(tokens[1],16)
        seg_end = int(tokens[2],16)
        if (tokens[0] == seg_search):
            seg_found = True
            break
        seg_last_end = seg_end
    if (seg_found == False):
        print ("segment "+seg_search+" not found in: "+map_file)
        return (0,0,0)
    return (seg_start, seg_end, seg_start - (seg_last_end+1))

def lua_generate(file_prefix,file_suffix,file_out):
    fo = open(file_out, "wt")
    #lf = read_symbols(file_prefix + "F" + file_suffix)
    le = read_symbols(file_prefix + "E" + file_suffix)
    ld = read_symbols(file_prefix + "D" + file_suffix)
    common_sym = [
        "i",
        "ih",
        "lizard_hitbox_x0",
        "lizard_hitbox_xh0",
        "lizard_hitbox_y0",
        "lizard_hitbox_x1",
        "lizard_hitbox_xh1",
        "lizard_hitbox_y1",
        "lizard_touch_x0",
        "lizard_touch_xh0",
        "lizard_touch_y0",
        "lizard_touch_x1",
        "lizard_touch_xh1",
        "lizard_touch_y1",
        "blocker_x0",
        "blocker_xh0",
        "blocker_y0",
        "blocker_x1",
        "blocker_xh1",
        "blocker_y1",
        "scroll_x",
        "bus_conflict",
        "dog_type",
        "DOG_RIVER",
        "DOG_FROB",
        "metric_time_h",
        "metric_time_m",
        "metric_time_s",
        "metric_time_f" ]
    banked_sym = [
        "lizard_overlap",
        "lizard_touch" ]
    s = "-- auto generated address include file\n"
    for sym in common_sym:
        n = "addr_" + sym
        if sym in le:
            s += n + " = 0x%04X;\n" % le[sym]
        elif sym in ld:
            s += n + " = 0x%04X;\n" % ld[sym]
        else:
            #s += n + " = 0; -- missing!\n"
            print("Missing symbol: "+sym)
    for sym in banked_sym:
        #nf = "addr_" + sym + "F"
        #if sym in lf:
        #    s += nf + " = 0x%04X;\n" % lf[sym]
        #else:
        #    #s += nf + " = 0; -- missing!\n"
        #    print("Missing symbol in F: "+sym)
        ne = "addr_" + sym + "E"
        if sym in le:
            ae = le[sym]
            s += ne + " = 0x%04X;\n" % ae
        else:
            #s += ne + " = 0; -- missing!\n"
            print("Missing symbol in E: "+sym)
        nd = "addr_" + sym + "D"
        if sym in ld:
            ad = ld[sym]
            s += nd + " = 0x%04X;\n" % ad
        else:
            #s += nd + " = 0; -- missing!\n"
            print("Missing symbol in D: "+sym)
    fo.write(s)

if __name__ == "__main__":
    label_to_nl("temp\\nsf.labels.txt", "temp\\lizard.nsf.ram.nl", 0x0000, 0x7FF)
    label_to_nl("temp\\nsf.labels.txt", "temp\\lizard.nsf.0.nl", 0x8000, 0xFFFF)
    label_to_wch("temp\\nsf.labels.txt", "temp\\lizard.nsf.wch", 0x0000, 0x07FF)
    label_to_nl("temp\\nes.0.labels.txt", "temp\\lizard.nes.ram.nl", 0x0000, 0x7FF)
    label_to_nl("temp\\nes.0.labels.txt", "temp\\lizard.nes.0.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.0.labels.txt", "temp\\lizard.nes.1.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.1.labels.txt", "temp\\lizard.nes.2.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.1.labels.txt", "temp\\lizard.nes.3.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.2.labels.txt", "temp\\lizard.nes.4.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.2.labels.txt", "temp\\lizard.nes.5.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.3.labels.txt", "temp\\lizard.nes.6.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.3.labels.txt", "temp\\lizard.nes.7.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.4.labels.txt", "temp\\lizard.nes.8.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.4.labels.txt", "temp\\lizard.nes.9.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.5.labels.txt", "temp\\lizard.nes.A.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.5.labels.txt", "temp\\lizard.nes.B.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.6.labels.txt", "temp\\lizard.nes.C.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.6.labels.txt", "temp\\lizard.nes.D.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.7.labels.txt", "temp\\lizard.nes.E.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.7.labels.txt", "temp\\lizard.nes.F.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.8.labels.txt", "temp\\lizard.nes.10.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.8.labels.txt", "temp\\lizard.nes.11.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.9.labels.txt", "temp\\lizard.nes.12.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.9.labels.txt", "temp\\lizard.nes.13.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.A.labels.txt", "temp\\lizard.nes.14.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.A.labels.txt", "temp\\lizard.nes.15.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.B.labels.txt", "temp\\lizard.nes.16.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.B.labels.txt", "temp\\lizard.nes.17.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.C.labels.txt", "temp\\lizard.nes.18.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.C.labels.txt", "temp\\lizard.nes.19.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.D.labels.txt", "temp\\lizard.nes.1A.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.D.labels.txt", "temp\\lizard.nes.1B.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.E.labels.txt", "temp\\lizard.nes.1C.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.E.labels.txt", "temp\\lizard.nes.1D.nl", 0xC000, 0xFFFF)
    label_to_nl("temp\\nes.F.labels.txt", "temp\\lizard.nes.1E.nl", 0x8000, 0xBFFF)
    label_to_nl("temp\\nes.F.labels.txt", "temp\\lizard.nes.1F.nl", 0xC000, 0xFFFF)
    label_to_wch("temp\\nes.0.labels.txt", "temp\\lizard.wch", 0x0000, 0x07FF)
    label_to_stats("temp\\nes.F.labels.txt", "temp\\lizard.nes.F.stats.txt", 0x8000, 0xFFFF)
    label_to_stats("temp\\nes.E.labels.txt", "temp\\lizard.nes.E.stats.txt", 0x8000, 0xFFFF)
    label_to_stats("temp\\nes.D.labels.txt", "temp\\lizard.nes.D.stats.txt", 0x8000, 0xFFFF)
    lua_generate("temp\\nes.",".labels.txt","temp\\lizard_addr.lua")
    # space.txt lists unused space
    bank_type = [
        "room0", "room1", "room2", "room3", "room4", "room5", "room6", "room7", "room8", "room9",
        "chr1", "chr0", "music", "dogs1", "dogs0", "main" ]
    spaces = "unused space:\n"
    space_total = 0
    space_room = 0
    for i in range(0,16):
        space = map_to_segment("temp\\%X.map.txt" % i,"FIXED")[2]
        spaces += " %5s - bank %X: %d bytes\n" % (bank_type[i],i,space)
        if (i < 0xA):
            space_room += space
        space_total += space
    spaces += "\nroom space: %d bytes\n" % space_room
    spaces += "total free: %d bytes\n\n" % space_total
    spaces += "TEXT: %d bytes\n" % (map_to_segment("temp\\F.map.txt","ALIGNED")[2])
    spaces += "FIXED: %d bytes\n" % (map_to_segment("temp\\F.map.txt","VECTORS")[2])
    spaces += "ZEROPAGE: %d bytes\n" % (0xFF - map_to_segment("temp\\F.map.txt","ZEROPAGE")[1])
    spaces += "RAM: %d bytes\n" % (0x7FF - map_to_segment("temp\\F.map.txt","RAM")[1])
    spaces += "\nUsed: %5.2f%%\n" % (100.0 * ((512 * 1024) - space_total) / (512 * 1024))
    open("temp\\space.txt","wt").write(spaces)
