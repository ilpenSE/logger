#!/bin/bash
# Run benchmark.sh N times in a row

bash_rst="\e[0m"
bash_bgreen="\e[1;32m"
bash_byellow="\e[1;33m"
bash_baqua="\e[1;36m"

RUNS=${1:-100}

printf "Will run ${bash_baqua}$RUNS$bash_rst times in a row\n"

# First, clear the results.txt
> results.txt

counter=1
while [ $counter -le $RUNS ]; do
  printf "Running $bash_byellow$counter$bash_rst..."
  ./benchmark.sh
  printf " ${bash_bgreen}[OK]$bash_rst\n"
  ((counter++))
done

printf "${bash_baqua}======== RESULTS ========$bash_rst\n"
python analyze.py
