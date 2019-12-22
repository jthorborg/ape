import os
import configparser
import sys
import shutil as sh
import distutils.dir_util as du
import common

output = sys.argv[1]

if os.path.splitext(output)[1] != ".component":
    print("error: not a component")
    exit(-1)

resources = os.path.join(output, "Contents", "Resources")

# build skeleton

common.external_to_skeleton(resources, True)
# copy build files


