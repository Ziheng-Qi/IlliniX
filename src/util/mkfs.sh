#!/bin/bash
make clean
make
# Define the root folder
ROOT_FOLDER="root_folder"

# Collect all files in the root folder
FILES=$(find "$ROOT_FOLDER" -type f)

# Execute the mkfs command with the collected files
echo ./mkfs kfs.raw $FILES
./mkfs kfs.raw $FILES

# Remove the existing kfs.raw file in the ../kern/ directory
rm -f ../kern/kfs.raw

# Move the new kfs.raw to the ../kern/ directory
mv kfs.raw ../kern/kfs.raw
echo Copied kfs.raw to ../kern/kfs.raw

make clean
