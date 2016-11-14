#!/bin/sh
#
# Wrapper script to run demo program with the right classpath and library path.
dir=$(dirname $0)
dir=$dir:$dir/dist/*
dir=$dir:$dir/lib/*
java -Djava.library.path=$dir -cp ${dir} reader.Demo "$@"
