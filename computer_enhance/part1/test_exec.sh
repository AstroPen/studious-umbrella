#!/bin/zsh

./disassemble $1 -exec > temp.txt
diff --strip-trailing-cr temp.txt $1.txt
if [ $? -ne 0 ]; then
  echo "Test failed.";
else
  echo "Test passed.";
fi
rm temp.txt

