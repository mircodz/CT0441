find . -not -path '*/\.*' | grep -E '\.[hc]$' | xargs wc | awk '{print $1}' | tail -n 1
