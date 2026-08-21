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
/// \file exampleB1.cc
/// \brief Main program of the basic/B1 example

#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
// Change from QGSP_BIC_HP to QGSP_BERT to use the Bertini inelastic model (0-9.9 GeV)
#include "QGSP_BERT.hh"

#include "G4RunManagerFactory.hh"
#include "G4SteppingVerbose.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

// You will need to create this custom header (explained below)
#include "CustomEMPhysics.hh"

using namespace B1;

int main(int argc, char** argv)
{
  G4UIExecutive* ui = nullptr;
  if (argc == 1) {
    ui = new G4UIExecutive(argc, argv);
  }
  
  G4String macro = "";
  G4String physOpt = "MSC"; 
  
  if (argc == 2) macro = argv[1];
  if (argc == 3) { 
      macro = argv[1]; 
      physOpt = argv[2]; 
  }

  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new DetectorConstruction());

  // 1. Set base physics list to match the Bertini (0-9.9 GeV) inelastic model.
  // QGSP_BERT already bundles G4EmStandardPhysics, G4HadronElasticPhysics
  // (which covers ion elastic scattering) and G4IonPhysics, so re-registering
  // them here is redundant: G4VModularPhysicsList::RegisterPhysics rejects a
  // constructor whose physics type is already present.
  G4VModularPhysicsList* physicsList = new QGSP_BERT;

  // 2. Register only the genuinely new physics from the paper: the
  // G4ScreenedNuclearRecoil nuclear-stopping process for protons.
  physicsList->RegisterPhysics(new CustomEMPhysics());

  runManager->SetUserInitialization(physicsList);

  // 3. Register the user actions. Without this the PrimaryGeneratorAction is
  // never constructed, so G4GeneralParticleSource never exists and none of
  // the /gps/... commands are defined -- which is why the macro used to fail
  // with "COMMAND NOT FOUND </gps/particle proton>". This also supplies the
  // RunAction/EventAction/SteppingAction that write the recoil ntuple.
  runManager->SetUserInitialization(new ActionInitialization());

  auto visManager = new G4VisExecutive(argc, argv);
  visManager->Initialize();
  auto UImanager = G4UImanager::GetUIpointer();

  if (!ui) {
    G4String command = "/control/execute ";
    G4String fileName = argv[1];
    UImanager->ApplyCommand(command + fileName);
  }
  else {
    UImanager->ApplyCommand("/control/execute init_vis.mac");
    ui->SessionStart();
    delete ui;
  }

  delete visManager;
  delete runManager;
  return 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo.....
