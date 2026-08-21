#include "CustomEMPhysics.hh"

// Particle definitions
#include "G4ParticleDefinition.hh"
#include "G4ProcessManager.hh"
#include "G4Proton.hh"
#include "G4Deuteron.hh"
#include "G4Triton.hh"
#include "G4GenericIon.hh"
#include "G4Gamma.hh"

// The one process that is genuinely new relative to the base physics list
#include "G4ScreenedNuclearRecoil.hh"

CustomEMPhysics::CustomEMPhysics(const G4String& name)
    : G4VPhysicsConstructor(name)
{
    // Deliberately leave the physics type at the default 0 ("undefined").
    // G4VModularPhysicsList::RegisterPhysics refuses to add a constructor
    // whose non-zero type is already registered, and QGSP_BERT already
    // contains G4EmStandardPhysics with type bElectromagnetic. Declaring
    // bElectromagnetic here would get this constructor silently rejected
    // (a JustWarning), so ConstructProcess() below would never run and
    // G4ScreenedNuclearRecoil would never be attached to the proton.
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
    // Everything else the paper mentions (G4HIonisation for deuteron/triton,
    // G4ionIonisation for the generic ion, G4ComptonScattering for gamma) is
    // already provided by G4EmStandardPhysics, which QGSP_BERT registers
    // internally, so adding those again here would just duplicate processes
    // that the base list already attaches to those particles.
    //
    // The only physics genuinely missing from the base list is the nuclear
    // stopping power of recoil atoms for protons below 100 MeV, so that is
    // the only process added here.
    G4ParticleDefinition* proton = G4Proton::Proton();
    G4ProcessManager* protonManager = proton->GetProcessManager();
    G4ScreenedNuclearRecoil* screenedRecoil = new G4ScreenedNuclearRecoil();
    // Setting max energy to 100 MeV as specified in the paper
    //screenedRecoil->SetMaxKinEnergy(100.0 * CLHEP::MeV);
    protonManager->AddDiscreteProcess(screenedRecoil);
}
