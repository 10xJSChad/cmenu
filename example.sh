#!/bin/sh

./cmenu output.txt < "colors.txt"
output=$(./cmenu output.txt -r)

if [ "$output" = "blue" ]
then
    echo "You chose blue"
else
    echo "You did not choose blue"
fi
