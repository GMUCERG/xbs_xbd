#! /usr/bin/env python3
import os
import hashlib
import collections
import binascii
import re
import sys

def dirchecksum(path):
    """Replicate `cd "$dir"; find  -not \( -name .svn -prune \) -and -not -iname "*~" -and -type f -exec sha256sum -t {} \; | sort -k 2 -f -d | sha256sum -t | cut -d " " -f 1`"""
    curpath = os.getcwd()
    os.chdir(path)

    paths = []
    sums = {}
    sum_strings = []
    for i in os.walk("."):
        for j in i[2]:
            paths += (os.path.join(i[0],j),)

    for fn in paths:
        aname = re.sub(r'[\W_]+', '', fn).lower()
        with open(fn,mode='rb') as f:
            h = hashlib.sha256()
            block = f.read(1024*1024)
            while len(block) != 0:
                h.update(block)
                block = f.read(1024*1024)
            
            sums[aname] = (fn,h.hexdigest())

    sums = collections.OrderedDict(sorted(sums.items()))
    for i in sums.values():
        sum_strings += (i[1],"  ", i[0], "\n")

    #print(''.join(sum_strings))
    h = hashlib.sha256()
    h.update(''.join(sum_strings).encode())

    os.chdir(curpath)

    return h.hexdigest() #binascii.hexlify(h.digest())



if __name__ == "__main__":
    print(dirchecksum(sys.argv[1]))
