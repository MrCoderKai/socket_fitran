#/bin/bash

for i in `seq 10`;do
    ./client &
    usleep 200
done
wait
