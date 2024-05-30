import sys
import shutil
import os
import json
import zipfile

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	version = data['version']
except:
	version = '0.0.0'

print("Version " + version)

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	project = data['project']
except:
	project = data['sketch']

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	sketch = data['sketch']
except:
	sketch = 'firmware'

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	output = data['output']
except:
	output = 'build'


print("Version " + version)

# Check if the destination directory exists
if not os.path.exists('./'+output): 
    os.makedirs('./'+output) 


# Specify the source file, the destination for the copy, and the new name
source_file = './'+output+'/'+sketch+'.hex'
destination_directory = './generated/'
new_file_name = project+'_V'+version+'.hex'
new_zip_name = project+'_V'+version+'.zip'

if os.path.isfile(destination_directory+new_file_name):
	try:
		os.remove(destination_directory+new_file_name)
	except:
		print('Cannot delete '+destination_directory+new_file_name)
	# finally:
	# 	print('Delete '+destination_directory+new_file_name)

if os.path.isfile(destination_directory+new_zip_name):
	try:
		os.remove(destination_directory+new_zip_name)
	except:
		print('Cannot delete '+destination_directory+new_zip_name)
	# finally:
	# 	print('Delete '+destination_directory+new_zip_name)
		
if os.path.isfile('./'+output+'/'+new_zip_name):
	try:
		os.remove('./'+output+'/'+new_zip_name)
	except:
		print('Cannot delete '+'./'+output+'/'+new_zip_name)
	# finally:
	# 	print('Delete '+'./'+output+'/'+new_zip_name)

if os.path.isfile('./generated/'+new_zip_name):
	try:
		os.remove('./generated/'+new_zip_name)
	except:
		print('Cannot delete '+'./generated/'+new_zip_name)
	# finally:
	# 	print('Delete '+'./generated/'+new_zip_name)

# Check if the destination directory exists
if not os.path.exists('./generated/'): 
    os.makedirs('./generated/') 

# Copy the files
try:
	shutil.copy2(source_file, destination_directory)
except:
	print('Cannot copy '+source_file +' to '+destination_directory)
# Get the base name of the source file
base_name = os.path.basename(source_file)

# print("Base name " + base_name)

# Construct the paths to the copied file and the new file name
copied_file = os.path.join(destination_directory, base_name)
new_file = os.path.join(destination_directory, new_file_name)
bin_name = sketch+'.bin'
zip_name = project+'_V'+version+'.zip'

# print("Copied file " + copied_file)
# print("Base name " + new_file)
# print("ZIP name " + zip_name)
# print("ZIP content " + bin_name) 

# Create ZIP file for WisToolBox
try:
	os.chdir("./build")
except:
	print('Cannot change dir to ./build')

try:
	zipfile.ZipFile(zip_name, mode='w').write(bin_name)
except:
	print('Cannot zip '+bin_name +' to '+zip_name)

os.chdir("../")

try:
	shutil.copy2("./build/"+zip_name, destination_directory)
except:
	print('Cannot copy '+"./build/"+zip_name +' to '+destination_directory)

# Rename the file
try:
	os.rename(copied_file, new_file)
except:
	print('Cannot rename '+copied_file +' to '+new_file)

