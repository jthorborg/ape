import os
import configparser

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



print(">> Dev environment setup without errors.")