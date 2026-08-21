#!/bin/bash

source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

NUM_PROTONS=100000000
ENERGIES=(10 30 60 100 185 500 1000 5000)
MODELS=(BERT BERT_HP BIC BIC_HP)
OUTDIR=results

mkdir -p $OUTDIR

for E in "${ENERGIES[@]}"; do
    for MODEL in "${MODELS[@]}"; do
        TAG=E${E}MeV_MSC_${MODEL}
        echo "====================================================="
        echo "Starting run: $TAG"
        echo "====================================================="

        cat > temp_run.mac <<EOF
/run/initialize
/analysis/setFileName ${TAG}
/run/printProgress 1000000
/gps/particle proton
/gps/pos/type Plane
/gps/pos/shape Square
/gps/pos/halfx 0.5 cm
/gps/pos/halfy 0.5 cm
/gps/pos/centre 0 0 -4 cm
/gps/direction 0 0 1
/gps/energy ${E} MeV
/run/beamOn ${NUM_PROTONS}
EOF

        if ! build/exampleB1 temp_run.mac MSC "$MODEL"; then
            echo "!!! Run FAILED for $TAG" >&2
            exit 1
        fi

        mv ${TAG}_nt_*.csv $OUTDIR/
    done
done

rm -f temp_run.mac
echo "All simulations completed successfully!"
