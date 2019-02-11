//
// This module merges the TriggerInfo results from the different Trigger paths
// it produces also a TriggerAlgCollection 
// Original author G. Pezzullo (Yale)
// 
//
// framework
#include "art/Framework/Principal/Event.h"
#include "fhiclcpp/ParameterSet.h"
#include "art/Framework/Principal/Handle.h"
#include "GeometryService/inc/GeomHandle.hh"
#include "art/Framework/Core/EDProducer.h"
#include "GeometryService/inc/DetectorSystem.hh"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileService.h"

// conditions
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/AcceleratorParams.hh"

//Dataproducts
#include "RecoDataProducts/inc/TriggerAlg.hh"
#include "RecoDataProducts/inc/TriggerInfo.hh"

#include "TH1F.h"

#include <string>
#include <vector>
#include <iostream>
#include <memory>



namespace mu2e {

  class TriggerInfoMerger : public art::EDProducer
  {
  public:
    explicit TriggerInfoMerger(fhicl::ParameterSet const& pset);
    virtual void produce (art::Event& e);
    virtual void beginRun(art::Run& run );
    virtual void beginJob();


  private:
    int    _diagLevel;

    // diagnostic
    TH1F*  _maxiter;

  };

  TriggerInfoMerger::TriggerInfoMerger(fhicl::ParameterSet const& pset) :
    _diagLevel(pset.get<int>("diagLevel",0))
  {
    produces<TriggerInfoCollection>();
    produces<TriggerAlg>();

    if (_diagLevel > 0) std::cout << "In TriggerInfoMerger constructor " << std::endl;
  }

  //------------------------------------------------------------------------------------------
  void TriggerInfoMerger::beginJob()
  {
    if(_diagLevel > 0){
      art::ServiceHandle<art::TFileService> tfs;
      _maxiter   = tfs->make<TH1F>( "maxiter",  "ADC max",16,-0.5,15.5 );
    }
  }

  void TriggerInfoMerger::beginRun(art::Run& run){ }

  //------------------------------------------------------------------------------------------
  void TriggerInfoMerger::produce(art::Event& event)
  {
    if (_diagLevel > 0) std::cout << "In TriggerInfoMerger produce " << std::endl;

    //create the output collections
    std::unique_ptr<TriggerInfoCollection> trigInfoCol(new TriggerInfoCollection);
    std::unique_ptr<TriggerAlg>            trigAlg    (new TriggerAlg);

    std::vector<art::Handle<TriggerInfo> > hTrigInfoVec;
    event.getManyByType(hTrigInfoVec);
    art::Handle<TriggerInfo>               hTrigInfo;

    //now loop pver the TriggerInfo objects generated by all the trigger paths
    for (size_t i=0; i<hTrigInfoVec.size(); ++i){
      hTrigInfo = hTrigInfoVec.at(i);
      if (!hTrigInfo.isValid())             continue;
      TriggerInfo trigInfo  = *(hTrigInfo.product());
      //      const TriggerFlag  flag      = trigInfo->triggerBits();
      TriggerAlg  alg       = trigInfo.triggerAlgBits();

      trigInfoCol->push_back(trigInfo);
      trigAlg->merge(alg);
    }

    event.put(std::move(trigInfoCol));
    event.put(std::move(trigAlg));
  }



}


using mu2e::TriggerInfoMerger;
DEFINE_ART_MODULE(TriggerInfoMerger);