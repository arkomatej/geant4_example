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
// Selectable via the third command-line argument (HADRON below); paper's
// stated choice is Bertini (0-9.9 GeV), so QGSP_BERT stays the default.
#include "QGSP_BERT.hh"
#include "QGSP_BERT_HP.hh"
#include "QGSP_BIC.hh"
#include "QGSP_BIC_HP.hh"

#include "G4EmStandardPhysicsSS.hh"
#include "G4IonElasticPhysics.hh"
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
  // Hadronic inelastic model. "BERT" (default) is what the paper specifies
  // (Bertini, 0-9.9 GeV). "BIC" swaps in the Binary Cascade for a direct
  // comparison against the earlier QGSP_BIC_HP runs. The "_HP" variants add
  // high-precision (ENDF-based) neutron transport below 20 MeV on top of
  // either cascade, without changing which model handles the primary
  // proton-nucleus inelastic collision.
  G4String hadronOpt = "BERT";

  if (argc == 2) macro = argv[1];
  if (argc == 3) {
      macro = argv[1];
      physOpt = argv[2];
  }
  if (argc == 4) {
      macro = argv[1];
      physOpt = argv[2];
      hadronOpt = argv[3];
  }

  G4int precision = 4;
  G4SteppingVerbose::UseBestUnit(precision);

  auto runManager = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);

  runManager->SetUserInitialization(new DetectorConstruction());

  // 1. Set base physics list. QGSP_BERT (the paper's stated choice) and its
  // siblings already bundle G4EmStandardPhysics, G4HadronElasticPhysics and
  // G4IonPhysics, so re-registering those below is redundant:
  // G4VModularPhysicsList::RegisterPhysics rejects a constructor whose physics
  // type is already present.
  G4VModularPhysicsList* physicsList = nullptr;
  if (hadronOpt == "BERT") {
    physicsList = new QGSP_BERT;
  }
  else if (hadronOpt == "BERT_HP") {
    physicsList = new QGSP_BERT_HP;
  }
  else if (hadronOpt == "BIC") {
    physicsList = new QGSP_BIC;
  }
  else if (hadronOpt == "BIC_HP") {
    physicsList = new QGSP_BIC_HP;
  }
  else {
    G4cerr << "Unknown hadronic option '" << hadronOpt
           << "', expected BERT|BERT_HP|BIC|BIC_HP. Defaulting to BERT."
           << G4endl;
    hadronOpt = "BERT";
    physicsList = new QGSP_BERT;
  }
  G4cout << "==== HADRONIC INELASTIC MODEL: " << hadronOpt << " ====" << G4endl;

  // G4HadronElasticPhysics covers protons, neutrons, mesons and the light ions,
  // but elastic scattering of the *generic* ion comes from G4IonElasticPhysics,
  // which QGSP_BERT does not appear to bundle. The earlier QGSP_BIC_HP build
  // registered it explicitly, so keep it to avoid losing elastic scattering of
  // the heavy recoils. If the base list does already provide it this is a
  // harmless no-op (RegisterPhysics warns and skips). Confirm either way in the
  // "Hadronic Processes for GenericIon" block of the run output: it should list
  // a "Process: ionElastic".
  physicsList->RegisterPhysics(new G4IonElasticPhysics());

  // 2. Honour the second command-line argument: "SS" swaps the default
  // multiple-scattering EM physics for single scattering. Without this the
  // "MSC" argument passed by run_master.sh is dead and the _MSC/_SS suffixes
  // on the output files would be meaningless.
  if (physOpt == "SS") {
    G4cout << "==== USING SINGLE SCATTERING ====" << G4endl;
    physicsList->ReplacePhysics(new G4EmStandardPhysicsSS());
  }
  else {
    G4cout << "==== USING MULTIPLE SCATTERING ====" << G4endl;
  }

  // 3. Register only the genuinely new physics from the paper: the
  // G4ScreenedNuclearRecoil nuclear-stopping process for protons.
  physicsList->RegisterPhysics(new CustomEMPhysics());

  runManager->SetUserInitialization(physicsList);

  // 4. Register the user actions. Without this the PrimaryGeneratorAction is
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
