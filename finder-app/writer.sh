#!/bin/bash
 
if [ $# -ne 2 ]

then
	echo "Error : Two arguments needed : Filepath and text string"
	exit 1
fi
 
writefile=$1

writestr=$2
 
dir_path=$(dirname "$writefile")
 
if [ ! -d "$dir_path" ]

then

	mkdir -p "$dir_path"

fi
 
echo "$writestr" > "$writefile"
 
if [ $? -ne 0 ]
then
	echo "Error : Unable to create file"
	exit 1
fi

exit 0
