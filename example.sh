#!/bin/sh

output=$(./cmenu < "colors.txt")

if [ "$output" = "blue" ]
then
    echo "You chose blue"
else
    echo "You did not choose blue"
fi
