#include "CustomEMPhysics.hh"

// Particle definitions
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4Proton.hh"
#include "G4Deuteron.hh"
#include "G4Triton.hh"
#include "G4GenericIon.hh"
#include "G4Gamma.hh"
#include "G4PhysicsListHelper.hh"
#include "G4BuilderType.hh"

// The specific processes mentioned in the paper
#include "G4ScreenedNuclearRecoil.hh"
#include "G4hIonisation.hh"
#include "G4ionIonisation.hh"
#include "G4ComptonScattering.hh"

CustomEMPhysics::CustomEMPhysics(const G4String& name)
    : G4VPhysicsConstructor(name)
{
    SetPhysicsType(bElectromagnetic);
}

CustomEMPhysics::~CustomEMPhysics() {}

void CustomEMPhysics::ConstructParticle()
{
    // Particles are usually constructed by standard lists, 
    // but we can ensure the specific ones we need are built.
    G4Proton::ProtonDefinition();
    G4Deuteron::DeuteronDefinition();
    G4Triton::TritonDefinition();
    G4GenericIon::GenericIonDefinition();
    G4Gamma::GammaDefinition();
}

void CustomEMPhysics::ConstructProcess()
{
    G4PhysicsListHelper* ph = G4PhysicsListHelper::GetPhysicsListHelper();

    // 1. Add G4ScreenedNuclearRecoil to Protons
    // The paper specifies adding this to simulate the nuclear stopping power 
    // of recoil atoms, specifically for protons < 100 MeV[cite: 1].
    G4ParticleDefinition* proton = G4Proton::Proton();
    G4ProcessManager* protonManager = proton->GetProcessManager();
    G4ScreenedNuclearRecoil* screenedRecoil = new G4ScreenedNuclearRecoil();
    // Setting max energy to 100 MeV as specified in the paper
    //screenedRecoil->SetMaxKinEnergy(100.0 * CLHEP::MeV); 
    protonManager->AddDiscreteProcess(screenedRecoil);

    // 2. Add G4HIonisation for Deuterium and Tritium
    // The paper uses G4HIonisation for the ionization of secondary deuterium and tritium[cite: 1].
    G4ParticleDefinition* deuteron = G4Deuteron::Deuteron();
    G4hIonisation* hIonDeuteron = new G4hIonisation();
    ph->RegisterProcess(hIonDeuteron, deuteron);

    G4ParticleDefinition* triton = G4Triton::Triton();
    G4hIonisation* hIonTriton = new G4hIonisation();
    ph->RegisterProcess(hIonTriton, triton);

    // 3. Add G4ionIonisation for Heavy Loads (Generic Ions)
    // The paper uses G4ionIonisation class for heavy load ionization[cite: 1].
    G4ParticleDefinition* genericIon = G4GenericIon::GenericIon();
    G4ionIonisation* ionIonisation = new G4ionIonisation();
    ph->RegisterProcess(ionIonisation, genericIon);

    // 4. Add G4ComptonScattering for Photons
    // The paper adopts G4ComptonScattering class for the elastic interaction of photons[cite: 1].
    G4ParticleDefinition* gamma = G4Gamma::Gamma();
    G4ComptonScattering* comptonScattering = new G4ComptonScattering();
    ph->RegisterProcess(comptonScattering, gamma);
}