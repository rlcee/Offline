#ifndef Mu2eG4_toggleProcesses_hh
#define Mu2eG4_toggleProcesses_hh
//
// Function that handles the switching on and off of G4 processes.  This
// is handled through the configuration files and includes the following
// commands:
//
// g4.noDecay - turns off decays of specified particles
//
//-----------------------------------------------------------------------------

namespace fhicl { class ParameterSet; }

namespace mu2e{
  class SimpleConfig;

  void switchDecayOff( const SimpleConfig& config  );
  void switchDecayOff( const fhicl::ParameterSet& pset);

  void switchCaptureDModel( const SimpleConfig& config);
  void switchCaptureDModel( const fhicl::ParameterSet& pset);

  void addUserProcesses( const SimpleConfig& config);
  void addUserProcesses( const fhicl::ParameterSet& pset);

}  // end namespace mu2e

#endif /* Mu2eG4_toggleProcesses_hh */
