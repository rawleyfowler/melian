#!/usr/bin/env bash
MELIAN_PID=$(pgrep melian-server)
REDIS_PID=$(pgrep redis-server)
OUT=${1:-metrics.csv}

echo "ts,proc,pid,cpu,mem_rss" > "$OUT"

while true; do
  ts=$(date +%s)
  for proc in melian redis; do
    pid_var="${proc^^}_PID"
    pid=${!pid_var}
    if ps -p "$pid" > /dev/null; then
      read -r _ _ cpu _ rss _ < <(ps -p "$pid" -o pid,%cpu,rss -h)
      echo "$ts,$proc,$pid,$cpu,$rss" >> "$OUT"
    fi
  done
  sleep 1
done

