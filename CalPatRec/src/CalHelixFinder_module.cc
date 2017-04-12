///////////////////////////////////////////////////////////////////////////////
// Calorimeter-driven track finding
// Pattern recognition only, passes results to CalSeedFit
// P.Murat, G.Pezzullo
// try to order routines alphabetically
///////////////////////////////////////////////////////////////////////////////
#include "fhiclcpp/ParameterSet.h"

#include "CalPatRec/inc/CalHelixFinder_module.hh"

// framework
#include "art/Framework/Principal/Handle.h"
#include "GeometryService/inc/GeomHandle.hh"
#include "GeometryService/inc/DetectorSystem.hh"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Services/Optional/TFileService.h"

// conditions
#include "ConditionsService/inc/AcceleratorParams.hh"
#include "ConditionsService/inc/ConditionsHandle.hh"
#include "ConditionsService/inc/TrackerCalibrations.hh"
#include "TTrackerGeom/inc/TTracker.hh"
#include "CalorimeterGeom/inc/DiskCalorimeter.hh"
#include "ConfigTools/inc/ConfigFileLookupPolicy.hh"
#include "CalPatRec/inc/KalFitResult.hh"
#include "RecoDataProducts/inc/StrawHitIndex.hh"

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/median.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/moment.hpp>
#include <boost/algorithm/string.hpp>

#include "CalPatRec/inc/CalHelixFinderData.hh"

#include "CalPatRec/inc/CprModuleHistBase.hh"
#include "art/Utilities/make_tool.h"

#include "TVector2.h"
#include "TSystem.h"
#include "TInterpreter.h"

using namespace std;
using namespace boost::accumulators;
using CLHEP::HepVector;
using CLHEP::Hep3Vector;

namespace mu2e {
//-----------------------------------------------------------------------------
// comparison functor for sorting by Z(wire)
//-----------------------------------------------------------------------------
  struct straw_zcomp : public binary_function<StrawHitIndex,StrawHitIndex,bool> {
    bool operator()(StrawHitIndex const& h1, StrawHitIndex const& h2) {

      mu2e::GeomHandle<mu2e::TTracker> handle;
      const TTracker* t = handle.get();
      const Straw* s1 = &t->getStraw(StrawIndex(h1));
      const Straw* s2 = &t->getStraw(StrawIndex(h2));

      return s1->getMidPoint().z() < s2->getMidPoint().z();
    }
  }; // a semicolumn here is required

//-----------------------------------------------------------------------------
// module constructor, parameter defaults are defiend in CalPatRec/fcl/prolog.fcl
//-----------------------------------------------------------------------------
  CalHelixFinder::CalHelixFinder(fhicl::ParameterSet const& pset) :
    _diagLevel   (pset.get<int>   ("diagLevel"                      )),
    _debugLevel  (pset.get<int>   ("debugLevel"                     )),
    _printfreq   (pset.get<int>   ("printFrequency"                 )),
    _useAsFilter (pset.get<int>   ("useAsFilter"                    )),    
    _shLabel     (pset.get<string>("StrawHitCollectionLabel"        )),
    _shpLabel    (pset.get<string>("StrawHitPositionCollectionLabel")),
    _shfLabel    (pset.get<string>("StrawHitFlagCollectionLabel"    )),
    _timeclLabel (pset.get<string>("TimeClusterCollectionLabel"       )),
    _tpart       ((TrkParticle::type)(pset.get<int>("fitparticle"))),
    _fdir        ((TrkFitDirection::FitDirection)(pset.get<int>("fitdirection"))),
    _hfinder     (pset.get<fhicl::ParameterSet>("HelixFinderAlg",fhicl::ParameterSet()))
  {
    produces<HelixSeedCollection>();
//-----------------------------------------------------------------------------
// provide for interactive disanostics
//-----------------------------------------------------------------------------
    _helTraj          = 0;
    _timeOffsets      = new fhicl::ParameterSet(pset.get<fhicl::ParameterSet>("TimeOffsets",fhicl::ParameterSet()));

    _data.shLabel     = _shLabel;
    _data.timeOffsets = _timeOffsets;

    if (_debugLevel != 0) _printfreq = 1;

    if (_diagLevel != 0) _hmanager = art::make_tool<CprModuleHistBase>(pset.get<fhicl::ParameterSet>("histograms"));
    else                 _hmanager = std::make_unique<CprModuleHistBase>();

  }

//-----------------------------------------------------------------------------
// destructor
//-----------------------------------------------------------------------------
  CalHelixFinder::~CalHelixFinder() {
    if (_helTraj) delete _helTraj;
    delete _timeOffsets;
  }

//-----------------------------------------------------------------------------
  void CalHelixFinder::beginJob(){
    art::ServiceHandle<art::TFileService> tfs;
    _hmanager->bookHistograms(tfs,&_hist);
  }

//-----------------------------------------------------------------------------
  bool CalHelixFinder::beginRun(art::Run& ) {
    mu2e::GeomHandle<mu2e::TTracker> th;
    _tracker = th.get();

    mu2e::GeomHandle<mu2e::Calorimeter> ch;
    _calorimeter = ch.get();
					// calibrations

    mu2e::ConditionsHandle<TrackerCalibrations> tcal("ignored");
    _trackerCalib = tcal.operator ->();

    _hfinder.setTracker    (_tracker);
    _hfinder.setCalorimeter(_calorimeter);

    return true;
  }

//-----------------------------------------------------------------------------
// find the input data objects
//-----------------------------------------------------------------------------
  bool CalHelixFinder::findData(const art::Event& evt) {

    //    art::Handle<mu2e::StrawHitCollection> strawhitsH;
    if (evt.getByLabel(_shLabel, _strawhitsH)) {
      _shcol = _strawhitsH.product();
    }
    else {
      _shcol  = 0;
      printf(" >>> ERROR in CalHelixFinder::findData: StrawHitCollection with label=%s not found.\n",
             _shLabel.data());
    }

    art::Handle<mu2e::StrawHitPositionCollection> shposH;
    if (evt.getByLabel(_shpLabel,shposH)) {
      _shpcol = shposH.product();
    }
    else {
      _shpcol = 0;
      printf(" >>> ERROR in CalHelixFinder::findData: StrawHitPositionCollection with label=%s not found.\n",
             _shpLabel.data());
    }

    art::Handle<mu2e::StrawHitFlagCollection> shflagH;
    if (evt.getByLabel(_shfLabel,shflagH)) {
      _shfcol = shflagH.product();
    }
    else {
      _shfcol = 0;
      printf(" >>> ERROR in CalHelixFinder::findData: StrawHitFlagCollection with label=%s not found.\n",
             _shfLabel.data());
    }

    if (evt.getByLabel(_timeclLabel, _timeclcolH)) {
      _timeclcol = _timeclcolH.product();
    }
    else {
      _timeclcol = 0;
      printf(" >>> ERROR in CalHelixFinder::findData: TimeClusterCollection with label=%s not found.\n",
             _timeclLabel.data());
    }
//-----------------------------------------------------------------------------
// done
//-----------------------------------------------------------------------------
    return (_shcol != 0) && (_shfcol != 0) && (_shpcol != 0) && (_timeclcol != 0);
  }

//-----------------------------------------------------------------------------
// event entry point
//-----------------------------------------------------------------------------
  bool CalHelixFinder::filter(art::Event& event ) {
    const char*             oname = "CalHelixFinder::filter";
    int                     npeaks;
    HelixSeed               helix_seed;
    CalHelixFinderData      hf_result;

    
    //    static StrawHitFlag     esel(StrawHitFlag::energysel), flag;
    
					// diagnostic info
    _data.event     = &event;
    _data.nseeds[0] = 0;
    _data.nseeds[1] = 0;
    _iev            = event.id().event();

    if ((_iev%_printfreq) == 0) printf("[%s] : START event number %8i\n", oname,_iev);

    unique_ptr<HelixSeedCollection>    outseeds(new HelixSeedCollection);
//-----------------------------------------------------------------------------
// find the data
//-----------------------------------------------------------------------------
    if (!findData(event)) {
      printf("%s ERROR: No straw hits found, RETURN\n", oname);
                                                            goto END;
    }
//-----------------------------------------------------------------------------
// loop over found time peaks - for us, - "eligible" calorimeter clusters
//-----------------------------------------------------------------------------
    hf_result._tpart  = _tpart;
    hf_result._fdir   = _fdir;
    hf_result._shcol  = _shcol;
    hf_result._shpos  = _shpcol;
    hf_result._shfcol = _shfcol;
   
    npeaks = _timeclcol->size();
    for (int ipeak=0; ipeak<npeaks; ipeak++) {
      const TimeCluster* tc = &_timeclcol->at(ipeak);
//-----------------------------------------------------------------------------
// create track definitions for the helix fit from this initial information
// track fitting objects for this peak
//-----------------------------------------------------------------------------
      hf_result._timeCluster    = tc;
      hf_result._timeClusterPtr = art::Ptr<mu2e::TimeCluster>(_timeclcolH,ipeak);
//-----------------------------------------------------------------------------
// Step 1: pattern recognition. Find initial helical approximation of a track
//-----------------------------------------------------------------------------
      int rc = _hfinder.findHelix(hf_result,tc);

      if (rc) {
//-----------------------------------------------------------------------------
// fill seed information
//-----------------------------------------------------------------------------
	initHelixSeed(helix_seed, hf_result);
	outseeds->push_back(helix_seed);

	if (_diagLevel > 0) {
//--------------------------------------------------------------------------------    
// fill diagnostic information
//--------------------------------------------------------------------------------
	  int             nhitsMin(15);
	  double          mm2MeV = 3./10.;  // approximately , at B=1T
	  
	  int loc = _data.nseeds[0];
	  if (loc < _data.maxSeeds()) {
	    int nhits          = helix_seed._hhits.size();
	    _data.nhits[loc]   = nhits;
	    _data.radius[loc]  = helix_seed.helix().radius();
	    _data.pT[loc]      = mm2MeV*_data.radius[loc];
	    _data.p[loc]       = _data.pT[loc]/std::cos( std::atan(helix_seed.helix().lambda()/_data.radius[loc]));
	
	    _data.chi2XY[loc]   = hf_result._sxyw.chi2DofCircle();
	    _data.chi2ZPhi[loc] = hf_result._srphi.chi2DofLine();
	    
	    _data.nseeds[0]++;
	    _data.good[loc] = 0;
	    if (nhits >= nhitsMin) {
	      _data.nseeds[1]++;
	      _data.good[loc] = 1;
	    }
	  }
	  else {
	    printf(" N(seeds) > %i, IGNORE SEED\n",_data.maxSeeds());
	  }
	}
      }
    }
//--------------------------------------------------------------------------------    
// fill histograms
//--------------------------------------------------------------------------------
    if (_diagLevel > 0) {
      _hmanager->fillHistograms(0,&_data,&_hist);
    }
//-----------------------------------------------------------------------------
// put reconstructed tracks into the event record
//-----------------------------------------------------------------------------
  END:;
    int    nseeds = outseeds->size();
    event.put(std::move(outseeds));
//-----------------------------------------------------------------------------
// filtering
//-----------------------------------------------------------------------------    
    if (_useAsFilter == 0) return true;
    if (nseeds       >  0) return true;
    else                   return false;
  }

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
  void CalHelixFinder::endJob() {
    // does this cause the file to close?
    art::ServiceHandle<art::TFileService> tfs;
  }

//--------------------------------------------------------------------------------
// set helix parameters
//-----------------------------------------------------------------------------
  void CalHelixFinder::initHelixSeed(HelixSeed& HelSeed, CalHelixFinderData& HfResult) {
    
    HelixTraj* hel = HfResult.helix();

    double   helixRadius   = 1./fabs(hel->omega());
    double   impactParam   = hel->d0();
    double   phi0          = hel->phi0();
    double   x0            = -(helixRadius + impactParam)*sin(phi0);
    double   y0            =  (helixRadius + impactParam)*cos(phi0);
    double   dfdz          = 1./hel->tanDip()/helixRadius;
    double   z0            = hel->z0();
					// center of the helix in the transverse plane
    Hep3Vector center(x0, y0, 0);
    					//define the reconstructed helix parameters

    HelSeed._helix._rcent  = center.perp();
    HelSeed._helix._fcent  = center.phi();
    HelSeed._helix._radius = helixRadius;
    HelSeed._helix._lambda = 1./dfdz;
    HelSeed._helix._fz0    = -z0*dfdz + phi0 - M_PI/2.;
    HelSeed._t0            = TrkT0(HfResult._timeClusterPtr->caloCluster()->time(), 0.1); //dummy error on T0
    HelSeed._timeCluster   = HfResult._timeClusterPtr;
    
    // cluster hits assigned to the reconsturcted Helix

    int nhits = HfResult.nGoodHits();
    for (int i=0; i<nhits; ++i){
      const StrawHitIndex loc        = HfResult._goodhits[i];
      const StrawHitPosition& shpos  = _shpcol->at(loc);
      double              shphi      = shpos.pos().z()*dfdz + phi0;

      HelixHit            hhit(shpos,loc,shphi);
      
      hhit._flag.clear(StrawHitFlag::resolvedphi);
					
      hhit._flag.merge(_shfcol->at(loc)); // merge in other flags, hit Quality no yet assigned
      HelSeed._hhits.push_back(hhit);
    }
  }

//-----------------------------------------------------------------------------
  int CalHelixFinder::initHelixFinderData(CalHelixFinderData&                Data,
					  const TrkParticle&                 TPart,
					  const TrkFitDirection&             FDir,
					  const StrawHitCollection*          StrawCollection ,
					  const StrawHitPositionCollection*  ShPosCollection , 
					  const StrawHitFlagCollection*      ShFlagCollection) {
    Data._fit         = TrkErrCode::fail;
    Data._tpart       = TPart;
    Data._fdir        = FDir;

    Data._shcol       = StrawCollection;
    Data._shpos       = ShPosCollection;
    Data._shfcol      = ShFlagCollection;
    
    Data._radius      = -1.0;
    Data._dfdz        = 0.;
    Data._fz0         = 0.;

    return 0;
  }

}

using mu2e::CalHelixFinder;
DEFINE_ART_MODULE(CalHelixFinder);