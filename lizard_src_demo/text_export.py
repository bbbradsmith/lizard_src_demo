#!/usr/bin/env python3
import sys
assert sys.version_info[0] == 3, "Python 3 required."

# Lizard
# Copyright Brad Smith 2018
# http://lizardnes.com

import os
import datetime
import enum

ASSETS_DIR = "assets"
INPUT_DIR = os.path.join(ASSETS_DIR,"src_text")
OUTPUT_DIR = os.path.join(ASSETS_DIR,"export")
EXPORT_ALL = False

TEXT_RESERVED = 0x2800 # see ALIGNED start in bank32k.cfg

now_string = datetime.datetime.now().strftime("%a %b %d %H:%M:%S %Y")

glyphs_empty = { 0xC0, 0xED, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC } # empties
glyphs_alpha = { x for x in range(0xC1,0xDB) } # A-Z
glyphs_sym   = { 0xDB, 0xDC, 0xDD, 0xDE, 0xDF, 0xEA, 0xEB, 0xEE } # !/.:'?,-
glyphs_allowed = glyphs_empty | glyphs_alpha | glyphs_sym

base_set = {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
    '0','1','2','3','4','5','6','7','8','9',
    ' ','!','/','.',':',"'",'?',',','-','$','_',';' }

if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

def debug(s):
    #print(s)
    pass

class LoadTextException(Exception):
    pass

class TextSet:
    def __init__(self,id,name,meta_index,text_enums,texts,glyphs):
        self.id = id
        self.name = name
        self.meta_index = meta_index
        self.text_enums = text_enums
        self.texts = texts
        self.glyphs = glyphs
    def __lt__(self,other):
        return self.name > other.name # alphabetic sort

def load_text(filename):
    print("Parsing: " + filename)
    try:
        f = open(filename,"rt",encoding="utf-8-sig")
    except Exception as e:
        raise LoadTextException('Unable to open file "%s"' % filename)
    class Mode(enum.Enum):
        FREE = 0
        ID = 1
        MAP = 2
        MAP_REPLACEMENT = 3
        TEXT_ENUM = 4
        TEXT = 5
        GLYPH_ENUM = 6
        GLYPH = 7
    mode = Mode.FREE
    id = -1
    name = None
    maps = []
    text_enums = []
    texts = []
    glyphs = []
    meta_index = -1
    line_count = 0
    for line in f.readlines():
        line_count += 1
        line = line.rstrip() # remove newline and any trailing whitespace
        line = line.lstrip() # remove leading whitespace
        debug("line %d: %s" % (line_count, line))
        while len(line) > 0:
            if line[0] == '#':
                # comment
                debug("Comment (line %d)" % line_count)
                break
            if line[0] == '"':
                # string (could be adding to a TEXT or MAP)
                qend = line.find('"',1)
                if qend < 0:
                    raise LoadTextException('Quote " without pair at line %d.' % line_count)
                s = line[1:qend]
                line = line[qend+1:].lstrip()
                debug('STRING: "%s" (line %d)' % (s,line_count))
                if mode == Mode.TEXT:
                    sunmapped = s
                    # validate characters
                    sv = s
                    for m in maps:
                        sv = sv.replace(m[0],"☂")
                    for c in sv:
                        if c != '☂' and c not in base_set:
                            raise LoadTextException("Out-of-gamut character '%c' (%02X) at line %d." % (c,ord(c),line_count))
                    # create the map
                    for m in maps:
                        s = s.replace(m[0],m[1])
                    if len(s) > 32:
                        raise LoadTextException("Maximum TEXT string length is 32. Exceeded at line %d." % line_count)
                    s = s.rstrip() # remove trailing space
                    i = len(texts)-1
                    texts[i].append(s)
                    tl = len(texts[i])
                    if tl > 1:
                        if len(texts[i][tl-2]) >= 32:
                            raise LoadTextException("Previous line must be < 32 characters long to accomodate newline at line %d." % line_count)
                        texts[i][tl-2] = texts[i][tl-2] + "$" # newline continuation
                    debug('%s append string: "%s" (line %d)' % (text_enums[i],s,line_count))
                    # if name
                    if text_enums[i] == "OPT_CURRENT_LANGUAGE":
                        name = sunmapped.rstrip()
                elif mode == Mode.MAP:
                    maps[len(maps)-1] = (s,"")
                    mode = Mode.MAP_REPLACEMENT
                    debug('MAP %d = "%s" (line %d)' % (len(maps)-1,s,line_count))
                    for c in s:
                        if c == '☂':
                            raise LoadTextException("Character ☂ has special internal use and is not allowed at line %d." % line_count)
                elif mode == Mode.MAP_REPLACEMENT:
                    i = len(maps)-1
                    maps[i] = (maps[i][0], maps[i][1] + s)
                    for c in s:
                        if c == '☂':
                            raise LoadTextException("Character ☂ has special internal use and is not allowed at line %d." % line_count)
                    debug('MAP "%s" -> "%s" (line %d)' % (maps[i][0],maps[i][1],line_count))
                elif mode == Mode.GLYPH:
                    if len(s) != 8:
                        raise LoadTextException("Glyph string must be 8 characters long at line %d." % line_count)
                    b = 0
                    bm = 0x80
                    for i in range(0,8):
                        c = s[i]
                        if c not in {' ','.'}:
                            b |= bm
                        bm >>= 1
                    i = len(glyphs)-1
                    glyphs[i].append(b)
                    if len(glyphs[i]) > 9:
                        raise LoadTextException("Glyph has too many rows (8 expected) at line %d." % line_count)
                    pass
                else:
                    raise LoadTextException("Unexpected string at line %d." % line_count)
                continue
            # not a string or comment, treat it as a single whitespace separated token
            t = line.split()[0]
            line = line[len(t):].lstrip()
            if t[0] == '$' or (t[0] >= '0' and t[0] <= '9'):
                # number, could be for ID or MAP_REPLACEMENT
                base = 10
                if t[0] == '$':
                    base = 16
                    t = t[1:]
                try:
                    v = int(t,base)
                except:
                    raise LoadTextException("Invalid number at line %d." % line_count)
                debug("Number: $%X (line %d)" % (v,line_count))
                if mode == Mode.ID:
                    id = v
                    if v < 0 or v > 255:
                        raise LoadTextException("Invalid ID at line %d." % line_count)
                    debug("ID: %d (line %d)" % (id,line_count))
                    mode = Mode.FREE
                elif mode == Mode.MAP_REPLACEMENT:
                    i = len(maps)-1
                    maps[i] = (maps[i][0], maps[i][1] + chr(v))
                    debug('MAP "%s" -> "%s" (line %d)' % (maps[i][0],maps[i][1],line_count))
                elif mode == Mode.GLYPH_ENUM:
                    if v not in glyphs_allowed:
                        raise LoadTextException("Glyph $%02X not allowed at line %d." % (v,line_count))
                    if v in glyphs_sym:
                        base_set.remove(chr(v))
                    if v in glyphs_alpha:
                        base_set.remove(chr(v).upper())
                        base_set.remove(chr(v).lower())
                    for b in glyphs:
                        if b[0] == v:
                           raise LoadTextException("Glyph $%02X already used before line %d." % (v,line_count))
                    g = bytearray([v])
                    glyphs.append(g)
                    mode = Mode.GLYPH
                else:
                    raise LoadTextException("Unexpected number at line %d." % line_count)
                continue
            if \
                mode != Mode.FREE and \
                mode != Mode.MAP_REPLACEMENT and \
                mode != Mode.TEXT_ENUM and \
                mode != Mode.TEXT and \
                mode != Mode.GLYPH:
                raise LoadTextException("Unexpected token %s at line %d." % (t,line_count))
            elif t.upper() == 'ID':
                mode = Mode.ID
                debug("ID (line %d)" % line_count)
                continue
            elif t.upper() == 'MAP':
                mode = Mode.MAP
                maps.append(None)
                debug("MAP %d (line %d)" % (len(maps)-1,line_count))
                continue
            elif t.upper() == 'TEXT':
                mode = Mode.TEXT_ENUM
                debug("TEXT %d (line %d)" % (len(maps)-1,line_count))
                continue
            elif t.upper() == 'META':
                meta_index = len(texts)
                mode = Mode.FREE
                debug("META (line %d)" % line_count)
                continue
            elif t.upper() == 'GLYPH':
                mode = Mode.GLYPH_ENUM
                debug("GLYPH (line %d)" % line_count)
                continue
            elif mode == Mode.TEXT_ENUM:
                text_enums.append(t.upper())
                texts.append([])
                for i in range(0,len(text_enums)-1):
                    if text_enums[i] == t.upper():
                        raise LoadTextException("Duplicate text %s at line %d." % (t,line_count))
                mode = Mode.TEXT
                assert len(text_enums) == len(texts), "text_enums/texts mismatch!"
                debug("TEXT %d = %s (line %d)" % (len(text_enums)-1,t,line_count))
                continue
            else:
                raise LoadTextException("Unexpected token %s at line %d." % (t,line_count))
    try:
        ni = text_enums.index("OPT_CURRENT_LANGUAGE")
        if len(texts[ni]) != 1:
            raise LoadTextException("OPT_CURRENT_LANGUAGE must have exactly one string.")
    except:
        raise LoadTextException("TEXT OPT_CURRENT_LANGUAGE not found.")
    glyph_size = 1
    for g in glyphs:
        if len(g) != 9:
            LoadTextException("Glyph $%02X unfinished. 8 lines expected, got: %d." % (g[0],len(g)))
        glyph_size += len(g)
    if glyph_size >= 256:
        LoadTextException("Too much glyph data (%d bytes). Maximum 256 bytes." % glyph_size)
    debug("Output:")
    debug("ID: %d" % id)
    debug("meta_index: %d" % meta_index)
    debug("texts: %d" % len(texts))
    debug("glyhps: %d" % len(glyphs))
    assert len(text_enums) == len(texts), "text_enums/texts mismatch!"
    for i in range(0,len(texts)):
        debug("text %d: %s" % (i,text_enums[i]))
        for l in texts[i]:
            debug('"%s"' % l)
    return TextSet(id,name,meta_index,text_enums,texts,glyphs)

class ExportTextException(Exception):
    pass

def character_print(c):
    a = ord(c)
    return (a >= 32 and a <= 126)

def character_hex(c):
    return (c >= 'A' and c <= 'F') or (c >= 'a' and c <= 'f') or (c >= '0' and c <= '9')

def export_text_set(ts):
    if ts.meta_index < 0:
        raise ExportTextException("META statement missing in text set: %2" % ts.name)
    # generate text set document
    scpp = ""
    scpp += "// automatically generated by text_export.py\n"
    scpp += "// " + now_string + "\n"
    scpp += "\n"
    sasm = ""
    sasm += "; automatically generated by text_export.py\n"
    sasm += "; " + now_string + "\n"
    sasm += "\n"
    # generate CPP text table
    scpp += '#include "../../lizard_game.h"\n'
    scpp += '#include "text_set.h"\n'
    scpp += "\n"
    scpp += "namespace Game {\n"
    scpp += "\n"
    cpp_name = ts.name.lower()
    if ts.id == 0:
        cpp_name = "default"
    scpp += "extern const char* const text_table_" + cpp_name + "[TEXT_COUNT_META] = {\n"
    for i in range(len(ts.texts)):
        enum = ts.text_enums[i]
        lines = ts.texts[i]
        if i == ts.meta_index:
            scpp += "\t// META\n"
        if (len(lines) < 1):
            raise ExportTextException("TEXT %s has no strings in text set: %s" % (enum,ts.name))
        for j in range(len(lines)):
            l = ""
            last_special = False
            for c in lines[j]:
                if character_print(c):
                    if last_special and character_hex(c):
                        l += '""'
                    l += c
                    last_special = False
                else:
                    l += "\\x%02X" % ord(c)
                    last_special = True
            ln = '\t"' + l + '"'
            if j == (len(lines)-1):
                ln += "," # last line needs comma
            if j == 0:
                ln = "%-40s // %s" % (ln,enum) # first line gets enum in comment
            scpp += ln + "\n"
    scpp += "};\n"
    scpp += "\n"
    scpp += "extern const uint8 text_glyphs_default[] = {\n"
    for g in ts.glyphs:
        scpp += "\t0x%02X" % g[0]
        for b in g[1:]:
            scpp += ", 0x%02X" % b
        scpp +=",\n"
    scpp += "\t0\n"
    scpp += "};\n"
    scpp += "\n"
    scpp += "} // namespace Game\n"
    scpp += "\n"
    # generate ASM text table
    sasm += "; Language: " + ts.name + "\n"
    #sasm += ".export text_table_low\n"
    #sasm += ".export text_table_high\n"
    sasm += "\n"
    sasm += "text_data_location:\n"
    sasm += ".addr text_table_low\n"
    sasm += ".addr text_table_high\n"
    sasm += ".addr text_glyphs\n"
    sasm += '.assert text_data_location=$8000,error,"text_data_location must be placed at $8000"\n'
    sasm += "\n"
    sasm_ttl = "text_table_low:\n"
    sasm_tth = "text_table_high:\n"
    for i in range(ts.meta_index):
        enum = ts.text_enums[i]       
        lines = ts.texts[i]
        sasm_ttl += ".byte <text_" + enum + "\n"
        sasm_tth += ".byte >text_" + enum + "\n"
        for j in range(len(lines)):
            l = ""
            line = lines[j]
            last_quoted = False
            for k in range(len(line)):
                c = line[k]
                if character_print(c):
                    if not last_quoted:
                        if k > 0:
                            l += ','
                        l += '"'
                    l += c
                    last_quoted = True
                else:
                    if last_quoted:
                        l += '"'
                    if k > 0:
                        l += ','
                    l += '$%02X' % ord(c)
                    last_quoted = False
            if last_quoted:
                l += '"'
            if j == (len(lines)-1):
                if len(line) > 0:
                    l += ","
                l += "0" # terminating zero
            e = ""
            if j == 0:
                e = "text_" + enum + ":" # first line gets enum
            ln = "%-31s .byte %s" % (e,l)
            sasm += ln + "\n"
    sasm += "\n"
    sasm += sasm_ttl
    sasm += "\n"
    sasm += sasm_tth
    sasm += "\n"
    sasm += "text_glyphs:\n"
    for g in ts.glyphs:
        sasm += ".byte $%02X" % g[0]
        for b in g[1:]:
            sasm += ", $%02X" % b
        sasm += "\n"
    sasm += ".byte 0\n"
    sasm += "\n"
    # PC/Mac binary version
    bin = bytearray()
    bin.append(ts.id)
    for t in ts.texts:
        for l in t:
            for c in l:
                bin.append(ord(c))
        bin.append(0)
    for g in ts.glyphs:
        for b in g:
            bin.append(b)
    bin.append(0)
    # IPS patch version
    # build data
    ips_table_low  = bytearray()
    ips_table_high = bytearray()
    ips_text       = bytearray()
    ips_glyphs     = bytearray()
    ips_text_addr = 0x8000 + 6
    for i in range(ts.meta_index):
        t = ts.texts[i]
        ips_table_high.append((ips_text_addr >>  8) & 0xFF)
        ips_table_low.append( (ips_text_addr >>  0) & 0xFF)
        for l in t:
            for c in l:
                ips_text.append(ord(c))
        ips_text.append(0)
        ips_text_addr = 0x8000 + 6 + len(ips_text)
    for g in ts.glyphs:
        for b in g:
            ips_glyphs.append(b)
    ips_glyphs.append(0)
    # build IPS file
    ips = bytearray()
    ips = ips + bytearray(map(ord,['P','A','T','C','H']))
    ips_addr_patch = (0xF * 0x8000) + 0x10
    ips_size_patch = TEXT_RESERVED
    ips.append((ips_addr_patch >> 16) & 0xFF)
    ips.append((ips_addr_patch >>  8) & 0xFF)
    ips.append((ips_addr_patch >>  0) & 0xFF)
    ips.append((ips_size_patch >>  8) & 0xFF)
    ips.append((ips_size_patch >>  0) & 0xFF)
    # patch data
    ips_addr_table_low  = 0x8000 + 6 + len(ips_text)
    ips_addr_table_high = ips_addr_table_low + len(ips_table_low)
    ips_addr_glyphs     = ips_addr_table_high + len(ips_table_high)
    ips.append((ips_addr_table_low  >>  0) & 0xFF)
    ips.append((ips_addr_table_low  >>  8) & 0xFF)
    ips.append((ips_addr_table_high >>  0) & 0xFF)
    ips.append((ips_addr_table_high >>  8) & 0xFF)
    ips.append((ips_addr_glyphs     >>  0) & 0xFF)
    ips.append((ips_addr_glyphs     >>  8) & 0xFF)
    ips += ips_text
    ips += ips_table_low
    ips += ips_table_high
    ips += ips_glyphs
    # pad with 0s to end of allotted text area
    ips_size_filled = 6 + len(ips_text) + len(ips_table_low) + len(ips_table_high) + len(ips_glyphs)
    ips_size_padded = (ips_size_patch - ips_size_filled)
    if ips_size_padded < 0:
        raise ExportTextException("Text set too large by %d bytes: %s" % (-ips_size_padded,ts.name))
    ips += bytearray([0] * ips_size_padded)
    # end of IPS
    ips += bytearray(map(ord,['E','O','F']))
    # finish and save
    scpp += "// end of file\n"
    sasm += "; end of file\n"
    file_base = "text_" + text_set.name.lower()
    print ("Exporting %15s: %5d bytes, %d free." % (file_base, ips_size_filled, ips_size_padded))
    if ts.id == 0:
        open(os.path.join(OUTPUT_DIR,"text_default.cpp"),"wt").write(scpp)
        open(os.path.join(OUTPUT_DIR,"text_default.s"),"wt").write(sasm)
    if ts.id != 0 or EXPORT_ALL:
        open(os.path.join(OUTPUT_DIR,file_base+".lit"),"wb").write(bin)
        open(os.path.join(OUTPUT_DIR,file_base+".ips"),"wb").write(ips)
    if EXPORT_ALL:
        open(os.path.join(OUTPUT_DIR,file_base+".cpp"),"wt").write(scpp)
        open(os.path.join(OUTPUT_DIR,file_base+".s"),"wt").write(sasm)

def export_text_sets(tss):
    if len(tss) < 1:
        raise ExportTextException("No text sets to export?")
    default_index = -1
    for i in range(0,len(tss)):
        id = tss[i].id
        name = tss[i].name
        if id == 0:
            default_index = i # the default langauge set has ID of 0
        if id < 0:
            raise ExportTextException("Text set %s has no ID value." % name)
        for j in range(0,i):
            if tss[j].id == id:
                raise ExportTextException("ID %d conflict between %s and %s." % (id, name, tss[j].name))
            if tss[j].name.lower() == name.lower():
                #raise ExportTextException("Duplicate text set name: " + name)
                pass # HACK
    if default_index < 0:
        raise ExportTextExecption("Default language (ID 0) not found.")
    # default language
    dts = tss[default_index]
    # sort language list alphabetically
    tss.sort()
    deafult_index = tss.index(dts)
    # generate files
    shpp = ""
    shpp += "#pragma once\n"
    shpp += "// automatically generated by text_export.py\n"
    shpp += "// " + now_string + "\n"
    shpp += "\n"
    stpp = shpp
    sasm = ""
    sasm += "; automatically generated by text_export.py\n"
    sasm += "; " + now_string + "\n"
    sasm += "\n"
    # generate CPP enums
    shpp += "namespace Game {\n"
    shpp += "\n"
    shpp += "enum TextEnum\n"
    shpp += "{\n"
    stpp += '#include "text_set.h"\n'
    stpp += "\n"
    stpp += "const wxChar* TEXT_ENUM_LIST[Game::TEXT_COUNT_META] = {\n"
    for i in range(len(dts.text_enums)):
        e = dts.text_enums[i]
        if i == dts.meta_index:
            shpp += "\n"
            shpp += "\t// META TEXT (Non-NES text: options, etc.)\n"
        shpp += "\tTEXT_" + e.upper() + ",\n"
        stpp += '\twxT("' + e.upper() + '"),\n'
        # verify that all TEXT entries exist in all sets, in the correct order
        for ts in tss:
            if len(ts.text_enums) <= i or ts.text_enums[i] != e:
                raise ExportTextException("Out of order or missing TEXT %s in %s." % (e, ts.name))
    shpp += "\tTEXT_COUNT_META\n"
    shpp += "};\n"
    shpp += "const TextEnum TEXT_COUNT = TEXT_%s;\n" % dts.text_enums[dts.meta_index].upper()
    shpp += "\n"
    shpp += "} // namespace Game\n"
    shpp += "\n"
    stpp += "};\n"
    stpp += "\n"
    # generate ASM enums
    sasm += ".enum\n"
    for i in range(dts.meta_index):
        sasm += "TEXT_" + dts.text_enums[i] + "\n"
    sasm += "TEXT_COUNT\n"
    sasm += ".endenum\n"
    sasm += "\n"
    # finish and save
    shpp += "// end of file\n"
    stpp += "// end of file\n"
    sasm += "; end of file\n"
    file_base = "text_set"
    print ("Exporting default %s: %d entries." % (file_base,len(dts.text_enums)))
    open(os.path.join(OUTPUT_DIR,file_base+".h"),"wt").write(shpp)
    open(os.path.join(OUTPUT_DIR,file_base+"_tool.h"),"wt").write(stpp)
    open(os.path.join(OUTPUT_DIR,file_base+".s"),"wt").write(sasm)   

try:
    text_sets = []
    for filename in os.listdir(INPUT_DIR):
        if (filename.lower().endswith(".txt")):
            full_filename = os.path.join(INPUT_DIR,filename)
            text_set = load_text(full_filename)
            export_text_set(text_set)
            text_sets.append(text_set)
    export_text_sets(text_sets)
except LoadTextException as e:
    print ("Parse error: " + str(e))
    sys.exit(-1)
except ExportTextException as e:
    print("Export error: " + str(e))
    sys.exit(-1)
print ("Done.")
