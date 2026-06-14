#!/bin/bash

SCRIPT="./run.sh"

if [ ! -f "$SCRIPT" ]; then
    echo "Errore: $SCRIPT non trovato"
    exit 1
fi

chmod +x "$SCRIPT"

echo "========================================="
echo "TEST PARALLELI"
echo "========================================="

# Coppie (P_R P_C)
CONFIGS=(
    "2 2"
    "3 3"
    "4 4"
)

# Kernel da testare
KERNELS=(1 2 3 4)

for CONF in "${CONFIGS[@]}"
do
    read P_R P_C <<< "$CONF"

    for KERNEL in "${KERNELS[@]}"
    do
        NP=$((P_R * P_C))

        echo ""
        echo "#########################################"
        echo "Esecuzione:"
        echo "P_r=$P_R | P_c=$P_C | NP=$NP | KERNEL=$KERNEL"
        echo "#########################################"
        echo ""

        $SCRIPT $P_R $P_C $KERNEL

        echo ""
    done
done

echo "========================================="
echo "FINE TEST"
echo "========================================="
