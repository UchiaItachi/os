if [ "$#" -ne 1 ]; then
    echo "Illegal number of parameters"
    echo "Usage : ./master-test-runner.sh <test#>"
    exit 1
fi

testcase="runtest${1}.py"

CPU_SCHED_PATH=/home/itachi/repositories/gatech/fall_2017/cs6210_aos/cs6210Project1_test/project1_delivery/CPU/vcpu_scheduler 

python $testcase ; echo "Launched test case ${1}" ; sleep 10 ; $CPU_SCHED_PATH 3
