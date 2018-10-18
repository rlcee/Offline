#include "RecoDataProducts/inc/TriggerFlag.hh"
#include <stdexcept>
#include <iostream>
#include <stdio.h>

namespace mu2e {

  std::string const& TriggerFlagDetail::typeName()
  {
     static std::string type("TriggerFlag");
     return type;
  }

  std::map<std::string,TriggerFlagDetail::mask_type> const& TriggerFlagDetail::bitNames()
  {
     static std::map<std::string,mask_type> bitnames;
     if (bitnames.size()==0)
     {
         bitnames[std::string("PrescaleRandom")]  = bit_to_mask(prescaleRandom);
         bitnames[std::string("HitCluster")]      = bit_to_mask(hitCluster);
         bitnames[std::string("Helix")]      = bit_to_mask(helix);
         bitnames[std::string("Track")]      = bit_to_mask(track);
         bitnames[std::string("CaloCluster")]   = bit_to_mask(caloCluster);
         bitnames[std::string("AnotherTrigger")]  = bit_to_mask(AnotherTrigger);
     }
     return bitnames;
  }

}
