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
    // One shared instance serves every projectile species: the process keys its
    // crossSectionHandlers map on the projectile Z, which is exactly why it is
    // designed to be attached to more than one particle.
    G4ScreenedNuclearRecoil* screenedRecoil = new G4ScreenedNuclearRecoil();

    G4ParticleDefinition* proton = G4Proton::Proton();
    G4ProcessManager* protonManager = proton->GetProcessManager();

    // Do NOT add SetMaxEnergyForScattering(100*MeV) here to chase the paper's
    // "energy less than 100 MeV" remark. That setter controls processMaxEnergy,
    // above which GetMeanFreePath returns DBL_MAX and the process is switched
    // off entirely -- so at 185 MeV it would disable screened nuclear recoil for
    // the whole primary track (the proton barely slows in a 0.1 mm target).
    //
    // The 100 MeV figure already exists as the built-in highEnergyLimit: above
    // it the mean free path is simply held constant rather than the process
    // being disabled, which is the behaviour the paper is describing. The
    // constructor defaults (processMaxEnergy = 50 GeV, highEnergyLimit =
    // 100 MeV) are therefore what we want.
    protonManager->AddDiscreteProcess(screenedRecoil);

    // Also attach it to the generic ion. The paper adds this process to model
    // "the nuclear stopping power of the recoil atom", and IsApplicable()
    // accepts anything of particle type "nucleus" while BuildPhysicsTable()
    // explicitly names GenericIon -- so the recoils are the intended target,
    // not just the primary proton.
    //
    // Without this the recoil atoms have only ionIonisation (electronic
    // stopping) and ionInelastic, so a knocked-on Ga/As/In atom simply slows
    // down and never displaces further lattice atoms: no secondary knock-on
    // cascade, and the displacement damage is badly under-counted.
    G4ParticleDefinition* genericIon = G4GenericIon::GenericIon();
    G4ProcessManager* ionManager = genericIon->GetProcessManager();
    ionManager->AddDiscreteProcess(screenedRecoil);
}
