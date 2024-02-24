#!/bin/zsh

./disassemble $1 > temp.asm
nasm temp.asm
diff temp $1
if [ $? -ne 0 ]; then
  echo "Test failed.";
else
  echo "Test passed.";
fi
rm temp temp.asm

