# To copy it to the Pintos file system under the name newname , add-a newname : 
#  To copy file into the Pintos file system with newname
pintos -p file -a newname -- -q .

# Here's a summary of how to create a disk with a file system partition, format the file
# system, copy the echo program into the new disk, and then run echo, passing argument x.
# (Argument passing won't work until you implemented it.) It assumes that you've already
# built the examples in examples and that the current directory is userprog/build :
pintos-mkdisk filesys.dsk --filesys-size=2
pintos -f -q
pintos -p ../../examples/echo -a echo -- -q
pintos -q run 'echo x'

# The three final steps can actually be combined into a single command:
pintos-mkdisk filesys.dsk--filesys-size=2
pintos-p ../../examples/echo-a echo---f-q run echo x

# You can delete a file from the Pintos file system using the rm file kernel action
pintos-q rm file

# prints a file's contents to the display
cat file

# to add a new test case
# The file system limits file names to 14 characters.
# In /examples/Makefile
# - add its filename to PROGS list
# - add $file_SRC = $file.c