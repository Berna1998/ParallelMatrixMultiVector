#!/bin/bash

P_R=$1
P_C=$2
KERNEL=$3

# valori di default
[ -z "$P_R" ] && P_R=2
[ -z "$P_C" ] && P_C=2
[ -z "$KERNEL" ] && KERNEL=1

# compila una volta
make

for SIZE in 1024 2048 4096 8192 16384
do
    M=$((SIZE * 3))
    N=$SIZE

    NP=$((P_R * P_C))

    echo "===================================="
    echo "M=$M | N=$N | P_r=$P_R | P_c=$P_C | NP=$NP | KERNEL=$KERNEL"
    echo "===================================="

    for K in 3 6 8 20 32
    do
        echo "------ K = $K ------"

        make run_parallelo \
            ARGS="$M $N $K $P_R $P_C $KERNEL" \
            P_R=$P_R P_C=$P_C

        echo ""
    done

    echo ""
done

for SIZE in 1024 2048 4096 8192 16384
do
    M=$SIZE
    N=$SIZE

    NP=$((P_R * P_C))

    echo "===================================="
    echo "M=$M | N=$N | P_r=$P_R | P_c=$P_C | NP=$NP | KERNEL=$KERNEL"
    echo "===================================="

    for K in 3 6 8 20 32
    do
        echo "------ K = $K ------"

        make run_parallelo \
            ARGS="$M $N $K $P_R $P_C $KERNEL" \
            P_R=$P_R P_C=$P_C

        echo ""
    done

    echo ""
done

for SIZE in 1024 2048 4096 8192 16384
do
    N=$((SIZE * 2))
    M=$SIZE

    NP=$((P_R * P_C))

    echo "===================================="
    echo "M=$M | N=$N | P_r=$P_R | P_c=$P_C | NP=$NP | KERNEL=$KERNEL"
    echo "===================================="

    for K in 3 6 8 20 32
    do
        echo "------ K = $K ------"

        make run_parallelo \
            ARGS="$M $N $K $P_R $P_C $KERNEL" \
            P_R=$P_R P_C=$P_C

        echo ""
    done

    echo ""
done
