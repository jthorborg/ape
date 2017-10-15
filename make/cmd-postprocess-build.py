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


# check build output

if not len(sys.argv) == 4:
	print(">> Invalid number of post processing arguments")
	exit(-1)


output_dir = sys.argv[1]
platform_agnostic_dir = sys.argv[3]

for file in os.listdir(output_dir):
    if file.endswith(".dll"):
        print(os.path.join(output_dir, file))

# cleanup earlier stuff
cleanup()

# build skeleton


sh.copytree("skeleton", temp_output)
sh.copytree("../external/tinycc/win32/include", os.path.join(temp_output, "includes", "tcc"))
du.copy_tree("../external/tinycc/include", os.path.join(temp_output, "includes", "tcc"))
sh.copytree("../external/ape-snippets", os.path.join(temp_output, "examples"), ignore = sh.ignore_patterns("*.md", "*.git"))

# copy build files

sh.copyfile(os.path.join(output_dir, "Audio Programming Environment.dll"), os.path.join(temp_output, "Audio Programming Environment.dll"))
sh.copyfile(os.path.join(output_dir, "syswrap.dll"), os.path.join(temp_output, "compilers", "syswrap", "syswrap.dll"))
sh.copyfile(os.path.join(output_dir, "Tcc4APE.dll"), os.path.join(temp_output, "compilers", "Tcc4APE", "Tcc4APE.dll"))
sh.copyfile(os.path.join(output_dir, "CppAPE.dll"), os.path.join(temp_output, "compilers", "CppAPE", "CppAPE.dll"))
sh.copyfile(os.path.join(platform_agnostic_dir, "cfront.exe"), os.path.join(temp_output, "compilers", "CppAPE", "cfront.exe"))

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

du.copy_tree(temp_output, dest_dir)

cleanup()

print("Postprocess finished succesfully.\n")