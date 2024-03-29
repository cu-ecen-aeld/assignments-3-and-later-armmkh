#!/bin/sh

dir=$1
str=$2

if [ $# -ne 2 ]; then
    echo "This script expects two arguments."
    exit 1
fi

if ! [ -d $dir ]; then
    echo "Directory $dir does not exist."
    exit 1
fi

files_num=`grep -rl ${str} ${dir} | wc -l`
match_num=`grep -or ${str} ${dir} | wc -w`

echo "The number of files are ${files_num} and the number of matching lines are ${match_num}"