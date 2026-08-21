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
#include "G4AnalysisManager.hh"
#include "G4Track.hh"           
#include "G4ParticleDefinition.hh"
#include "G4VProcess.hh"
#include "G4ThreeVector.hh"

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

  G4double nielDeposit = step->GetNonIonizingEnergyDeposit();

  // if (nielDeposit > 0.0) {
  //     G4String processName = "Unknown";
  //     const G4VProcess* process = step->GetPostStepPoint()->GetProcessDefinedStep();
  //     if (process != nullptr) {
  //         processName = process->GetProcessName();
  //     }

  //     // 1. Is this the Primary Proton that we shot from the particle gun?
  //     if (track->GetTrackID() == 1 && particleDef->GetParticleName() == "proton") {
          
  //         // Did it crash into a nucleus?
  //         if (processName == "hadElastic" || processName.find("Inelastic") != std::string::npos) {
  //             fEventAction->AddNuclearNIEL(nielDeposit);
  //         } 
  //         // Otherwise, it is just flying through the lattice (Coulomb drag)
  //         else {
  //             fEventAction->AddCoulombNIEL(nielDeposit);
  //         }
  //     } 
  //     // 2. Is this a Secondary Particle? (Alphas, Neutrons, Heavy Recoils, etc.)
  //     else {
  //         // All secondaries are born from nuclear events, so ALL their NIEL is Hadronic!
  //         fEventAction->AddNuclearNIEL(nielDeposit);
  //     }
  // }

  // 2. Catch the Nuclear Spallation Recoils (PKAs)
  // ParentID > 0 means it's a secondary. Step 1 means it was just born.
  if (track->GetParentID() > 0 && track->GetCurrentStepNumber() == 1) {
      if (particleDef->GetBaryonNumber() > 4) {
          G4double kinEnergy = track->GetKineticEnergy();
          G4double atomicNumber = particleDef->GetAtomicNumber();
          G4int parentID = track->GetParentID();
          G4int massNumber = particleDef->GetBaryonNumber();
          G4String particleName = particleDef->GetParticleName();

          G4String creatorProcessName = "Unknown";
          const G4VProcess* creatorProcess = track->GetCreatorProcess();
          if (creatorProcess != nullptr) {
              creatorProcessName = creatorProcess->GetProcessName();
          }

          G4ThreeVector position = track->GetPosition();

          if (kinEnergy > 0.0){
            
            // Get the current Event ID and the Primary Proton Energy
            G4int eventID = G4RunManager::GetRunManager()->GetCurrentEvent()->GetEventID();
            G4double primaryEnergy = fEventAction->GetPrimaryEnergy();

            // Fill Ntuple 0 (PKAs)
            analysisManager->FillNtupleIColumn(0, 0, eventID);
            analysisManager->FillNtupleDColumn(0, 1, primaryEnergy);
            analysisManager->FillNtupleDColumn(0, 2, kinEnergy);
            analysisManager->FillNtupleDColumn(0, 3, atomicNumber);
            analysisManager->FillNtupleIColumn(0, 4, parentID);
            analysisManager->FillNtupleIColumn(0, 5, massNumber);
            analysisManager->FillNtupleSColumn(0, 6, particleName);
            analysisManager->FillNtupleSColumn(0, 7, creatorProcessName);
            
            // ADD THESE FILL COMMANDS:
            analysisManager->FillNtupleDColumn(0, 8, position.x());
            analysisManager->FillNtupleDColumn(0, 9, position.y());
            analysisManager->FillNtupleDColumn(0, 10, position.z());
            analysisManager->AddNtupleRow();
          }
      }
  }

  // --- DEFAULT B1 BEHAVIOR (Total Energy Deposit) ---
  G4double edep = step->GetTotalEnergyDeposit();
  fEventAction->AddEdep(edep);
}

} // namespace B1