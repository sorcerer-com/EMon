import gzip
import os
import shutil
import sys


def compress(filepath):
    gzFilepath = filepath + ".gz"
    print("compress - %s ->     %s" % (filepath.ljust(40), gzFilepath))
    with open(filepath, "rb") as f_in:
        with gzip.open(gzFilepath, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)


def decompress(gzFilepath):
    filepath = os.path.splitext(gzFilepath)[0]
    print("decompress - %s ->     %s" % (gzFilepath.ljust(40), filepath))
    with gzip.open(gzFilepath, "rb") as f_in:
        with open(filepath, "wb") as f_out:
            shutil.copyfileobj(f_in, f_out)


path = "..\\data"
if len(sys.argv) > 1:
    path = sys.argv[1]

for root, directories, filenames in os.walk(path):
    for filename in filenames:
        filepath = os.path.join(root, filename)
        if os.path.splitext(filename)[1] != ".gz":
            compress(filepath)
        else:
            decompress(filepath)
        os.remove(filepath)
