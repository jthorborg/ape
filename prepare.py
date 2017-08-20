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


print(">> Dev environment setup without errors.")