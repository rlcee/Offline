//
// Set the G4 minimum range cut as specified in the geometry file.
//
// $Id: setMinimumRangeCut.cc,v 1.2 2012/07/15 22:06:17 kutschke Exp $
// $Author: kutschke $
// $Date: 2012/07/15 22:06:17 $
//

// C++ includes
#include <iostream>

// Framework includes
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

// G4 includes
#include "G4VUserPhysicsList.hh"
#include "G4RegionStore.hh"
#include "G4ProductionCuts.hh"

// Mu2e includes
#include "Mu2eG4/inc/setMinimumRangeCut.hh"

namespace mu2e{

  void setMinimumRangeCut(const fhicl::ParameterSet& pset, G4VUserPhysicsList* mPL){
    double minRangeCut = pset.get<double>("physics.minRangeCut");
    mf::LogInfo("GEOM_MINRANGECUT")
      << "Setting minRange cut to " << minRangeCut << " mm";
    //setCutCmd equivalent:
    mPL->SetDefaultCutValue(minRangeCut);
    mPL->SetCuts(); // does not work for proton production cut when
                    // called during pre_init or after initialization;
                    // the selective SetCutValue does work

    // special cuts per region (this would need to be generalized if we have many regions)
    if (pset.has_key("physics.minRangeCut2")) {
      std::string regName("Calorimeter");
      G4Region* region = G4RegionStore::GetInstance()->GetRegion(regName);
      if (region!=nullptr) {
        G4ProductionCuts* cuts = new G4ProductionCuts();
        double minRangeCut2 = pset.get<double>("physics.minRangeCut2");
        cuts->SetProductionCut(minRangeCut2); // same cut for gamma, e- and e+, proton/ions
        double protonProductionCut = pset.get<double>("physics.protonProductionCut");
        cuts->SetProductionCut(protonProductionCut,"proton");
        if (pset.get<int>("debug.diagLevel") > 0) {
          std::cout << __func__ << " Setting gamma, e- and e+ production cut for "
                    << regName << " to " << minRangeCut2 << " mm and for proton to "
                    << protonProductionCut << " mm" << std::endl;
        }
        std::cout << __func__ << " Resulting cuts for gamma, e-, e+, proton: " << std::endl;
        for (auto const& rcut : cuts->GetProductionCuts() ) {
          std::cout << " " << rcut;
        }
        std::cout << std::endl;
        region->SetProductionCuts(cuts);
      }
    }
    if (pset.has_key("physics.minRangeCut3")) {
      std::string regName("Tracker");
      G4Region* region = G4RegionStore::GetInstance()->GetRegion(regName);
      if (region!=nullptr) {
        G4ProductionCuts* cuts = new G4ProductionCuts();
        double minRangeCut3 = pset.get<double>("physics.minRangeCut3");
        cuts->SetProductionCut(minRangeCut3); // same cut for gamma, e- and e+, proton/ions
        double protonProductionCut = pset.get<double>("physics.protonProductionCut");
        cuts->SetProductionCut(protonProductionCut,"proton");
        if (pset.get<int>("debug.diagLevel") > 0) {
          std::cout << __func__ << " Setting gamma, e- and e+ production cut for "
                    << regName << " to " << minRangeCut3 << " mm and for proton to "
                    << protonProductionCut  << " mm" << std::endl;
        }
        std::cout << __func__ << " Resulting cuts for gamma, e-, e+, proton: " << std::endl;
        for (auto const& rcut : cuts->GetProductionCuts() ) {
          std::cout << " " << rcut;
        }
        std::cout << std::endl;
        region->SetProductionCuts(cuts);
      }
    }
  }
}  // end namespace mu2e
