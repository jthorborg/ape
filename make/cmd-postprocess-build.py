import os
import configparser
import sys
import shutil as sh
import distutils.dir_util as du

config = configparser.ConfigParser()
temp_output = "temp"

# check configurations

if not os.path.isfile("config.ini"):
	print(">> Error: Run python prepare.py firstly")
	exit(-1)


config.read("config.ini")

def cleanup():
	if os.path.isdir(temp_output):
		sh.rmtree(temp_output)
	return

def dirlink(source, dest):
	if os.name == "nt":
		print("Creating junction for: " + dest)
		os.system("mklink /J \"" + dest + "\" \"" + source + "\"")
	else:
		print("Creating symlink for: " + dest)
		os.symlink(dest, source)

# check build output

if not len(sys.argv) == 4:
	print(">> Invalid number of post processing arguments")
	exit(-1)

# TODO: query from args.
release = False

output_dir = sys.argv[1]
platform_agnostic_dir = sys.argv[3]

for file in os.listdir(output_dir):
    if file.endswith(".dll"):
        print(os.path.join(output_dir, file))

# cleanup earlier stuff
cleanup()

# build skeleton


sh.copytree("skeleton", temp_output)
sh.copytree("../external/ccore/include", os.path.join(temp_output, "includes", "ccore"))
sh.copytree("../external/libcxx/include", os.path.join(temp_output, "includes", "libcxx"))
sh.copytree("../external/libcxx/src", os.path.join(temp_output, "compilers", "CppAPE", "runtime", "libcxx-src"))
# TODO: copy presets from /external/signalizer/make/presets that match *oscilloscope*
# du.copy_tree("../external/tinycc/include", os.path.join(temp_output, "includes", "tcc"))
du.copy_tree("../shared-src", os.path.join(temp_output, "includes", "shared-src"))


examples_output = "examples" if release else "examples-release"
sh.copytree("../external/ape-snippets", os.path.join(temp_output, examples_output), ignore = sh.ignore_patterns("*.md", "*.git"))

# copy build files

sh.copyfile(os.path.join(output_dir, "Audio Programming Environment.dll"), os.path.join(temp_output, "Audio Programming Environment.dll"))
sh.copyfile(os.path.join(output_dir, "syswrap.dll"), os.path.join(temp_output, "compilers", "syswrap", "syswrap.dll"))
sh.copyfile(os.path.join(output_dir, "Tcc4APE.dll"), os.path.join(temp_output, "compilers", "Tcc4APE", "Tcc4APE.dll"))
sh.copyfile(os.path.join(output_dir, "CppAPE.dll"), os.path.join(temp_output, "compilers", "CppAPE", "CppAPE.dll"))

# copy into vst folder

dest_dir = ""

if sys.argv[2] == "Win32":
	dest_dir = config.get("local", "vst-x86-output")
elif sys.argv[2] == "x64":
	dest_dir = config.get("local", "vst-x64-output")
else:
	print("Unknown binary target: " + sys.argv[1])
	cleanup()
	exit(1)

dest_dir = os.path.join(dest_dir, "Audio Programming Environment")

print("\nInstalling into: " + dest_dir)

du.copy_tree(temp_output, dest_dir, not release)

if not release and not os.path.isdir(os.path.join(dest_dir, "examples")):
	dirlink("../external/ape-snippets", os.path.join(dest_dir, "examples"))

cleanup()

print("Postprocess finished succesfully.\n")