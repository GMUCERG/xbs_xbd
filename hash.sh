#!/bin/bash
# Simple script to update the hash in impl_conf.ini because it was a pain in
# the ass to do manually.

SRCDIR=$(cd "$(dirname "$0")" && pwd)
HASHDIR=$1
IMPL_CONF="$SRCDIR/impl_conf.ini"

if [ -z "$HASHDIR" ] || [ ! -d "$HASHDIR" ]; then
	echo "Usage: $0 [path]"
	exit 1
fi

HASHDIR=$(cd "$1" && pwd)
OP_DIR=$(echo "$HASHDIR" | sed 's|.*/crypto_|crypto_|')

HASH=$(python "$SRCDIR/python/xbx/dirchecksum.py" "$HASHDIR")

if [ ! "$(grep -ls "$OP_DIR" "$IMPL_CONF")" ]; then
	echo "Error: $OP_DIR not in $IMPL_CONF"
	echo "Please add:"
	echo "$OP_DIR $HASH [deps...]"
	exit 1
else
	echo "Updating hash for $OP_NAME: $HASH"
	sed -i.bak "s|$OP_DIR\ [0-9a-f]*|$OP_DIR\ $HASH|" "$IMPL_CONF"
	if [ -z "$(grep -ls "$HASH" "$IMPL_CONF")" ]; then
		echo "Failure"
		exit 1
	else
		echo "Success"
	fi
fi
