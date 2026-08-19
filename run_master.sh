#!/bin/bash

source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

# Note: 1,000,000,000 protons will take a long time to run! 
NUM_PROTONS_MSC=100000000

MACROS=("./mac_185MeV.mac" "./mac_30MeV.mac" "./mac_spectrum.mac")

for MACRO in "${MACROS[@]}"; do
    echo "====================================================="
    echo "Starting run for: $MACRO"
    echo "====================================================="

    # Extract clean filename without path or extension (e.g., "mac_185MeV")
    BASENAME=$(basename $MACRO .mac)

    echo "-> Running Multiple Scattering (MSC)..."
    cp $MACRO temp_run.mac
    echo "" >> temp_run.mac
    echo "/analysis/setFileName ${BASENAME}_MSC" >> temp_run.mac
    echo "/run/beamOn $NUM_PROTONS_MSC" >> temp_run.mac
    
    build/exampleB1 temp_run.mac MSC
done

rm -f temp_run.mac
echo "All simulations completed successfully!"