#!/bin/bash

jobpath=$(realpath $1)
export binpath=$(printf '%q' $(pwd))
export solver=natt++
export TIMEOUT=60
export csv=$jobpath/results/result.csv
export proofdir=$jobpath/proofs

echo benchmark,solver,result,wallclock time > $csv

(	cd ~/TPDB-ARI/TRS_Standard
	printf '%q\n' */*.ari | xargs -P $(nproc) -L 1 bash -c '
		f=${1%.*}
		start=$EPOCHREALTIME
		result=$(timeout $TIMEOUT $binpath/$solver $1 2> $proofdir/${f////_}_$solver.txt
			if [ $? -eq 124 ]; then echo timeout; fi
		)
		end=$EPOCHREALTIME
		echo $f,$solver,$result,$(awk "BEGIN {printf \"%.3f\", ($end - $start)}")
	' _
) | tee -a $csv
echo -n "YES: "; grep -c ',YES' $csv
echo -n "NO: "; grep -c ',NO' $csv
echo -n "TIMEOUT: "; grep -c ',timeout' $csv
