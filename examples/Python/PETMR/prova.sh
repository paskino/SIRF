#! /bin/bash

# defaults
gamma=1.0
alpha=0.5
while getopts ha:g:i: option
 do
 case "${option}"
 in
  a) alpha=${OPTARG};;
  g) gamma=${OPTARG};;
  i) initial=${OPTARG};;
  h)
   echo "Usage: $0 -a alpha -g gamma"
   exit 
   ;;
  *)
   echo "Wrong option passed. Use the -h option to get some help." >&2
   exit 1
  ;;
 esac
done

echo "Current alpha ${alpha}" > mylog.txt
echo "Current gamma ${gamma}" >> mylog.txt