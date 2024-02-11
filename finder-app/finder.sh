#!/bin/bash

set -e

if [ $# != 2 ]
then
	echo "This script expects two arguments."
	exit 1	
else 
    if ! test -d $1; 
    then 
        echo "Directory does not exist."
        exit 1
    fi
fi

cd $1 
NFILES=$(find | wc -l)
((NFILES--)) 
NMATCHES=$(grep -r $2 ./* | wc -l)

echo "The number of files are ${NFILES} and the number of matching lines are ${NMATCHES}"  


