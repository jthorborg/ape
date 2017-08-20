import os
import configparser
import sys

config = configparser.ConfigParser()

if not os.path.isfile("config.ini"):
	print(">> Error: Run python prepare.py firstly")
	exit(-1)


config.read("config.ini")

if not len(sys.argv) == 3:
	print(">> Invalid number of post processing arguments")

output_dir = sys.argv[1]


print('Number of arguments:', len(sys.argv), 'arguments.')
print('Argument List:', str(sys.argv))