make clean
cd ../util
echo "Creating raw fs image"
sh test.sh
cd ../kern
make rubric_test.elf; make run-rubric_test
echo "Rubric test complete"
rm kfs.raw