#!/bin/bash

BUILDDIR=d-xbdbuild
BUILDLIST=(
	'ek-tm4c123gxl_16mhz crypto_kem babybear ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem mamabear ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem papabear ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem kyber512 ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem kyber768 ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem kyber1024 ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem newhope512cca ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem newhope1024cca ref enc'
	'ek-tm4c123gxl_16mhz crypto_kem ntruhrss701 ref enc'
	'ek-tm4c123gxl_16mhz crypto_sign dilithium2 ref sign'
	'ek-tm4c123gxl_16mhz crypto_sign dilithium3 ref sign'
	'ek-tm4c123gxl_16mhz crypto_sign qtesla1 ref sign'
	'ek-tm4c123gxl_16mhz crypto_sign qtesla1p ref sign'
#	'ek-tm4c123gxl_16mhz crypto_kem babybear ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem mamabear ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem papabear ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem kyber512 ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem kyber768 ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem kyber1024 ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem newhope512cca ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem newhope1024cca ref dec'
#	'ek-tm4c123gxl_16mhz crypto_kem ntruhrss701 ref dec'
#	'ek-tm4c123gxl_16mhz crypto_sign dilithium2 ref open'
#	'ek-tm4c123gxl_16mhz crypto_sign dilithium3 ref open'
#	'ek-tm4c123gxl_16mhz crypto_sign qtesla1 ref open'
#	'ek-tm4c123gxl_16mhz crypto_sign qtesla1p ref open'
)

gen_build() {
	platform="$1"
	op="$2"
	algo="$3"
	impl="$4"
	mode="$5"

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

	echo -e "$o_h" > ${BUILDDIR}/${platform}/$op/$algo/$impl/${op}.h
	echo -e "$op_h" > ${BUILDDIR}/${platform}/$op/$algo/$impl/${op}_${algo}.h
}

make_key() {
	platform=$1
	op=$2
	algo=$3
	impl=$4
	mode=$5

	ext=''
	if [[ "$mode" == "sign" || "$mode" == "dec" ]]; then
		ext='sec'
	elif [[ "$mode" == "open" || "$mode" == "enc" ]]; then
		ext='pub'
	else
		echo "ERROR: Invalid mode `$mode`"
		return
	fi
	keyfile="xbdkeys/$op/$algo/$impl/1.$ext"

	# Check for the key
	if ! [ -f "$keyfile" ]; then
		echo "ERROR: Key file does not exist `$keyfile`"
		return
	fi

	# Convert base64 to c-style include
	bytes=$(base64 -d "$keyfile" | xxd -i)

	# Create header file
	headerfile="${BUILDDIR}/$platform/$op/$algo/$impl/xbdkey.h"
	echo -e "#ifndef XBD_KEY_H" > "$headerfile"
	echo -e "#define XBD_KEY_H\n" >> "$headerfile"
	echo -e "unsigned char xbdkey[] = {" >> "$headerfile"
	echo -e "$bytes" >> "$headerfile"
	echo -e "};\n" >> "$headerfile"
	echo -e "#endif\n" >> "$headerfile"
}

build() {
	make -f d-xbdmakefile PLATFORM=$1 OP=$2 ALGO=$3 IMPL=$4 MODE=$5 >/dev/null
	echo $?
}

####################################################
# MAIN
####################################################

command=$1
if ! [ -z "$command" ]; then
	if [[ $command == clean ]]; then
		make -f d-xbdmakefile clean
		exit
	elif [[ $command == gdb ]]; then
		openocd=$(netstat -tln | grep -P ':3333\s')
		if [ -z "$openocd" ]; then
			echo "ERROR: Run openocd first"
			exit
		fi
		for b in $(seq 0 $((${#BUILDLIST[@]} - 1))); do
			build=(${BUILDLIST[$b]})
			platform="${build[0]}"
			op="${build[1]}"
			algo="${build[2]}"
			impl="${build[3]}"
			mode="${build[4]}"
			if [ -f "${BUILDDIR}/$platform/$op/$algo/$impl/$mode.axf" ]; then
				echo "$b $platform-$op-$algo-$impl-$mode"
			fi
		done
		read -p "Input test number (default 0): " choice
		if [ -z "$choice" ]; then
			choice=0
		elif ! [[ $choice =~ ^[0-9]+$ ]]; then
			echo "ERROR: Invalid choice"
			exit
		elif [[ $choice -ge ${#BUILDLIST[@]} ]]; then
			echo "ERROR: Invalid choice"
			exit
		fi
		build=(${BUILDLIST[$choice]})
		platform="${build[0]}"
		op="${build[1]}"
		algo="${build[2]}"
		impl="${build[3]}"
		mode="${build[4]}"
		arm-none-eabi-gdb -x "${BUILDDIR}/$platform/$op/$algo/$impl/$mode-gdbinit"
		exit
	elif ! [[ $command == make ]]; then
		echo "ERROR: Unknown command `$command`"
		exit
	fi
fi

for b in $(seq 0 $((${#BUILDLIST[@]} - 1))); do
	build=(${BUILDLIST[$b]})
	platform="${build[0]}"
	op="${build[1]}"
	algo="${build[2]}"
	impl="${build[3]}"
	mode="${build[4]}"

	if [[ -z "$platform" || -z "$op" || -z "$algo" || -z "$impl" || -z "$mode" ]]; then
		continue
	fi

	echo '================================================================================'
	echo -n "Building $platform/$op/$algo/$impl..."
	mkdir -p "${BUILDDIR}/$platform/$op/$algo/$impl"
	gen_build "$platform" "$op" "$algo" "$impl" "$mode"
	ret=$(make_key "$platform" "$op" "$algo" "$impl" "$mode")
	if ! [ -z "$ret" ]; then
		echo "$ret"
		continue
	fi
	ret=$(build "$platform" "$op" "$algo" "$impl" "$mode")
	if [ $ret -eq 0 ]; then
		echo 'Successful'
	else
		echo 'Failed'
		echo "Run `make PLATFORM=$platform OP=$op ALGO=$algo IMPL=$impl MODE=$mode` for more information"
		continue
	fi
	echo 'target extended-remote :3333' > "${BUILDDIR}/$platform/$op/$algo/$impl/$mode-gdbinit"
	echo "file ${BUILDDIR}/$platform/$op/$algo/$impl/$mode.axf" \
		>> "${BUILDDIR}/$platform/$op/$algo/$impl/$mode-gdbinit"
	echo 'load' >> "${BUILDDIR}/$platform/$op/$algo/$impl/$mode-gdbinit"
	echo '================================================================================'
done

