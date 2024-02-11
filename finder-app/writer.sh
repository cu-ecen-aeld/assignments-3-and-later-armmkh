#!/bin/bash

set -e

if [ $# != 2 ]
then
	echo "This script expects two arguments."
	exit 1	
 
fi
DIR="$(dirname $1)" 
FILE="$(basename $1)"
mkdir -p ${DIR}
cd ${DIR}
echo $2>  ${FILE} 