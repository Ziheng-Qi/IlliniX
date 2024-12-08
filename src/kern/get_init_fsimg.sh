!#/bin/bash
cd ../util
make clean
make
cd ../user
make
cp -r bin ../util
cd ../util
FILES_INBIN=$(find ./bin -type f -name "*" | xargs)
echo $FILES_INBIN
./mkfs kfs.raw $FILES_INBIN
mv kfs.raw ../kern
cd ../kern