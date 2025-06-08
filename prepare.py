import os
import configparser
from urllib.request import urlopen
from shutil import copyfile
import zipfile
import sys
import platform

print(">> Updating submodules...")
os.system("git submodule update --init --recursive")
os.system("git submodule update")

config = configparser.ConfigParser()

# OS X specific

if sys.platform == "darwin":
    vsymlink = os.path.join("projects", "plugin", "JuceLibraryCode", "version.h")
    vsymsource = os.path.join("projects", "plugin", "src", "version.h")
    if not os.path.isfile(vsymlink):
        os.symlink(os.path.abspath(vsymsource), os.path.abspath(vsymlink))

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

def dirlink(source, dest):
	if os.name == "nt":
		print("Creating junction for: " + dest)
		os.system("mklink /J \"" + dest + "\" \"" + source + "\"")
	else:
		print("Creating symlink for: " + dest)
		os.symlink(os.path.abspath(source), os.path.abspath(dest))

for filename in os.listdir("projects"):
	path = os.path.join("projects", filename)
	if os.path.isdir(path):
		for subfilename in os.listdir(path):
			if subfilename == "JuceLibraryCode":
				subpath = os.path.join(path, subfilename)
				if os.path.isdir(subpath):
					moduleFolder = os.path.join(subpath, "modules")
					if not os.path.isdir(moduleFolder):
						dirlink("external/JuceLibraryCode/modules", moduleFolder)

print(">> Setting up skeletons...")
print(">> Downloading latest cpp-jit binaries...")

if platform.system() == "Windows":
	cppjit_url = "https://github.com/jthorborg/cppjit/releases/download/v0.5.0/libCppJit-0.5-windows.zip"
elif platform.system() == "Darwin":
	cppjit_url = "https://github.com/jthorborg/cppjit/releases/download/v0.5.0/libCppJit-0.5-macos.zip"

latest_cppjit = urlopen(cppjit_url)
with open("temp.zip", "wb") as out:
	out.write(latest_cppjit.read())

print(">> Extracting cpp-jit...")

with zipfile.ZipFile("temp.zip") as cppjit:
    
	if platform.system() == "Windows":

		with cppjit.open("libCppJit.dll") as dll:
			with open(os.path.join("make", "skeleton", "compilers", "CppAPE", "libCppJit.dll"), "wb") as outdll:
				outdll.write(dll.read())

		with cppjit.open("libCppJit.lib") as lib:
			with open(os.path.join("projects", "cppape", "builds", "VisualStudio", "libCppJit.lib"), "wb") as outlib:
				outlib.write(lib.read())

	elif platform.system() == "Darwin":
        
		with cppjit.open("libcppjit.dylib") as dylib:
			with open(os.path.join("make", "skeleton", "compilers", "CppAPE", "libcppjit.dylib"), "wb") as outdyld:
				outdyld.write(dylib.read())

	with cppjit.open("libCppJit.h") as header:
		with open(os.path.join("projects", "cppape", "src", "libCppJit.h"), "wb") as outheader:
			outheader.write(header.read())

if os.path.exists("temp.zip"):
	os.remove("temp.zip")

print(">> Dev environment setup without errors.")
