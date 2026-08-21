#!/bin/bash

source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

# Note: 1,000,000,000 protons will take a long time to run!
NUM_PROTONS_MSC=100000000

MACROS=("./mac_185MeV.mac")
#"./mac_30MeV.mac" "./mac_spectrum.mac"

for MACRO in "${MACROS[@]}"; do
    echo "====================================================="
    echo "Starting run for: $MACRO"
    echo "====================================================="

    # Extract clean filename without path or extension (e.g., "mac_185MeV")
    BASENAME=$(basename "$MACRO" .mac)

    echo "-> Running Multiple Scattering (MSC)..."
    cp "$MACRO" temp_run.mac
    echo "" >> temp_run.mac
    # NOTE: the command is /analysis/setFileName. There is no
    # /analysis/file/setName in this Geant4 build -- using it aborts the batch.
    echo "/analysis/setFileName ${BASENAME}_MSC" >> temp_run.mac
    echo "/run/beamOn $NUM_PROTONS_MSC" >> temp_run.mac

    # Check the exit status: Geant4 returns non-zero when a macro command fails,
    # but without this the loop would print "completed successfully" even though
    # the batch was interrupted.
    if ! build/exampleB1 temp_run.mac MSC; then
        echo "!!! Run FAILED for $MACRO -- see the Geant4 output above." >&2
        rm -f temp_run.mac
        exit 1
    fi
done

rm -f temp_run.mac
echo "All simulations completed successfully!"
