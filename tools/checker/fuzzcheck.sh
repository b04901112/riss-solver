#!/bin/bash

#
# create cnf files via fuzzing, check with verifier, apply cnfdd if required
#

prg=$1

# this can be initialized to a max. size of an unreduced buggy formula
bestcls=5000

cnf=/tmp/runcnfuzz-$$.cnf
sol=/tmp/runcnfuzz-$$.sol
err=/tmp/runcnfuzz-$$.err
out=/tmp/runcnfuzz-$$.out
log=runcnfuzz-$$.log
rm -f $log $cnf $sol
echo "[runcnfuzz] running $prg"
echo "[runcnfuzz] logging $log"
echo "run $1 $2" > $log
echo "[runcnfuzz] run with pid $$"
echo "[runcnfuzz] started at $(date)"
i=0

# set limit for number of instances to be solved
limit=1
if [ "x$2" != "x" ]
then
  limit=$2
fi
test=0

while [ "$test" -lt "$limit" ]
do
  rm -f $cnf; 
  ./cnfuzz $CNFUZZOPTIONS > $cnf
  # control whether algorithm should sleep
#  sleep 0.02
  seed=`grep 'c seed' $cnf|head -1|awk '{print $NF}'`
  head="`awk '/p cnf /{print $3, $4}' $cnf`"
  printf "%d %16d          %6d    %6d               \r" "$i" "$seed" $head
  i=`expr $i + 1`
  rm -f $sol

  # set limit for number of instances to be solved
  if [ "x$2" != "x" ]
  then
    test=$(($test+1))
  fi

  thiscls=`awk '/p cnf /{print $4}' $cnf`
  if [ $bestcls -ne "-1" ]
  then
    if [ $bestcls -le $thiscls ]
    then
#      echo "reject new bug with $thiscls clauses"
      continue
    fi
  fi

  ./toolCheck.sh $prg $cnf > $out 2> $err
  res=$?
#  echo "result $res"
  case $res in
    124)
      echo "($SECONDS s) timeout with $seed" > $log
      mv $cnf timeout-$seed.cnf
      continue
      ;;
    10)
      continue
      ;;
    20)
      continue
      ;;
    15)
    ;;
    25)
    ;;
  esac
  head="`awk '/p cnf /{print $3, $4}' $cnf`"
  echo "($SECONDS s) [runcnfuzz] bug-$seed $head             with exit code $res                         "
  echo $seed >> $log
  echo "($SECONDS s) out"
  cat $out
  echo "($SECONDS s) err"
  cat $err
  
  #
  # consider only bugs that are smaller then the ones we found already
  #
  if [ $bestcls -eq "-1" ]
  then
    bestcls=$thiscls
    echo "($SECONDS s) init bestcls with $bestcls"
  elif [ $bestcls -le $thiscls ]
  then
    echo "($SECONDS s) reject new bug with $thiscls clauses"
    continue
  fi
  
  bestcls=$thiscls # store better clause count!
  echo "($SECONDS s) set bestcls to $bestcls"
  red=red-$seed.cnf
  bug=bug-$seed.cnf
  mv $cnf $bug
  #if [ x"$qbf" = x ]
  #then

#  ./cnfdd $bug $red ./toolCheck.sh $prg 1>/dev/null 2>/dev/null

  #else
  #  qbfdd.py -v -v -o $red $bug $prg 1>/dev/null 2>/dev/null
  #fi

  if [ -f "$red" ]; then
    head="`awk '/p cnf /{print $3, $4}' $red`"
    echo "($SECONDS s) [runcnfuzz] $red $head"
  fi
  # rm -f $bug
done

echo "($SECONDS s) "
echo "($SECONDS s) did $test out of $limit tests"

