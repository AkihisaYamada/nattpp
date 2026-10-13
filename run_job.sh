#!/bin/bash

jobpath=$(realpath $1)
shift
export binpath=$(printf '%q' $(pwd))
export solver=natt++
export config=$1
shift
export opt=$*
export TIMEOUT=60
export csv=$jobpath/results/result.csv
export proofdir=$jobpath/proofs

echo benchmark,solver,configuration,result,wallclock time > $csv

(	cd ~/TPDB-ARI/TRS_Standard
	printf '%q\n' */*.ari | xargs -P $(nproc) -L 1 bash -c '
		start=$EPOCHREALTIME
		result=$(timeout $TIMEOUT $binpath/$solver $opt $1 2> $proofdir/${1////_}_$solver.txt
			if [ $? -eq 124 ]; then echo timeout; fi
		)
		end=$EPOCHREALTIME
		echo $1,$solver,$config,$result,$(awk "BEGIN {printf \"%.3f\", ($end - $start)}")
	' _
) | tee -a $csv
echo -n "YES: "; grep -c ',YES' $csv
echo -n "NO: "; grep -c ',NO' $csv
echo -n "TIMEOUT: "; grep -c ',timeout' $csv
