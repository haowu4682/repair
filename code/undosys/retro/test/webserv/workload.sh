#!/bin/sh

for page in 1 2 3; do
    python -u ./workload.py 'do' ${page}
done