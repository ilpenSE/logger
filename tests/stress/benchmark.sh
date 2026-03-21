#!/bin/bash

target="app"

threads=("single" "multi")
policies=("drop" "block")
stderrs=("true" "false")

date=$(date +"%Y-%m-%d %H:%M:%S")

echo "////////// TEST BEGIN" >> results.txt
echo "////////// $date" >> results.txt
for policy in "${policies[@]}"; do
  for stderr in "${stderrs[@]}"; do
    for thread in "${threads[@]}"; do
      result_t=$(./$target $thread $policy $stderr 2> /dev/null | grep 'Throughput' | awk '{print $2}')
      echo "thread=$thread,stderr=$stderr,policy=$policy: $result_t logs/sec" >> results.txt
    done
  done
done
echo "////////// TEST END" >> results.txt

