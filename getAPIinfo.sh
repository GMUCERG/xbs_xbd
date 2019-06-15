#!/bin/bash
#
# Brian Hession
#
# Crawls through algobase/{OP} and grabs api.h information
#

# Get the directory of the script
DIR=$(readlink "$0")
if [ $? -ne 0 ]; then
	DIR="$0"
fi
DIR="$(cd "$(dirname "$0")" && pwd)"

print_usage_and_exit() {
	echo "$0 [-h] [-d algobase-dir] <crypto_kem|crypto_sign|crypto_aead>" >&2
	exit
}

# Parameter check
targetdir="$DIR/algobase"
if [ -z "$1" ]; then
	print_usage_and_exit
elif [[ $1 == -h ]]; then
	print_usage_and_exit
elif [[ $1 == -d ]]; then
	shift
	if ! [ -d "$1" ]; then
		print_usage_and_exit
	fi
	targetdir="$(cd "$1" && pwd)"
	shift
fi
optype="$1"

# Populate the operation's relevant fields (minus the CRYPTO_)
fields=()
if [ -z "$optype" ]; then
	print_usage_and_exit
elif [[ $optype == crypto_kem ]]; then
	fields=("PUBLICKEYBYTES" "SECRETKEYBYTES" "CIPHERTEXTBYTES" "BYTES")
elif [[ $optype == crypto_sign ]]; then
	fields=("PUBLICKEYBYTES" "SECRETKEYBYTES" "BYTES")
elif [[ $optype == crypto_aead ]]; then
	fields=("KEYBYTES" "NSECBYTES" "NPUBBYTES" "ABYTES")
else
	print_usage_and_exit
fi

# Print CSV header
echo "ALGORITHM ${fields[@]}" | tr '[[:blank:]]' ','

# Crawls through algorithms
for algo in "$targetdir/$optype"/*; do
	if [ -d "$algo" ]; then
		pushd "$algo" >/dev/null

		# Assumes failure
		gotit=0

		# Crawls through every instance of api.h
		for f in $(find . -name 'api.h' -type f); do
			api_h="$(cat "$f")"
			vals="$(basename $algo)"

			# Attempts to populate all of the fields
			for field in ${fields[@]}; do

				# Creates the regex to check for
				regex="^#define\s+CRYPTO_$field\s+"

				# Get a count of matching lines
				count="$(echo "$api_h" | grep -Pc "$regex")"
				if [ $count -ne 1 ]; then
					# api.h does not contain the key or has too many instances of it
					continue 2
				fi

				# Grab the line and extract the information
				line="$(echo "$api_h" | grep -P "$regex")"
				val=$(echo "$line" | sed -r -e "s/$regex//" -e "s/\s.*//")
				if [ -z "$val" ]; then
					# The regex failed..?
					continue 2
				elif ! [[ $val =~ ^[0-9]+$ ]]; then
					# The value is not numeric
					continue 2
				fi

				# Update entry
				vals="$vals,$val"
			done

			# Mark completion and print the entry
			gotit=1
			echo "$vals"
			break 1
		done

		if [ $gotit -eq 0 ]; then
			# Something went wrong
			echo "ERROR: Could not grab sizes for $(basename "$algo")" >&2
		fi
		popd >/dev/null
	fi
done

