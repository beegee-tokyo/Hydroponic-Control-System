import sys
import shutil
import os
import json
import zipfile

print("+++++++++++++++++++++++++++++++++++++++++++++++++++")
print("Create Generated")
print("+++++++++++++++++++++++++++++++++++++++++++++++++++")

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

is_rak4631 = False

try:
	with open('./.vscode/arduino.json') as f:
		data = json.load(f)
	this_board = data['board']
	found_rak4631 = this_board.find('RAK4631')
	if found_rak4631 != -1:
		is_rak4631 = True
except:
	is_rak4631 = False


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
old_zip_name = sketch+'.zip'
zip_name = project+'_V'+version+'.zip'

print("Copied file " + copied_file)
print("Base name " + new_file)
print("ZIP name " + zip_name)
print("RAK4631 ZIP name " + old_zip_name)
print("ZIP content " + bin_name) 

# Create ZIP file for WisToolBox
try:
	os.chdir("./build")
except:
	print('Cannot change dir to ./build')

if is_rak4631:
	try:
		print('Try RAK4631 file type')
		shutil.copy2(os.path.join('./',old_zip_name), os.path.join('../generated/',zip_name))
	except:
		print('Cannot copy '+old_zip_name +' to '+zip_name)
else:
	try:
		print('Try RAK3172 file type')
		zipfile.ZipFile(zip_name, mode='w').write(bin_name)
	except:
		print('Cannot zip '+bin_name +' to '+zip_name)


os.chdir("../")

if not is_rak4631:
	try:
		shutil.copy2("./build/"+zip_name, destination_directory)
	except:
		print('Cannot copy '+"./build/"+zip_name +' to '+destination_directory)

# Rename the file
try:
	os.rename(copied_file, new_file)
except:
	print('Cannot rename '+copied_file +' to '+new_file)

