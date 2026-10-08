#!/usr/bin/env python3 -I
"""Fan-triangulate OBJ n-gons (raylib's tinyobj loader caps polygon size). Usage: triangulate_obj.py in.obj out.obj"""
import sys

def main(src, dst):
    out = []
    for line in open(src, encoding="utf8"):
        if line.startswith("f "):
            idx = line.split()[1:]
            for i in range(1, len(idx) - 1):
                out.append("f %s %s %s\n" % (idx[0], idx[i], idx[i + 1]))
        else:
            out.append(line)
    open(dst, "w", encoding="utf8").writelines(out)

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
