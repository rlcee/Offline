#ifndef MakeCrvRecoPulses_h
#define MakeCrvRecoPulses_h

#include <vector>

namespace mu2eCrv
{

class MakeCrvRecoPulses
{
  private:
  MakeCrvRecoPulses();

  public:
  MakeCrvRecoPulses(double scale, double offset, bool useFittedPulseHeight, bool useFittedPulseTime, bool doLEfit);
  void         SetWaveform(const std::vector<double> &waveform, double startTime, double binWidth);
  unsigned int GetNPulses();
  int          GetPEs(int pulse);
  double       GetPulseTime(int pulse);
  double       GetPulseHeight(int pulse);
  double       GetPulseWidth(int pulse);
  double       GetPulseFitChi2(int pulse);
  double       GetFitParam0(int pulse);
  double       GetFitParam1(int pulse);
  double       GetFitParam2(int pulse);
  double       GetT1(int pulse);
  double       GetT2(int pulse);
  double       GetLEtime(int pulse);
  double       GetLEfitChi2(int pulse);

  private:
  double _scale, _offset;
  bool   _useFittedPulseHeight, _useFittedPulseTime, _doLEfit;

  std::vector<int>    _PEs;
  std::vector<double> _pulseTimes, _pulseHeights, _pulseWidths, _pulseFitChi2s;
  std::vector<double> _fitParams0, _fitParams1, _fitParams2, _t1s, _t2s;
  std::vector<double> _LEtimes, _LEfitChi2s;
};

}

#endif
