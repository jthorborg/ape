import os
import configparser
import sys
import shutil as sh
import distutils.dir_util as du

def dirlink(source, dest):
	if os.name == "nt":
		print("Creating junction for: " + dest)
		os.system("mklink /J \"" + dest + "\" \"" + source + "\"")
	else:
		print("Creating symlink for: " + dest)
		os.symlink(dest, source)

# build skeleton


# recursively merge two folders including subfolders
# https://lukelogbook.tech/2018/01/25/merging-two-folders-in-python/
def mergefolders(root_src_dir, root_dst_dir):
    for src_dir, dirs, files in os.walk(root_src_dir):
        dst_dir = src_dir.replace(root_src_dir, root_dst_dir, 1)
        if not os.path.exists(dst_dir):
            os.makedirs(dst_dir)
        for file_ in files:
            src_file = os.path.join(src_dir, file_)
            dst_file = os.path.join(dst_dir, file_)
            if os.path.exists(dst_file):
                os.remove(dst_file)
            sh.copy(src_file, dst_dir)

def external_to_skeleton(where, release):
    mergefolders("skeleton", where)
    mergefolders("../external/ccore/include", os.path.join(where, "includes", "ccore"))
    mergefolders("../external/libcxx/include", os.path.join(where, "includes", "libcxx"))
    mergefolders("../external/libcxx/src", os.path.join(where, "compilers", "CppAPE", "runtime", "libcxx-src"))
    # TODO: copy presets from /external/signalizer/make/presets that match *oscilloscope*
    # du.copy_tree("../external/tinycc/include", os.path.join(temp_output, "includes", "tcc"))
    mergefolders("../shared-src", os.path.join(where, "includes", "shared-src"))

    examples_output = "examples" if release else "examples-release"
    sh.copytree("../external/ape-snippets", os.path.join(where, examples_output), ignore = sh.ignore_patterns("*.md", "*.git"))

def symlink_snippets(where):
    if not os.path.isdir(os.path.join(where, "examples")):
        dirlink("../external/ape-snippets", os.path.join(where, "examples"))
