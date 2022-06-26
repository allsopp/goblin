#!/usr/bin/env python3

import filecmp
import os
import subprocess
import sys

def compare(a, b):
    rs = filecmp.cmp(a, b, shallow=False)
    if rs:
        print("files are identical", file=sys.stderr)
    else:
        print("files differ", file=sys.stderr)
        sys.exit(1)

if len(sys.argv) < 2:
    print("usage: {} <binary> <size>".format(sys.argv[0]), file=sys.stderr)
    sys.exit(1)

b = sys.argv[1]
f = "%03d.png" % (int(sys.argv[2]),)

if not os.path.exists(f):
    cmd = "gm convert -size {0}x{0} xc: +noise Random PNG8:{1}".format(sys.argv[2], f)
    rs = os.system(cmd)
    if rs != 0:
        sys.exit(1)

os.system("gm identify {}".format(f))

cmd = "gm convert {0} PNG24:- | gm convert - {0}.tga".format(f)
rs = os.system(cmd)
if rs != 0:
    sys.exit(1)

with open("{}.goblin.tga".format(f), "w") as fp:
    rs = subprocess.run([b, f], stdout=fp)
    if rs.returncode != 0:
        sys.exit(1)

compare("{}.tga".format(f), "{}.goblin.tga".format(f))
