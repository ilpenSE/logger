#!/bin/bash

target="app"

policies=("drop" "block")

date=$(date +"%Y-%m-%d %H:%M:%S")

echo "////////// TEST BEGIN" >> results.txt
echo "////////// $date" >> results.txt
for policy in "${policies[@]}"; do
  result_t=$(./$target $policy true 2> /dev/null | grep 'Per log' | awk '{print $3}')
  echo "stderr=true,policy=$policy: $result_t ns" >> results.txt
  result_f=$(./$target $policy false 2> /dev/null | grep 'Per log' | awk '{print $3}')
  echo "stderr=false,policy=$policy: $result_f ns" >> results.txt
done
echo "////////// TEST END" >> results.txt

