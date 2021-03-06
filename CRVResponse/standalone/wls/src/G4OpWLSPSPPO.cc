/*
GEANT doesn't provide a process which allows multiple absorption and emission spectra, and also uses a quantum yield.
Therefore, two process was developed based of G4OpWLS, one for polystyrene+PPO, and another one for POPOP.
They use different property table names.
*/


#include "G4OpWLSPSPPO.hh"

#include "G4ios.hh"
#include "G4PhysicalConstants.hh"
#include "G4SystemOfUnits.hh"
#include "G4OpProcessSubType.hh"

#include "G4WLSTimeGeneratorProfileDelta.hh"
#include "G4WLSTimeGeneratorProfileExponential.hh"

/////////////////////////
// Class Implementation
/////////////////////////

        //////////////////////
        // static data members
        //////////////////////

/////////////////
// Constructors
/////////////////

G4OpWLSPSPPO::G4OpWLSPSPPO(const G4String& processName, G4ProcessType type)
  : G4VDiscreteProcess(processName, type)
{
  SetProcessSubType(fOpWLS);   //TODO: There isn't a process name for OpWLSPSPPO

  theIntegralTable = NULL;

  WLSTimeGeneratorProfile = 
          new G4WLSTimeGeneratorProfileDelta("WLSTimeGeneratorProfileDelta");
 
  if (verboseLevel>0) G4cout << GetProcessName() << " is created " << G4endl;
}

////////////////
// Destructors
////////////////

G4OpWLSPSPPO::~G4OpWLSPSPPO()
{
  if (theIntegralTable) {
    theIntegralTable->clearAndDestroy();
    delete theIntegralTable;
  }
  delete WLSTimeGeneratorProfile;
}

////////////
// Methods
////////////

// PostStepDoIt
// -------------
//
G4VParticleChange*
G4OpWLSPSPPO::PostStepDoIt(const G4Track& aTrack, const G4Step& aStep)
{
  aParticleChange.Initialize(aTrack);
  
  aParticleChange.ProposeTrackStatus(fStopAndKill);

  if (verboseLevel>0) {
    G4cout << "\n** Photon absorbed! **" << G4endl;
  }
  
  const G4Material* aMaterial = aTrack.GetMaterial();

  G4StepPoint* pPostStepPoint = aStep.GetPostStepPoint();
    
  G4MaterialPropertiesTable* aMaterialPropertiesTable =
    aMaterial->GetMaterialPropertiesTable();
  if (!aMaterialPropertiesTable)
    return G4VDiscreteProcess::PostStepDoIt(aTrack, aStep);

  const G4MaterialPropertyVector* WLS_Intensity = 
    aMaterialPropertiesTable->GetProperty("WLSPSPPOCOMPONENT"); 

  if (!WLS_Intensity)
    return G4VDiscreteProcess::PostStepDoIt(aTrack, aStep);

  G4int NumPhotons = 1;

  const G4MaterialPropertyVector* WLS_QuantumYield = 
    aMaterialPropertiesTable->GetProperty("WLSPSPPOQUANTUMYIELD");

  if (WLS_QuantumYield) 
  {
     const G4DynamicParticle* aParticle = aTrack.GetDynamicParticle();
     G4double thePhotonEnergy = aParticle->GetTotalEnergy();
     G4double quantumYield = WLS_QuantumYield->Value(thePhotonEnergy);

     if (G4UniformRand() > quantumYield) {

        // return unchanged particle and no secondaries

        aParticleChange.SetNumberOfSecondaries(0);

        return G4VDiscreteProcess::PostStepDoIt(aTrack, aStep);

     }

  }

  aParticleChange.SetNumberOfSecondaries(NumPhotons);

  G4double primaryEnergy = aTrack.GetDynamicParticle()->GetKineticEnergy();

  G4int materialIndex = aMaterial->GetIndex();

  // Retrieve the WLS Integral for this material
  // new G4PhysicsOrderedFreeVector allocated to hold CII's

  G4double WLSTime = 0.*ns;
  G4PhysicsOrderedFreeVector* WLSIntegral = 0;

  WLSTime   = aMaterialPropertiesTable->
    GetConstProperty("WLSPSPPOTIMECONSTANT");
  WLSIntegral =
    (G4PhysicsOrderedFreeVector*)((*theIntegralTable)(materialIndex));
   
  // Max WLS Integral
  
  G4double CIImax = WLSIntegral->GetMaxValue();
 
  G4int NumberOfPhotons = NumPhotons;
 
  for (G4int i = 0; i < NumPhotons; i++) {

    G4double sampledEnergy;
    
    // Make sure the energy of the secondary is less than that of the primary

    for (G4int j = 1; j <= 100; j++) {

        // Determine photon energy

        G4double CIIvalue = G4UniformRand()*CIImax;
        sampledEnergy = WLSIntegral->GetEnergy(CIIvalue);

        if (verboseLevel>1) {
           G4cout << "sampledEnergy = " << sampledEnergy << G4endl;
           G4cout << "CIIvalue =      " << CIIvalue << G4endl;
        }

        if (sampledEnergy <= primaryEnergy) break;
    }

    // If no such energy can be sampled, return one less secondary, or none

    if (sampledEnergy > primaryEnergy) {
       if (verboseLevel>1)
       G4cout << " *** One less WLS photon will be returned ***" << G4endl;
       NumberOfPhotons--;
       aParticleChange.SetNumberOfSecondaries(NumberOfPhotons);
       if (NumberOfPhotons == 0) {
          if (verboseLevel>1)
          G4cout << " *** No WLS photon can be sampled for this primary ***"
                 << G4endl;
          // return unchanged particle and no secondaries
          return G4VDiscreteProcess::PostStepDoIt(aTrack, aStep);
       }
       continue;
    }

    // Generate random photon direction
    
    G4double cost = 1. - 2.*G4UniformRand();
    G4double sint = std::sqrt((1.-cost)*(1.+cost));

    G4double phi = twopi*G4UniformRand();
    G4double sinp = std::sin(phi);
    G4double cosp = std::cos(phi);
    
    G4double px = sint*cosp;
    G4double py = sint*sinp;
    G4double pz = cost;
    
    // Create photon momentum direction vector
    
    G4ParticleMomentum photonMomentum(px, py, pz);
    
    // Determine polarization of new photon
    
    G4double sx = cost*cosp;
    G4double sy = cost*sinp;
    G4double sz = -sint;
    
    G4ThreeVector photonPolarization(sx, sy, sz);
    
    G4ThreeVector perp = photonMomentum.cross(photonPolarization);
    
    phi = twopi*G4UniformRand();
    sinp = std::sin(phi);
    cosp = std::cos(phi);
    
    photonPolarization = cosp * photonPolarization + sinp * perp;
    
    photonPolarization = photonPolarization.unit();
    
    // Generate a new photon:
    
    G4DynamicParticle* aWLSPhoton =
      new G4DynamicParticle(G4OpticalPhoton::OpticalPhoton(),
			    photonMomentum);
    aWLSPhoton->SetPolarization
      (photonPolarization.x(),
       photonPolarization.y(),
       photonPolarization.z());
    
    aWLSPhoton->SetKineticEnergy(sampledEnergy);
    
    // Generate new G4Track object:
    
    // Must give position of WLS optical photon

    G4double TimeDelay = WLSTimeGeneratorProfile->GenerateTime(WLSTime);
    G4double aSecondaryTime = (pPostStepPoint->GetGlobalTime()) + TimeDelay;

    G4ThreeVector aSecondaryPosition = pPostStepPoint->GetPosition();

    G4Track* aSecondaryTrack = 
      new G4Track(aWLSPhoton,aSecondaryTime,aSecondaryPosition);
   
    aSecondaryTrack->SetTouchableHandle(aTrack.GetTouchableHandle()); 
    // aSecondaryTrack->SetTouchableHandle((G4VTouchable*)0);
    
    aSecondaryTrack->SetParentID(aTrack.GetTrackID());
    
    aParticleChange.AddSecondary(aSecondaryTrack);
  }

  if (verboseLevel>0) {
    G4cout << "\n Exiting from G4OpWLSPSPPO::DoIt -- NumberOfSecondaries = " 
	   << aParticleChange.GetNumberOfSecondaries() << G4endl;  
  }
  
  return G4VDiscreteProcess::PostStepDoIt(aTrack, aStep);
}

// BuildPhysicsTable for the wavelength shifting process
// --------------------------------------------------

void G4OpWLSPSPPO::BuildPhysicsTable(const G4ParticleDefinition&)
{
  if (theIntegralTable) {
     theIntegralTable->clearAndDestroy();
     delete theIntegralTable;
     theIntegralTable = NULL;
  }

  const G4MaterialTable* theMaterialTable = 
    G4Material::GetMaterialTable();
  G4int numOfMaterials = G4Material::GetNumberOfMaterials();
  
  // create new physics table
  
  theIntegralTable = new G4PhysicsTable(numOfMaterials);
  
  // loop for materials
  
  for (G4int i=0 ; i < numOfMaterials; i++)
    {
      G4PhysicsOrderedFreeVector* aPhysicsOrderedFreeVector =
	new G4PhysicsOrderedFreeVector();
      
      // Retrieve vector of WLS wavelength intensity for
      // the material from the material's optical properties table.
      
      G4Material* aMaterial = (*theMaterialTable)[i];

      G4MaterialPropertiesTable* aMaterialPropertiesTable =
	aMaterial->GetMaterialPropertiesTable();

      if (aMaterialPropertiesTable) {

	G4MaterialPropertyVector* theWLSVector = 
	  aMaterialPropertiesTable->GetProperty("WLSPSPPOCOMPONENT");

	if (theWLSVector) {
	  
	  // Retrieve the first intensity point in vector
	  // of (photon energy, intensity) pairs
	  
	  G4double currentIN = (*theWLSVector)[0];
	  
	  if (currentIN >= 0.0) {

	    // Create first (photon energy) 
	   
	    G4double currentPM = theWLSVector->Energy(0);
	    
	    G4double currentCII = 0.0;
	    
	    aPhysicsOrderedFreeVector->
	      InsertValues(currentPM , currentCII);
	    
	    // Set previous values to current ones prior to loop
	    
	    G4double prevPM  = currentPM;
	    G4double prevCII = currentCII;
	    G4double prevIN  = currentIN;
	    
	    // loop over all (photon energy, intensity)
	    // pairs stored for this material

            for (size_t j = 1;
                 j < theWLSVector->GetVectorLength();
                 j++)	    
	      {
		currentPM = theWLSVector->Energy(j);
		currentIN = (*theWLSVector)[j];
		
		currentCII = 0.5 * (prevIN + currentIN);
		
		currentCII = prevCII +
		  (currentPM - prevPM) * currentCII;
		
		aPhysicsOrderedFreeVector->
		  InsertValues(currentPM, currentCII);
		
		prevPM  = currentPM;
		prevCII = currentCII;
		prevIN  = currentIN;
	      }
	  }
	}
      }
	// The WLS integral for a given material
	// will be inserted in the table according to the
	// position of the material in the material table.

	theIntegralTable->insertAt(i,aPhysicsOrderedFreeVector);
    }
}

// GetMeanFreePath
// ---------------
//
G4double G4OpWLSPSPPO::GetMeanFreePath(const G4Track& aTrack,
 				         G4double ,
				         G4ForceCondition* )
{
  const G4DynamicParticle* aParticle = aTrack.GetDynamicParticle();
  const G4Material* aMaterial = aTrack.GetMaterial();

  G4double thePhotonEnergy = aParticle->GetTotalEnergy();

  G4MaterialPropertiesTable* aMaterialPropertyTable;
  G4MaterialPropertyVector* AttenuationLengthVector;
	
  G4double AttenuationLength = DBL_MAX;

  aMaterialPropertyTable = aMaterial->GetMaterialPropertiesTable();

  if ( aMaterialPropertyTable ) {
    AttenuationLengthVector = aMaterialPropertyTable->
      GetProperty("WLSPSPPOABSLENGTH");
    if ( AttenuationLengthVector ){
      AttenuationLength = AttenuationLengthVector->
	Value(thePhotonEnergy);
    }
    else {
      //             G4cout << "No WLS absorption length specified" << G4endl;
    }
  }
  else {
    //           G4cout << "No WLS absortion length specified" << G4endl;
  }
  
  return AttenuationLength;
}

void G4OpWLSPSPPO::UseTimeProfile(const G4String name)
{
  if (name == "delta")
    {
      delete WLSTimeGeneratorProfile;
      WLSTimeGeneratorProfile = 
             new G4WLSTimeGeneratorProfileDelta("delta");
    }
  else if (name == "exponential")
    {
      delete WLSTimeGeneratorProfile;
      WLSTimeGeneratorProfile =
             new G4WLSTimeGeneratorProfileExponential("exponential");
    }
  else
    {
      G4Exception("G4OpWLSPSPPO::UseTimeProfile", "em0202",
                  FatalException,
                  "generator does not exist");
    }
}
