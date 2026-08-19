//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// ********************************************************************
//
/// \file DetectorConstruction.cc
/// \brief Implementation of the B1::DetectorConstruction class

#include "DetectorConstruction.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4Element.hh"
#include "G4Material.hh"

namespace B1
{

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  // 1. Get nist material manager
  G4NistManager* nist = G4NistManager::Instance();
  
  // 2. Define the Elements
  G4Element* elIn = nist->FindOrBuildElement("In");
  G4Element* elGa = nist->FindOrBuildElement("Ga");
  G4Element* elAs = nist->FindOrBuildElement("As");
    
  // 3. Build the InGaAs Material (Density: 5.8 g/cm3, 3 components)
  // In_0.53 Ga_0.47 As_1.0
  G4Material* InGaAs = new G4Material("InGaAs", 5.8*g/cm3, 3);
  InGaAs->AddElement(elIn, 53);
  InGaAs->AddElement(elGa, 47);
  InGaAs->AddElement(elAs, 100);

  // Option to switch on/off checking of volumes overlaps
  G4bool checkOverlaps = true;

  //
  // World Volume (A vacuum box to hold everything)
  //
  G4double world_sizeXY = 10 * cm;
  G4double world_sizeZ  = 10 * cm;
  G4Material* world_mat = nist->FindOrBuildMaterial("G4_Galactic"); // Vacuum is better than Air for this

  auto solidWorld = new G4Box("World", 0.5 * world_sizeXY, 0.5 * world_sizeXY, 0.5 * world_sizeZ); 
  auto logicWorld = new G4LogicalVolume(solidWorld, world_mat, "World"); 

  auto physWorld = new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0, checkOverlaps); 

  //
  // The InGaAs Detector Volume
  //
  // Let's make it a 1cm x 1cm block, 1mm thick (you can adjust this to match your actual sensor chip)
  G4double det_sizeX = 1 * cm;
  G4double det_sizeY = 1 * cm;
  G4double det_thickness = 0.1 * mm; 

  auto solidDetector = new G4Box("Detector", 0.5 * det_sizeX, 0.5 * det_sizeY, 0.5 * det_thickness);
  
  // Assign our custom InGaAs material here!
  auto logicDetector = new G4LogicalVolume(solidDetector, InGaAs, "Detector"); 

  new G4PVPlacement(nullptr,            // no rotation
                    G4ThreeVector(),    // at (0,0,0) in the center of the world
                    logicDetector,      // its logical volume
                    "Detector",         // its name
                    logicWorld,         // its mother volume (the world)
                    false,              // no boolean operation
                    0,                  // copy number
                    checkOverlaps);     // overlaps checking

  // Set the detector as the scoring volume
  fScoringVolume = logicDetector;

  //
  // always return the physical World
  //
  return physWorld;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

}  // namespace B1