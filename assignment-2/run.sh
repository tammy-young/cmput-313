#!/usr/bin/env bash
# Runs parameter sweep for buffer bloat experiments

run() {
  local -i queue_size="$1"
  local -i runtime="$2"
  local algo="$3"

  local data_dir="data_${algo}_${queue_size}"

  sudo python3 main.py "$queue_size" "$runtime" "$algo" "$data_dir"
  sudo python3 plotting.py "$data_dir" "$algo" "$runtime"
}

if ! command -v viewnior &>/dev/null; then
  echo 'Installing image viewer'
  sudo apt update
  sudo apt install viewnior
fi

for queue_size in {10,50}; do
  for algo in {reno,cubic,bbr}; do
    run "$queue_size" 60 "$algo"
  done
done
