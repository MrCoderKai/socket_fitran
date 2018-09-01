#/bin/bash

for i in `seq 500`;do
    ./client &
done
wait
