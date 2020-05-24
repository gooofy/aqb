#!/bin/env python3

#
# _very_ crude converter amiga os include -> AQB
#

import re
import sys

in_type = False

for line in sys.stdin:

    if not line:
        continue

    if not in_type:
        # struct Menu
        g = re.match(r'^\s*struct\s+([A-Za-z_]+)', line)
        if g:
            print ("TYPE %s" % g.group(1))
            in_type = True
            continue

        # #define MENUENABLED 0x0001  /* whether or not this menu is enabled */
        g = re.match(r'^\#define\s*([A-Za-z_]+)\s+0x([0-9A-Fa-f]+)', line)
        if g:
            print ("CONST AS ? %s = &H%s" % (g.group(1), g.group(2)))
            continue

        # #define foo 0001  /* whether or not this menu is enabled */
        g = re.match(r'^\#define\s*([A-Za-z_]+)\s+([0-9]+)', line)
        if g:
            print ("CONST AS ? %s = %s" % (g.group(1), g.group(2)))
            continue

    if in_type:
        #};
        g = re.match(r'^\s*}\s*;', line)
        if g:
            print ("END TYPE")
            in_type = False
            continue

        #     struct Menu *NextMenu;  /* same level */
        g = re.match(r'^\s*struct\s+([A-Za-z_]+)\s+(\*?)\s*([^;]+)', line)
        if g:
            print ("    AS %s %s    %s" % (g.group(1), g.group(2).replace('*','PTR'), g.group(3)))
            continue

        #     WORD LeftEdge, TopEdge; /* position of the select box */
        g = re.match(r'^\s+([A-Za-z_]+)\s+(\*?)\s*([^;]+)', line)
        if g:
            print ("    AS %s %s    %s" % (g.group(1).replace('WORD', 'INTEGER'), g.group(2).replace('*','PTR'), g.group(3)))
            continue

    print ("REM %s" % line.strip())

