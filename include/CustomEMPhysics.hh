#ifndef CustomEMPhysics_h
#define CustomEMPhysics_h 1

#include "G4VPhysicsConstructor.hh"
#include "globals.hh"

class CustomEMPhysics : public G4VPhysicsConstructor
{
public:
    CustomEMPhysics(const G4String& name = "CustomEM");
    virtual ~CustomEMPhysics();

    // This method is required to construct the particles
    virtual void ConstructParticle() override;

    // This method is where we assign the specific processes to the particles
    virtual void ConstructProcess() override;
};

#endif