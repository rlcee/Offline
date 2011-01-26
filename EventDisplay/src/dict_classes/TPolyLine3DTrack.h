//
// Class which displays tracks (used by Track class). It is inherited from ROOT's TPolyLine3D and the ComponentInfo class which stores specific information for this track. The context menu is overwritten with a menu item allowing the user to display information for this track.
//
// $Id: TPolyLine3DTrack.h,v 1.2 2011/02/03 07:37:03 ehrlich Exp $
// $Author: ehrlich $ 
// $Date: 2011/02/03 07:37:03 $
//
// Original author Ralf Ehrlich
//

#ifndef TPOLYLINE3D_TRACK_H
#define TPOLYLINE3D_TRACK_H

#include <TPolyLine3D.h>
#include "ComponentInfo.h"
#include <TClass.h>
#include <TClassMenuItem.h>

namespace mu2e_eventdisplay
{

class TPolyLine3DTrack : public TPolyLine3D, public ComponentInfo
{
  TPolyLine3DTrack();
  TPolyLine3DTrack(const TPolyLine3DTrack &);
  TPolyLine3DTrack& operator=(const TPolyLine3DTrack &);

  public:
#ifndef __CINT__
  TPolyLine3DTrack(const TObject *mainframe, const boost::shared_ptr<ComponentInfo> info):TPolyLine3D(),ComponentInfo(info) 
  {
    TList  *l=IsA()->GetMenuList();
    TClassMenuItem *m = new TClassMenuItem(TClassMenuItem::kPopupUserFunction,IsA(),"Information","showInfo",const_cast<TObject*>(mainframe),"TObject*",1); //bare pointer needed since ROOT manages this object
    l->Clear();
    l->AddFirst(m);
    IsA()->SetName("Track");
  }
#endif

  virtual ~TPolyLine3DTrack() {}

  ClassDef(TPolyLine3DTrack,0);
};

}
#endif
