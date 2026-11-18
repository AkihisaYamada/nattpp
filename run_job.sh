#!/bin/bash

export binpath=$(printf '%q' $(pwd))
export solver=natt++

if [ $1 == '-P' ]; then
	shift
	export nproc=$1
	shift
else
	export nproc=$(nproc)
fi

benchmark=$1
shift
export jobpath=$(realpath $1)
shift
export config=$1
shift

export TIMEOUT=60
export csv=$jobpath/results/result.csv
export proofdir=$jobpath/proofs

if [[ -z $config ]]
then export participant=$solver
else export participant=${solver}_$config
fi

mkdir $jobpath
mkdir $jobpath/results
mkdir $jobpath/proofs

echo benchmark,solver,configuration,status,result,wallclock time > $csv

(	cd $benchmark
	shopt -s globstar
	printf '%q\n' **/*.ari | xargs -P $nproc -L 1 -I {} bash -c '
		start=$EPOCHREALTIME
		result=$(timeout $TIMEOUT $binpath/$solver "$@" 2> $proofdir/${1////_}_$participant.txt)
		if [ $? -eq 124 ]
		then status=timeout
		else status=complete
		fi
		end=$EPOCHREALTIME
		echo $1,$solver,$config,$status,$result,$(awk "BEGIN {printf \"%.3f\", ($end - $start)}")
	' bash {} "$@"
) | tee -a $csv
echo -n "YES: "; grep -c ',YES' $csv
echo -n "NO: "; grep -c ',NO' $csv
echo -n "TIMEOUT: "; grep -c ',timeout' $csv
