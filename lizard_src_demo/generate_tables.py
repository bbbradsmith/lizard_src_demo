#!/usr/bin/env python3
import sys
assert sys.version_info[0] == 3, "Python 3 required."

# Lizard
# Copyright Brad Smith 2018
# http://lizardnes.com

import math

def table_cpp(n,t):
    s = "signed char " + n + "[" + str(len(t)) + "] = {"
    for i in range(0,len(t)):
        if (i & 15) == 0:
            s += "\n\t"
        s += "%4d," % t[i]
    s += "\n"
    s += ("};\n")
    print(s)


def table_asm(n,t):
    s = n + ":"
    for i in range(0,len(t)):
        if (i & 15) == 0:
            s += "\n.byte "
        s += "%4d" % t[i]
        if (i & 15) != 15 and (i != (len(t)-1)):
            s += ","
    s += "\n"
    print(s)

def circle4_gen(x):
    r = x * math.pi / 128
    v = 3.8 * math.sin(r)
    return math.floor(v + 0.5)

def circle32_gen(x):
    r = x * math.pi / 128
    v = 31.55 * math.sin(r)
    return math.floor(v + 0.5)

def circle108_gen(x):
    r = x * math.pi / 128
    v = 107.55 * math.sin(r)
    return math.floor(v + 0.5)

circle32 = [circle32_gen(x) for x in range(0,256) ]
table_cpp("circle32",circle32)
table_asm("circle32",circle32)
symmetry32 = [circle32[x] + circle32[x+128] for x in range(0,128)]
print (str(symmetry32)+" min "+str(min(symmetry32))+" max "+str(max(symmetry32)))
print ("")

circle108 = [circle108_gen(x) for x in range(0,256) ]
table_cpp("circle108",circle108)
table_asm("circle108",circle108)
symmetry108 = [circle108[x] + circle108[x+128] for x in range(0,128)]
print (str(symmetry108)+" min "+str(min(symmetry108))+" max "+str(max(symmetry108)))
print ("")

circle4 = [circle4_gen(x) for x in range(0,256) ]
table_cpp("circle4",circle4)
table_asm("circle4",circle4)
symmetry4 = [circle4[x] + circle4[x+128] for x in range(0,128)]
print (str(symmetry4)+" min "+str(min(symmetry4))+" max "+str(max(symmetry4)))
print ("")
