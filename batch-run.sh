#!/bin/bash

num_runs='0\n 1\n 2\n 3\n 4\n 5\n 6\n 7\n 8\n 9'
#num_runs='0'

runs=(
#      'crypto_kem babybear'
#      'crypto_kem firesaber'
#      'crypto_kem kyber1024'
#      'crypto_kem kyber512'
#      'crypto_kem kyber768'
#      'crypto_kem lightsaber'
#      'crypto_kem mamabear'
#      'crypto_kem newhope1024cca'
      'crypto_kem newhope512cca'
#      'crypto_kem ntruhrss701'
#      'crypto_kem ntrukem443'
#      'crypto_kem ntrukem743'
#      'crypto_kem papabear'
#      'crypto_kem saber'
#      'crypto_kem sntrup4591761')
#      'crypto_kem sikep503'
#      'crypto_kem sikep751'
)

rm -f "batch-compile.log"
rm -f "batch-run.log"
for i in $(seq 0 $((${#runs[@]} - 1))); do
	run=(${runs[i]})
	op=${run[0]}
	algo=${run[1]}

	# Back up config.ini
	if ! [ -f "config.ini.batch" ]; then
		cp "config.ini" "config.ini.batch"
	fi

	# Create temporary config
	sed -e "s/%op%/$op/" -e "s/%algo%/$algo/" -e "s/%num_runs%/$num_runs/" \
			"batch-config.ini" > "config.ini"
	if [ $? -ne 0 ]; then
		echo "Error making configuration file"
		exit
	fi

	# Run the tests
	rm -rf "data.db" "work/ek-tm4c123gxl_16mhz/"
	./compile.py | tee -a "batch-compile.log"
	./execute.py | tee -a "batch-run.log"
	if [ $? -ne 0 ]; then
		exit
	fi

	# Extract the data
	./extract_data.py kem
	mv -v "data.db" "../../ECE746/project/$algo.db"
	mv -v "data.csv" "../../ECE746/project/$algo.csv"

	# Restore config.ini
	rm "config.ini"
	mv "config.ini.batch" "config.ini"
done

