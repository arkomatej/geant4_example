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
/// \file RunAction.cc
/// \brief Implementation of the B1::RunAction class

#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4Run.hh"
#include "G4AccumulableManager.hh"
#include "G4LogicalVolumeStore.hh"
#include "G4LogicalVolume.hh"
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4AnalysisManager.hh" // <-- CRITICAL for CSV output

namespace B1
{

RunAction::RunAction()
{
  // Add edep and edep2 to the accumulable manager
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Register(fEdep);
  accumulableManager->Register(fEdep2);

  // --- OUR CUSTOM HISTOGRAM SETUP ---
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetDefaultFileType("csv");
  analysisManager->SetFileName("recoils");
  
  // Histogram 0: PKA Recoil Energy (For the Nuclear Lognormal Tail)
  //analysisManager->CreateH1("PKA", "Recoil Energy (MeV)", 1000, 0.0, 185.0);
  
  // Histogram 1: Coulomb NIEL (For the Gaussian Baseline)
  //analysisManager->CreateH1("NIEL", "Lumped NIEL per Proton (MeV)", 1000, 0.0, 1.0);
  
  // ---------------------------------------------------------
  // NTUPLE 0: Primary Knock-on Atoms (PKAs)
  // ---------------------------------------------------------
  analysisManager->CreateNtuple("PKAs", "Raw Recoil Energies");
  analysisManager->CreateNtupleIColumn("EventID");           // Column 0 (Integer)
  analysisManager->CreateNtupleDColumn("PrimaryEnergy_MeV"); // Column 1 (Double)
  analysisManager->CreateNtupleDColumn("PKA_Energy_MeV");    // Column 2 (Double)
  analysisManager->CreateNtupleDColumn("AtomicNumber_Z");    // Column 3 (Double)
  analysisManager->CreateNtupleIColumn("MassNumber_A");      // Column 5 (Integer)
  analysisManager->CreateNtupleIColumn("ParentID");
  analysisManager->CreateNtupleSColumn("Particle");          // Column 6 (String)
  analysisManager->CreateNtupleSColumn("Process");           // Column 7 (String)
  analysisManager->CreateNtupleDColumn("PositionX_mm");      // Column 8 (Double)
  analysisManager->CreateNtupleDColumn("PositionY_mm");      // Column 9 (Double)
  analysisManager->CreateNtupleDColumn("PositionZ_mm");      // Column 10 (Double)
  analysisManager->FinishNtuple();

  // ---------------------------------------------------------
  // NTUPLE 1: Total NIEL Tracking per Event
  // ---------------------------------------------------------
  //analysisManager->CreateNtuple("EventNIEL", "NIEL Tracking per Event");
  //analysisManager->CreateNtupleDColumn("CoulombNIEL");    // Column 0
  //analysisManager->CreateNtupleDColumn("NuclearNIEL");    // Column 1
  //analysisManager->CreateNtupleDColumn("TotalNIEL");      // Column 2
  //analysisManager->FinishNtuple();
  
}

void RunAction::BeginOfRunAction(const G4Run*)
{
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Reset();

  // Open the CSV file at the start of the run
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run* run)
{
  G4int nofEvents = run->GetNumberOfEvent();
  if (nofEvents == 0) return;

  // Merge accumulables 
  G4AccumulableManager* accumulableManager = G4AccumulableManager::Instance();
  accumulableManager->Merge();

  // Write and close the CSV file at the end of the run
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->Write();
  analysisManager->CloseFile();
}

void RunAction::AddEdep(G4double edep)
{
  fEdep  += edep;
  fEdep2 += edep*edep;
}

} // namespace B1