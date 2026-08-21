#!/bin/bash

source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

# Note: 1,000,000,000 protons will take a long time to run! 
NUM_PROTONS_MSC=100000000

MACROS=("./mac_spectrum.mac")

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


# # Source Geant4 environment
# source /nfs/Tango/users/mateja/geant4/geant4-install/bin/geant4.sh

# # Define number of protons
# NUM_PROTONS_MSC=100000000

# # Define your list of energies in MeV (just separate them with a space)
# ENERGIES=(10 30 60 100 185 500 1000 5000)

# for E in "${ENERGIES[@]}"; do
#     echo "====================================================="
#     echo "Starting run for proton energy: ${E} MeV"
#     echo "====================================================="

#     # Generate the macro file dynamically for this specific energy
#     cat <<EOF > temp_run.mac
# /run/initialize
# /run/printProgress 1000000
# /analysis/setFileName output_${E}MeV_MSC
# /gps/particle proton
# /gps/pos/type Plane
# /gps/pos/shape Square
# /gps/pos/halfx 0.5 cm
# /gps/pos/halfy 0.5 cm
# /gps/pos/centre 0 0 -4 cm
# /gps/direction 0 0 1
# /gps/energy ${E} MeV
# /run/beamOn ${NUM_PROTONS_MSC}
# EOF

#     # Run the simulation
#     echo "-> Running Multiple Scattering (MSC) for ${E} MeV..."
#     build/exampleB1 temp_run.mac MSC

# done

# # Clean up
# rm -f temp_run.mac
# echo "====================================================="
# echo "All energy simulations completed successfully!"