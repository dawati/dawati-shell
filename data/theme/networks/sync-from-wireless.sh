#! /bin/sh

for tech in 3g bluetooth wimax; do
    for strength in strong weak; do
        echo $tech $strength
        cp network-wireless-$strength.png network-$tech-$strength.png
        cp network-wireless-$strength-hover.png network-$tech-$strength-hover.png
    done
done
