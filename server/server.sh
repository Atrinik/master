#!/bin/sh
#
# Script to run the server.

# 'lib' directory doesn't exist, create it.
if [ ! -d "lib" ]; then
	mkdir "lib"
fi

# 'data' directory doesn't exist, copy 'install' as 'data'.
if [ ! -d "data" ]; then
	cp -R "install" "data"
fi

# Copy all files from 'arch' to the 'lib' directory.
cp ../arch/* lib > /dev/null 2>&1
# Clean up 'tmp' directory in 'data'.
rm data/tmp/* > /dev/null 2>&1

# Start up the server. If running from a terminal, pass options to the
# executable. Otherwise, start up the server with some sane options,
# which includes redirecting the log to a file.
if [ -t 1 ]; then
	./atrinik-server "$@"
else
	./atrinik-server -log logfile.log
fi
