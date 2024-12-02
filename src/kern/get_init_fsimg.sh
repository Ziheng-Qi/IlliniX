!#/bin/bash
cd ../util
make clean
make
cd ../user
make clean
make
cp -r bin ../util
cd ../util
./mkfs kfs.raw ./bin/init0 ./bin/init1 ./bin/init2 ./bin/trek ./bin/init3 ./bin/init4 ./bin/ls ./bin/helloworld.txt 
mv kfs.raw ../kern
cd ../kern
