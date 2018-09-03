#/bin/bash

for i in `seq 10`;do
    nohup ./client > $i.log 2>&1  &
    usleep 200
done
wait
