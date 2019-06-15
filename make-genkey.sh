#!/bin/bash

KEYDIR=xbdkeys
BUILDDIR=x86build
BUILDLIST=(
	'crypto_kem babybear xbdref'
	'crypto_kem firesaber xbd'
	'crypto_kem frodo640aes m4'
	'crypto_kem frodo640cshake m4'
	'crypto_kem kindi256342 m4'
	'crypto_kem kyber512 ref'
	'crypto_kem kyber768 ref'
	'crypto_kem kyber1024 ref'
	'crypto_kem lightsaber xbd'
	'crypto_kem mamabear xbdref'
	'crypto_kem newhope512cca ref'
	'crypto_kem newhope1024cca ref'
	'crypto_kem ntruhrss701 pqm4ref'
	'crypto_kem ntrukem443 xbdref'
	'crypto_kem ntrukem743 pqm4ref'
	'crypto_kem papabear xbdref'
#	'crypto_kem rlizard102411 m4'
	'crypto_kem saber m4'
	'crypto_kem sikep503 xbdref'
	'crypto_kem sikep751 pqm4ref'
	'crypto_kem sntrup4591761 m4'
#	'crypto_sign dilithium2 ref'
#	'crypto_sign dilithium3 ref'
#	'crypto_sign qtesla1 ref'
#	'crypto_sign qtesla1p ref'
	'crypto_sign sphincss128shake256 xbdref'
)
NUM_KEYPAIRS=10

gen_build() {
	local op="$1"
	local algo="$2"
	local impl="$3"

	# Get operation specs
	declare -a vars
	declare -a defs
	while read -r line; do
		if [[ $line =~ $op.* ]]; then
			read -r vs ds <<< "${line/$op /}"
			OIFS=$IFS
			IFS=':'
			for v in $vs; do
				vars[${#vars[@]}]="$v"
			done
			for d in $ds; do
				defs[${#defs[@]}]="$d"
			done
			IFS=$OIFS
		fi
	done < OPERATIONS

	o_macros=''
	for i in $(seq 1 ${#vars[@]}); do
		var=${vars[$i]}
		o_macros="${o_macros}#define ${op}${var} ${op}_${algo}${var}\n"
	done

	o_h="#ifndef ${op}_H\n"
	o_h="${o_h}#define ${op}_H\n\n"
	o_h="${o_h}#include \"${op}_${algo}.h\"\n"
	o_h="${o_h}${o_macros}"
	o_h="${o_h}#define ${op}_PRIMITIVE \"${algo}\"\n"
	o_h="${o_h}#define ${op}_IMPLEMENTATION ${op}_${algo}_IMPLEMENTATION\n"
	o_h="${o_h}#define ${op}_VERSION ${op}_${algo}_VERSION\n\n"
	o_h="${o_h}#endif"

	api_h=''
	while read -r line; do
		nl=$(echo "$line" | sed "s/CRYPTO/${op}_${algo}_${impl}/")
		api_h="${api_h}${nl}\n"
	done < "algobase/$op/$algo/$impl/api.h"

	opi_prototypes=''
	for i in $(seq 0 $((${#defs[@]} - 1))); do
		opi_prototypes="${opi_prototypes}extern int ${op}_${algo}_${impl}${defs[$i]};\n"
	done

	op_macros=''
	for i in $(seq 1 ${#vars[@]}); do
		var=${vars[$i]}
		op_macros="${op_macros}#define ${op}_${algo}${var} ${op}_${algo}_${impl}${var}\n"
	done

	op_h="#ifndef ${op}_${algo}_H\n"
	op_h="${op_h}#define ${op}_${algo}_H\n\n"
	op_h="${op_h}${api_h}\n\n"
	op_h="${op_h}#ifdef __cplusplus\n"
	op_h="${op_h}extern \"C\" {\n"
	op_h="${op_h}#endif\n"
	op_h="${op_h}${opi_prototypes}"
	op_h="${op_h}#ifdef __cplusplus\n"
	op_h="${op_h}}\n"
	op_h="${op_h}#endif\n\n"
	op_h="${op_h}${op_macros}\n"
	op_h="${op_h}#define ${op}_${algo}_IMPLEMENTATION \"${impl}\"\n"
	op_h="${op_h}#ifndef ${op}_${algo}_${impl}_VERSION\n"
	op_h="${op_h}#define ${op}_${algo}_${impl}_VERSION \"-\"\n"
	op_h="${op_h}#endif\n"
	op_h="${op_h}#define ${op}_${algo}_VERSION ${op}_${algo}_${impl}_VERSION\n\n"
	op_h="${op_h}#endif"

	echo -e "$o_h" > ${BUILDDIR}/$op/$algo/$impl/${op}.h
	echo -e "$op_h" > ${BUILDDIR}/$op/$algo/$impl/${op}_${algo}.h
}

build() {
	make -f x86makefile OP=$1 ALGO=$2 IMPL=$3 >/dev/null
	echo $?
}

gen_key() {
	local resp=$("$BUILDDIR/$1/$2/$3/genkey")
	echo -e "$?\n$resp"
}

do_main() {
	for b in $(seq 0 ${#BUILDLIST[@]}); do
		build=(${BUILDLIST[$b]})
		op="${build[0]}"
		algo="${build[1]}"
		impl="${build[2]}"
		implref=$impl
		if [ "$implref" == "m4" ]; then
			implref="pqm4ref"
		elif [ "$implref" == "xbd" ]; then
			implref="xbdref"
		fi

		if [[ -z "$op" || -z "$algo" || -z "$impl" ]]; then
			continue
		fi

		echo '================================================================================'
		echo -n "Building $op/$algo/$impl..."
		mkdir -p "${BUILDDIR}/$op/$algo/$implref"
		mkdir -p "${KEYDIR}/$op/$algo/$impl"
		gen_build "$op" "$algo" "$implref"
		ret=$(build "$op" "$algo" "$implref")
		if [ $ret -eq 0 ]; then
			echo 'Successful'
		else
			echo 'Failed'
			echo "Run `make OP=$op ALGO=$algo IMPL=$implref` for more information"
			continue
		fi

		# Generate keypairs NUM_KEYPAIRS times
		for i in $(seq 0 $(($NUM_KEYPAIRS - 1))); do
			echo -n "$i Generating keys..."
			resp=$(gen_key "$op" "$algo" "$implref")

			# Return code is the first line
			ret=$(head -n1 <<< "$resp")
			if [ $ret -ne 0 ]; then
				echo 'Failed'
				continue
			fi

			# Data starts at the 2nd line
			keys=$(tail -n +2 <<< "$resp")

			# The keys are separated by an empty line
			sep=0
			p=''
			s=''
			while read -r line; do
				if [ -z "$line" ]; then
					sep=1
				elif [ $sep -eq 0 ]; then
					p="${p}${line}\n"
				elif [ $sep -eq 1 ]; then
					s="${s}${line}\n"
				fi
			done <<< "$keys"
			echo -e "$p" > "${KEYDIR}/$op/$algo/$impl/$i.pub"
			echo -e "$s" > "${KEYDIR}/$op/$algo/$impl/$i.sec"
			echo 'Successful'
		done
		echo '================================================================================'
	done
}

####################################################
# MAIN
####################################################

commands=$@
if [ -z "$commands" ]; then
	do_main
	exit
fi

for com in ${commands[@]}; do
	if [[ "$com" == "make" || "$com" == "all" ]]; then
		do_main
	elif [[ "$com" == "clean" ]]; then
		make -f x86makefile clean
	else
		echo "ERROR: Unknown command `$com`"
	fi
done

