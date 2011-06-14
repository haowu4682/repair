#!/bin/bash
D=$(readlink -f -f $(dirname $0))
echo $D
