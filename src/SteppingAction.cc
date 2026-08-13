//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file SteppingAction.cc
/// \brief Implementation of the B1::SteppingAction class

#include "SteppingAction.hh"
#include "EventAction.hh"
#include "DetectorConstruction.hh"

#include "G4Step.hh"
#include "G4Event.hh"
#include "G4RunManager.hh"
#include "G4LogicalVolume.hh"
#include "G4AnalysisManager.hh" // <-- CRITICAL
#include "G4Track.hh"           // <-- CRITICAL
#include "G4ParticleDefinition.hh"

namespace B1
{

SteppingAction::SteppingAction(EventAction* eventAction)
: fEventAction(eventAction)
{}

void SteppingAction::UserSteppingAction(const G4Step* step)
{
  if (!fScoringVolume) {
    const DetectorConstruction* detConstruction = static_cast<const DetectorConstruction*>
      (G4RunManager::GetRunManager()->GetUserDetectorConstruction());
    fScoringVolume = detConstruction->GetScoringVolume();
  }

  // Get volume of the current step
  G4LogicalVolume* volume = step->GetPreStepPoint()->GetTouchableHandle()->GetVolume()->GetLogicalVolume();

  // Check if we are in the detector
  if (volume != fScoringVolume) return;

  // --- OUR CUSTOM PHYSICS TRACKING ---
  G4Track* track = step->GetTrack();
  G4ParticleDefinition* particleDef = track->GetDefinition();
  auto analysisManager = G4AnalysisManager::Instance();

  // 1. Catch the Coulomb NIEL from the Primary Proton
  if (track->GetParentID() == 0 && particleDef->GetParticleName() == "proton") {
      G4double nielDeposit = step->GetNonIonizingEnergyDeposit();
      if (nielDeposit > 0.0) {
          analysisManager->FillH1(1, nielDeposit); // Fill Histogram 1
      }
  }

  // 2. Catch the Nuclear Spallation Recoils (PKAs)
  // ParentID > 0 means it's a secondary. Step 1 means it was just born.
  if (track->GetParentID() > 0 && track->GetCurrentStepNumber() == 1) {
      // Baryon number > 4 filters out electrons, gammas, protons, and alphas
      if (particleDef->GetBaryonNumber() > 4) {
          G4double kinEnergy = track->GetKineticEnergy();
          G4double atomicNumber = particleDef->GetAtomicNumber();
          if (kinEnergy > 0.0){
            analysisManager->FillH1(0, kinEnergy); // Fill Histogram 0
            // Fill the Ntuple instead of the histogram
            analysisManager->FillNtupleDColumn(0, kinEnergy); // Fill column 0
            analysisManager->FillNtupleDColumn(1, atomicNumber);
            analysisManager->AddNtupleRow();                  // Write the row
          }
      }
  }

  // --- DEFAULT B1 BEHAVIOR (Total Energy Deposit) ---
  G4double edep = step->GetTotalEnergyDeposit();
  fEventAction->AddEdep(edep);
}

} // namespace B1