//
//  Utility class to print TrackSummary
// 
#ifndef Print_inc_TrackSummaryPrinter_hh
#define Print_inc_TrackSummaryPrinter_hh

#include <cstring>
#include <iostream>

#include "CLHEP/Vector/ThreeVector.h"

#include "Print/inc/ProductPrinter.hh"
#include "RecoDataProducts/inc/TrackSummary.hh"
#include "art/Framework/Principal/Handle.h"
#include "canvas/Persistency/Common/Ptr.h"

namespace mu2e {

  class TrackSummaryPrinter : public ProductPrinter {
  public:

    typedef std::vector<std::string> vecstr;

    TrackSummaryPrinter() { set( fhicl::ParameterSet() ); }
    TrackSummaryPrinter(const fhicl::ParameterSet& pset) { set(pset); }

    // tags to select which product instances to process
    void setTags(const vecstr& tags) { _tags = tags; }

    // pset should contain a table called TrackSummaryPrinter
    void set(const fhicl::ParameterSet& pset);

    // the vector<string> list of inputTags
    const vecstr& tags() const {return _tags; }

    // all the ways to request a printout
    void Print(art::Event const& event,
	       std::ostream& os = std::cout) override;
    void Print(const art::Handle<TrackSummaryCollection>& handle, 
	       std::ostream& os = std::cout);
    void Print(const art::ValidHandle<TrackSummaryCollection>& handle, 
	       std::ostream& os = std::cout);
    void Print(const TrackSummaryCollection& coll, 
	       std::ostream& os = std::cout);
    void Print(const art::Ptr<TrackSummary>& ptr, 
	       int ind = -1, std::ostream& os = std::cout);
    void Print(const mu2e::TrackSummary& obj, 
	       int ind = -1, std::ostream& os = std::cout);

    void PrintHeader(const std::string& tag, 
		     std::ostream& os = std::cout);
    void PrintListHeader(std::ostream& os = std::cout);

  private:

    vecstr _tags;

  };

}
#endif