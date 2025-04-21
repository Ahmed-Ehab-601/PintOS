# make sure that sha256 is changed
# sudo docker run --rm -it -v "$(pwd):/root/pintos" sha256:47a83b2b7454bcdac14cd8a257f5acd7428d6cab91af51b5c5371a5faad1ccea
docker run --rm -it -v ".:/root/pintos" sha256:47a83b2b7454bcdac14cd8a257f5acd7428d6cab91af51b5c5371a5faad1ccea

# Phase 1 run
cd ~/pintos/src/threads/
make clean
make


#	TEST CASES

# ALARM CLOCK
# pintos run alarm-single
# pintos run alarm-multiple
# pintos run alarm-simultaneous
# pintos run alarm-priority
# pintos run alarm-zero
# pintos run alarm-negative


# PRIORITY SCHEDULER
# pintos run priority-change
# pintos run priority-donate-one
# pintos run priority-donate-multiple
# pintos run priority-donate-multiple2
# pintos run priority-donate-nest
# pintos run priority-donate-sema
# pintos run priority-donate-lower
# pintos run priority-donate-chain
# pintos run priority-fifo
# pintos run priority-preempt
# pintos run priority-sema
# pintos run priority-condvar


# MLFQS
# pintos -q -mlfqs run mlfqs-load-1				
# pintos -q -mlfqs run mlfqs-load-60			passed
# pintos -q -mlfqs run mlfqs-load-avg			passed
# pintos -q -mlfqs run mlfqs-recent-1			
# pintos -q -mlfqs run mlfqs-fair-2 			passed
# pintos -q -mlfqs run mlfqs-fair-20			passed
# pintos -q -mlfqs run mlfqs-nice-2				passed
# pintos -q -mlfqs run mlfqs-nice-10			passed
# pintos -q -mlfqs run mlfqs-block				passed