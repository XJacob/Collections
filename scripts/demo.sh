#!/bin/bash

version=1

#function and get file string by for loop
list_file()
{
	#list files in the directory
	for file in $1/*; do
		echo "file is $file"
	done
}

echo -n "demo.sh "
echo "V$version."

# if/else and string compare
# parameter1 is $1, parameter2 is $2....
if [ "$1" != "" ]; then
list_file $1
else
read -p "please input path:" path
path=${path:-"./"}   # if path is empty, set the default value
list_file $path
fi

echo ""

################ if/else, test
filename="./demo.sh"
test -z $filename && echo "file is empty"
test ! -z $filename && echo "file is $filename"
test -f $filename && echo "$filename exist"
test $filename == "./demo.sh" && test ! -d $filename  && echo "$filename is demo.sh"
if [ $filename == "./demo.sh" ] && [ ! -d $filename ]; then
	echo "$filename is demo.sh( == !d )"
fi

if [ ! -d $filename -a -f $filename ]; then
	echo "$filename exist( !d a f)"
fi

################ case
case $filename in
	"./demo.sh")
		echo "$filename exist(case)"
		;;
	" ")
		echo "don't input anything"
		;;
esac

############### loop
while [ "$yn" != "y" ]
do
	read -p "input y to stop loop:" yn
done

name="dog cat elephant"
for animal in $name; do
	echo "animal is $animal"
done

for num in $(seq 1 10); do
	echo -n "$num"
done

