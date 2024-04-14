# `xAOD` analysis

1. Apply the MC event weight.
2. Select for events with exactly 2 electrons with $p_{\mathrm{T}} > 10\,\mathrm{GeV}$ and $ \eta < 2.4 $.
3. Compute & plot their di-invariant mass, $ m_{ee} $.

```cpp
#include "AnaQuery/Event.h"
#include "AnaQuery/Hist.h"

#include <xAODEgamma/ElectronContainer.h>
#include <xAODEventInfo/EventInfo.h>

using VecF = ROOT::RVec<float>;
using VecD = ROOT::RVec<double>;

#include "queryosity.h"

using dataflow = queryosity::dataflow;
namespace multithread = queryosity::multithread;
namespace dataset = queryosity::dataset;
namespace column = queryosity::column;
namespace query = queryosity::query;
namespace systematic = queryosity::systematic;

#include "TCanvas.h"
#include "TH1F.h"
#include "TPad.h"
#include <ROOT/RVec.hxx>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>

std::vector<std::string> daodFiles{
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000001.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000002.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000003.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000004.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000005.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000006.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000007.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000008.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000009.pool.root.1",
    "/project/6001378/thpark/public/"
    "mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_"
    "p5855/DAOD_PHYS.35010014._000010.pool.root.1"};
std::string treeName = "CollectionTree";

float EventWeight(const xAOD::EventInfo &eventInfo) {
  return eventInfo.mcEventWeight();
}

class ElectronSelection
    : public column::definition<ConstDataVector<xAOD::ElectronContainer>(
          xAOD::ElectronContainer)> {
public:
  ElectronSelection(double pT_min, double eta_max)
      : m_pT_min(pT_min), m_eta_max(eta_max) {}
  virtual ~ElectronSelection() = default;
  virtual ConstDataVector<xAOD::ElectronContainer>
  evaluate(column::observable<xAOD::ElectronContainer> els) const override {
    ConstDataVector<xAOD::ElectronContainer> els_sel(SG::VIEW_ELEMENTS);
    for (const xAOD::Electron *el : *els) {
      if (el->pt() < m_pT_min)
        continue;
      if (TMath::Abs(el->eta()) > m_eta_max)
        continue;
      els_sel.push_back(el);
    }
    return els_sel;
  }

protected:
  double m_pT_min;
  double m_eta_max;
};

bool TwoElectrons(ConstDataVector<xAOD::ElectronContainer> const &els) {
  return els.size() == 2;
}

float DiElectronsMass(ConstDataVector<xAOD::ElectronContainer> const &els) {
  return (els[0]->p4() + els[1]->p4()).M();
};

int main() {  
  dataflow df(multithread::enable());

  auto ds = df.load(dataset::input<AnaQ::Event>(daodFiles, treeName));
  auto eventInfo = ds.read(dataset::column<xAOD::EventInfo>("EventInfo"));
  auto allElectrons =
      ds.read(dataset::column<xAOD::ElectronContainer>("Electrons"));

  auto selectedElectrons =
      df.define(column::definition<ElectronSelection>(10.0, 1.5))(allElectrons);
  auto diElectronsMassMeV =
      df.define(column::expression(DiElectronsMass))(selectedElectrons);
  auto toGeV = df.define(column::constant(1.0 / 1000.0));
  auto diElectronsMassGeV = diElectronsMassMeV * toGeV;

  auto eventWeight = df.define(column::expression(EventWeight))(eventInfo);
  auto atLeastTwoSelectedElectrons =
      df.weight(eventWeight)
          .filter(column::expression(TwoElectrons))(selectedElectrons);

  auto selectedElectronsPtHist =
      df.get(query::plan<AnaQ::Hist<1, float>>("diElectronMass", 100, 0, 500))
          .fill(diElectronsMassGeV)
          .at(atLeastTwoSelectedElectrons);

  selectedElectronsPtHist->Draw();
  gPad->SetLogy();
  gPad->Print("mee.pdf");

  return 0;
}
```

```{image} ../images/mee.png
```
