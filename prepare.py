import os
import configparser
from urllib.request import urlopen
from shutil import copyfile
import zipfile

print(">> Updating submodules...")
os.system("git submodule update --init --recursive")
os.system("git submodule update")

config = configparser.ConfigParser()


if not os.path.isfile("make/config.ini"):
	config.add_section("local")
	config.set("local", "vst-x86-output", "C:/Audio/VSTx86")
	config.set("local", "vst-x64-output", "C:/Audio/VSTx64")
	
	with open("make/config.ini", "w") as f:
		config.write(f, True)
	print(">> Creating default configuration...")
else:
	print(">> Reading configuration...")

print(">> Setting up symlinks...")

for filename in os.listdir("projects"):
	path = os.path.join("projects", filename)
	if os.path.isdir(path):
		for subfilename in os.listdir(path):
			if subfilename == "JuceLibraryCode":
				subpath = os.path.join(path, subfilename)
				if os.path.isdir(subpath):
					moduleFolder = os.path.join(subpath, "modules")
					if not os.path.isdir(moduleFolder):
						if os.name == "nt":
							print("Creating junction for: " + moduleFolder)
							os.system("mklink /J \"" + moduleFolder + "\" \"external/JuceLibraryCode/modules")
						else:
							print("Creating symlinkk for: " + moduleFolder)
							os.symlink(moduleFolder, "external/JuceLibraryCode/modules")


print(">> Setting up skeletons...")
print(">> Downloading latest cpp-jit binaries...")

latest_cppjit = urlopen("https://bitbucket.org/Mayae/cppjit/downloads/libCppJit-0.1-windows.zip")
with open("temp.zip", "wb") as out:
	out.write(latest_cppjit.read())

print(">> Extracting cpp-jit...")

with zipfile.ZipFile("temp.zip") as cppjit:
	with cppjit.open("libCppJit.dll") as dll:
		with open(os.path.join("make", "skeleton", "compilers", "CppAPE", "libCppJit.dll"), "wb") as outdll:
		   outdll.write(dll.read())

	with cppjit.open("libCppJit.lib") as lib:
		with open(os.path.join("projects", "cppape", "builds", "VisualStudio", "libCppJit.lib"), "wb") as outlib:
		   outlib.write(lib.read())

if os.path.exists("temp.zip"):
	os.remove("temp.zip")

print(">> Dev environment setup without errors.")