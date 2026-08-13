#!/bin/bash

source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

NUM_PROTONS_MSC=10000000
NUM_PROTONS_SS=10000

MACROS=("../mac_185MeV.mac" "../mac_30MeV.mac" "../mac_spectrum.mac")

for MACRO in "${MACROS[@]}"; do
    echo "====================================================="
    echo "Starting runs for: $MACRO"
    echo "====================================================="

    # 1. Multiple Scattering
    echo "-> Running Multiple Scattering (MSC)..."
    cp $MACRO temp_run.mac
    echo "" >> temp_run.mac
    echo "/analysis/setFileName ${MACRO%.mac}_MSC" >> temp_run.mac
    echo "/run/beamOn $NUM_PROTONS_MSC" >> temp_run.mac
    ./exampleB1 temp_run.mac MSC

    # 2. Single Scattering
    echo "-> Running Single Scattering (SS)..."
    cp $MACRO temp_run.mac
    echo "" >> temp_run.mac
    echo "/analysis/setFileName ${MACRO%.mac}_SS" >> temp_run.mac
    echo "/run/setCut 1 nm" >> temp_run.mac
    echo "/run/beamOn $NUM_PROTONS_SS" >> temp_run.mac
    ./exampleB1 temp_run.mac SS

done

rm temp_run.mac
echo "All simulations completed successfully!"
EOFs