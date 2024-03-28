@page example Examples
@tableofcontents

@section example-hello-world Hello World

```cpp
#include "queryosity/json.h"
#include "queryosity/hist.h"

#include "queryosity.h"

#include <fstream>
#include <vector>
#include <sstream>

namespace qty = queryosity;

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::linear;

int main() {

	dataflow df( multithread::enable() );

	std::ifstream data("data.json");
	auto [x, w] = df.read( 
		dataset::input<json>(data), 
		dataset::column<std::vector<double>>("x"),
		dataset::column<double>("w") 
		);

	auto zero = df.define( column::constant(0) );
	auto x0 = x[zero];

	auto sel = df.weight(w).filter(
		column::expression([](std::vector<double> const& v){return v.size()}), x
		);

	auto h_x0_w = df.make( 
		query::plan<h1d>( linax(100,0.0,1.0) ) 
		).fill(x0).book(sel).result();

	std::ostringstream os;
	os << *h_x0_w;
	std::cout << os.str() << std::endl;

}
```

@section example-hww ROOT TTree with systematic variations

- Simulated ggF HWW* events: [ATLAS open data](https://opendata.cern.ch/record/3825).
- ROOT extensions for queryosity: [queryosity-hep](https://github.com/taehyounpark/queryosity-hep)

1. Apply the MC event weight.
2. Select entries for which there are exactly two opposite-sign leptons in the event.
3. Separate into different/same-flavour cases for electrons and muons.
4. Apply signal region cuts: @f$E_{\mathrm{T}}^{\mathrm{miss}} > 30\,\mathrm{GeV}@f$ and @f$m_{\ell\ell}< 60\,\mathrm{GeV}@f$.
5. Merge back together to form flavour-inclusive opposite-sign signal region.
6. In each case, plot the distribution of the dilepton+MET transverse momentum.
	- Scale lepton energy scale by +/- 2% as systematic variations.

(The order of selections is needlessly & purposefully convoluted to showcase the cutflow interface)

@cpp
#include "qhep/Hist.h"
#include "qhep/Tree.h"

#include "queryosity/queryosity.h"

namespace qty = queryosity;
using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;
namespace systematic = qty::systematic;

#include "Math/Vector4D.h"
#include "TCanvas.h"
#include "TH1F.h"
#include "TStyle.h"
#include "TVector2.h"
#include <ROOT/RVec.hxx>

using VecF = ROOT::RVec<float>;
using VecD = ROOT::RVec<double>;
using VecI = ROOT::RVec<int>;
using VecUI = ROOT::RVec<unsigned int>;
using P4 = ROOT::Math::PtEtaPhiEVector;

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

class NthP4 : public column::definition<P4(VecD, VecD, VecD, VecD)> {

public:
  NthP4(unsigned int index) : m_index(index) {}
  virtual ~NthP4() = default;

  virtual P4 evaluate(column::observable<VecD> pt, column::observable<VecD> eta,
                      column::observable<VecD> phi,
                      column::observable<VecD> es) const override {
    return P4(pt->at(m_index), eta->at(m_index), phi->at(m_index),
              es->at(m_index));
  }

protected:
  const unsigned int m_index;
};

int main() {

  // the tree doesn't have enough events to multithread
  dataflow df(multithread::disable());

  std::vector<std::string> tree_files{"hww.root"};
  std::string tree_name = "mini";
  auto ds = df.load(dataset::input<Tree>(tree_files, tree_name));

  // weights
  auto mc_weight = ds.read(dataset::column<float>("mcWeight"));
  auto mu_sf = ds.read(dataset::column<float>("scaleFactor_MUON"));
  auto el_sf = ds.read(dataset::column<float>("scaleFactor_ELE"));

  // leptons
  auto [lep_pt_MeV, lep_eta, lep_phi, lep_E_MeV, lep_Q, lep_type] = ds.read(
      dataset::column<VecF>("lep_pt"), dataset::column<VecF>("lep_eta"),
      dataset::column<VecF>("lep_phi"), dataset::column<VecF>("lep_E"),
      dataset::column<VecF>("lep_charge"), dataset::column<VecUI>("lep_type"));

  // missing transverse energy
  auto [met_MeV, met_phi] = ds.read(dataset::column<float>("met_et"),
                                    dataset::column<float>("met_phi"));

  // units
  auto MeV = df.define(column::constant(1000.0));
  auto lep_pt = lep_pt_MeV / MeV;
  auto lep_E = lep_E_MeV / MeV;
  auto met = met_MeV / MeV;

  // vary the energy scale by +/-2%
  auto Escale =
      df.vary(column::expression([](VecD E) { return E; }),
              systematic::variation("eg_up", [](VecD E) { return E * 1.02; }),
              systematic::variation("eg_dn", [](VecD E) { return E * 0.98; }));

  // apply the energy scale (uncertainties)
  // and select ones within acceptance
  auto lep_pt_min = df.define(column::constant(15.0));
  auto lep_eta_max = df.define(column::constant(2.4));
  auto lep_selection = (lep_eta < lep_eta_max) && (lep_eta > (-lep_eta_max)) &&
                       (lep_pt > lep_pt_min);
  auto lep_pt_sel = Escale(lep_pt)[lep_selection];
  auto lep_E_sel = Escale(lep_E)[lep_selection];
  auto lep_eta_sel = lep_eta[lep_selection];
  auto lep_phi_sel = lep_phi[lep_selection];

  // put (sub-)leading lepton into four-momentum
  auto l1p4 = df.define(column::definition<NthP4>(0), lep_pt_sel, lep_eta_sel,
                        lep_phi_sel, lep_E_sel);
  auto l2p4 = df.define(column::definition<NthP4>(1), lep_pt_sel, lep_eta_sel,
                        lep_phi_sel, lep_E_sel);

  // compute dilepton invariant mass & higgs transverse momentum
  auto llp4 = l1p4 + l2p4;
  auto mll =
      df.define(column::expression([](const P4 &p4) { return p4.M(); }), llp4);
  auto higgs_pt =
      df.define(column::expression([](const P4 &p4, float q, float q_phi) {
                  TVector2 p2;
                  p2.SetMagPhi(p4.Pt(), p4.Phi());
                  TVector2 q2;
                  q2.SetMagPhi(q, q_phi);
                  return (p2 + q2).Mod();
                }),
                llp4, met, met_phi);

  // compute number of leptons
  auto nlep_req = df.define(column::constant<unsigned int>(2));
  auto nlep_sel =
      df.define(column::expression([](VecD const &lep) { return lep.size(); }),
                lep_pt_sel);

  // apply cuts & weights
  auto weighted = df.weight(mc_weight * el_sf * mu_sf);
  auto cut_2l = weighted.filter(nlep_sel == nlep_req);
  auto cut_2los =
      cut_2l.filter(column::expression([](const VecF &lep_charge) {
                      return lep_charge.at(0) + lep_charge.at(1) == 0;
                    }),
                    lep_Q);
  auto cut_2ldf =
      cut_2los.filter(column::expression([](const VecI &lep_type) {
                        return lep_type.at(0) + lep_type.at(1) == 24;
                      }),
                      lep_type);
  auto cut_2lsf =
      cut_2los.filter(column::expression([](const VecI &lep_type) {
                        return (lep_type.at(0) + lep_type.at(1) == 22) ||
                               (lep_type.at(0) + lep_type.at(1) == 26);
                      }),
                      lep_type);
  auto mll_max = df.define(column::constant(80.0));
  auto met_min = df.define(column::constant(30.0));
  auto cut_2ldf_sr = cut_2ldf.filter((mll < mll_max) && (met > met_min));
  auto cut_2lsf_sr = cut_2lsf.filter((mll < mll_max) && (met > met_min));
  auto cut_2los_sr =
      df.filter(cut_2ldf_sr || cut_2lsf_sr).weight(mc_weight * el_sf * mu_sf);
  // once two selections are joined, they "forget" everything upstream
  // i.e. need to re-apply the event weight!

  // histograms:
  // - at three regions
  // - nominal & eg_up & eg_dn
  auto [pth_2los_sr, pth_2ldf_sr, pth_2lsf_sr] =
      df.make(query::plan<Hist<1, float>>("pth", 30, 0, 150))
          .fill(higgs_pt)
          .book(cut_2los_sr, cut_2ldf_sr, cut_2lsf_sr);
  // all done at once :)

  Double_t w = 1600;
  Double_t h = 800;
  TCanvas c("c", "c", w, h);
  c.SetWindowSize(w + (w - c.GetWw()), h + (h - c.GetWh()));
  c.Divide(3, 1);
  c.cd(1);
  pth_2los_sr.nominal()->SetLineColor(kBlack);
  pth_2los_sr.nominal()->Draw("ep");
  pth_2los_sr["eg_up"]->SetLineColor(kRed);
  pth_2los_sr["eg_up"]->Draw("same hist");
  pth_2los_sr["eg_dn"]->SetLineColor(kBlue);
  pth_2los_sr["eg_dn"]->Draw("same hist");
  c.cd(2);
  pth_2ldf_sr.nominal()->SetLineColor(kBlack);
  pth_2ldf_sr.nominal()->Draw("ep");
  pth_2ldf_sr["eg_up"]->SetLineColor(kRed);
  pth_2ldf_sr["eg_up"]->Draw("same hist");
  pth_2ldf_sr["eg_dn"]->SetLineColor(kBlue);
  pth_2ldf_sr["eg_dn"]->Draw("same hist");
  c.cd(3);
  pth_2lsf_sr.nominal()->SetLineColor(kBlack);
  pth_2lsf_sr.nominal()->Draw("ep");
  pth_2lsf_sr["eg_up"]->SetLineColor(kRed);
  pth_2lsf_sr["eg_up"]->Draw("same hist");
  pth_2lsf_sr["eg_dn"]->SetLineColor(kBlue);
  pth_2lsf_sr["eg_dn"]->Draw("same hist");
  c.SaveAs("pth.png");

  return 0;
}
@endcpp

@image html pth.png

@section example-phys ATLAS DAOD_PHYS

1. Apply the MC event weight.
2. Select for events with exactly 2 electrons with @f$p_{\mathrm{T}} > 10\,\mathrm{GeV}@f$ and @f$ \eta < 2.4 @f$.
3. Compute & plot their di-invariant mass, @f$ m_{ee} @f$.

@cpp
#include "qhep/Event.h"
#include "qhep/Hist.h"

#include <xAODEventInfo/EventInfo.h>
#include <xAODEgamma/ElectronContainer.h>

using VecF = ROOT::RVec<float>;
using VecD = ROOT::RVec<double>;

#include "queryosity/queryosity.h"

using dataflow = queryosity::dataflow;
namespace multithread = queryosity::multithread;
namespace dataset = queryosity::dataset;
namespace column = queryosity::column;
namespace query = queryosity::query;
namespace systematic = queryosity::systematic;

#include "TH1F.h"
#include "TPad.h"
#include "TCanvas.h"
#include <ROOT/RVec.hxx>

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>

std::vector<std::string> daodFiles{
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000001.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000002.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000003.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000004.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000005.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000006.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000007.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000008.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000009.pool.root.1",
"/project/6001378/thpark/public/mc23_13p6TeV.601189.PhPy8EG_AZNLO_Zee.deriv.DAOD_PHYS.e8514_s4162_r14622_p5855/DAOD_PHYS.35010014._000010.pool.root.1"
};
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
  virtual ConstDataVector<xAOD::ElectronContainer> evaluate(
      column::observable<xAOD::ElectronContainer> els) const override {
    ConstDataVector<xAOD::ElectronContainer> els_sel(
        SG::VIEW_ELEMENTS);
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

void analyze(unsigned int n) {
  dataflow df(multithread::enable(n));

  auto ds = df.load(dataset::input<Event>(daodFiles, treeName));
  auto eventInfo = ds.read(dataset::column<xAOD::EventInfo>("EventInfo"));
  auto allElectrons =
      ds.read(dataset::column<xAOD::ElectronContainer>("Electrons"));

  auto selectedElectrons =
      df.define(column::definition<ElectronSelection>(10.0,1.5), allElectrons);
  auto diElectronsMassMeV =
      df.define(column::expression(DiElectronsMass), selectedElectrons);
  auto toGeV = df.define(column::constant(1.0 / 1000.0));
  auto diElectronsMassGeV = diElectronsMassMeV * toGeV;

  auto eventWeight = df.define(column::expression(EventWeight), eventInfo);
  auto atLeastTwoSelectedElectrons =
      df.weight(eventWeight)
          .filter(column::expression(TwoElectrons), selectedElectrons);

  auto selectedElectronsPtHist =
      df.make(query::plan<Hist<1,float>>("diElectronMass", 100, 0, 500))
          .fill(diElectronsMassGeV)
          .book(atLeastTwoSelectedElectrons);

  selectedElectronsPtHist->Draw();
  gPad->SetLogy();
  gPad->Print("mee.pdf");
}

int main(int argc, char *argv[]) { 
  int nthreads = 0;
  if (argc==2) { nthreads=strtol(argv[1], nullptr, 0); }

  auto tic = std::chrono::steady_clock::now();
  analyze(nthreads);
  auto toc = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = toc-tic;
  std::cout << "elapsed time (" << nthreads << " threads) = " << elapsed_seconds.count() << "s"
            << std::endl;
}
@endcpp

@image html mee.png

@out
elapsed time (1 threads) = 63.0538s
elapsed time (10 threads) = 10.4677s
@endout

@section example-task7 IRIS-HEP ADL benchmark

- Collision dataset: [2012 CMS open data](http://opendata.cern.ch/record/6021) (16 GiB, 53 million events).
- Task 7: plot the scalar sum in each event of the @f$p_{\mathrm{T}}@f$ of jets with @f$p_{\mathrm{T}}>30\,\mathrm{GeV}@f$ that are not within @f$\Delta R < 0.4@f$ of any light lepton with @f$p_{\mathrm{T}}>10\,\mathrm{GeV}@f$.

@cpp
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>

#include "queryosity/queryosity.h"

#include "TCanvas.h"
#include <ROOT/RVec.hxx>

#include "AnalysisPlugins/Hist.h"
#include "AnalysisPlugins/Tree.h"

template <typename T> using Vec = ROOT::RVec<T>;
using VecUI = Vec<unsigned int>;
using VecI = Vec<int>;
using VecF = Vec<float>;
using VecD = Vec<double>;

using dataflow = queryosity::dataflow;
namespace multithread = queryosity::multithread;
namespace dataset = queryosity::dataset;
namespace column = queryosity::column;
namespace query = queryosity::query;
namespace systematic = queryosity::systematic;

class DRMinMaxSel : public column::definition<VecI(VecF, VecF, VecF,
                                                               VecF, VecF)> {
public:
  DRMinMaxSel(float minDR, float pt2Min) : m_minDR(minDR), m_pt2Min(pt2Min) {}
  virtual ~DRMinMaxSel() = default;
  virtual VecI
  evaluate(column::observable<VecF> eta1,
           column::observable<VecF> phi1,
           column::observable<VecF> pt2,
           column::observable<VecF> eta2,
           column::observable<VecF> phi2) const override {
    VecI mask(eta1->size(), 1);
    if (eta2->size() == 0) {
      return mask;
    }

    const auto ptcut = (*pt2) > m_pt2Min;
    const auto eta2_ptcut = (*eta2)[ptcut];
    const auto phi2_ptcut = (*phi2)[ptcut];
    if (eta2_ptcut.size() == 0) {
      return mask;
    }

    const auto c = ROOT::VecOps::Combinations(*eta1, eta2_ptcut);
    for (auto i = 0u; i < c[0].size(); i++) {
      const auto i1 = c[0][i];
      const auto i2 = c[1][i];
      const auto dr = ROOT::VecOps::DeltaR((*eta1)[i1], eta2_ptcut[i2],
                                           (*phi1)[i1], phi2_ptcut[i2]);
      if (dr < m_minDR)
        mask[i1] = 0;
    }
    return mask;
  }

protected:
  const float m_minDR;
  const float m_pt2Min;
};

void task(int n) {

  dataflow df(multithread::enable(n));

  std::vector<std::string> tree_files{"Run2012B_SingleMu.root"};
  std::string tree_name = "Events";
  auto ds = df.load(dataset::input<Tree>(tree_files, tree_name));

  auto n_jet = ds.read(dataset::column<unsigned int>("nJet"));
  auto jets_pt = ds.read(dataset::column<VecF>("Jet_pt"));
  auto jets_eta = ds.read(dataset::column<VecF>("Jet_eta"));
  auto jets_phi = ds.read(dataset::column<VecF>("Jet_phi"));
  auto jets_m = ds.read(dataset::column<VecF>("Jet_mass"));

  auto n_muon = ds.read(dataset::column<unsigned int>("nMuon"));
  auto mus_pt = ds.read(dataset::column<VecF>("Muon_pt"));
  auto mus_eta = ds.read(dataset::column<VecF>("Muon_eta"));
  auto mus_phi = ds.read(dataset::column<VecF>("Muon_phi"));

  auto n_elec = ds.read(dataset::column<unsigned int>("nElectron"));
  auto els_pt = ds.read(dataset::column<VecF>("Electron_pt"));
  auto els_eta = ds.read(dataset::column<VecF>("Electron_eta"));
  auto els_phi = ds.read(dataset::column<VecF>("Electron_phi"));

  auto jets_ptcut = df.define(
      column::expression([](VecF const &pts) { return pts > 30; }),
      jets_pt);
  auto jets_mudr =
      df.define(column::definition<DRMinMaxSel>(0.4, 10.0),
                jets_eta, jets_phi, mus_pt, mus_eta, mus_phi);
  auto jets_eldr =
      df.define(column::definition<DRMinMaxSel>(0.4, 10.0),
                jets_eta, jets_phi, els_pt, els_eta, els_phi);
  auto goodjet_mask = jets_ptcut && jets_mudr && jets_eldr;
  auto goodjet_sumpt = df.define(
      column::expression(
          [](VecI const &good, VecF const &pt) { return Sum(pt[good]); }),
      goodjet_mask, jets_pt);

  auto cut_1jet = df.filter(
      column::expression([](int njet) { return njet >= 1; }),
      n_jet);
  auto cut_goodjet =
      cut_1jet.filter(column::expression(
                          [](VecI const &goodjet) { return Sum(goodjet); }),
                      goodjet_mask);

  auto h_sumpt_goodjet =
      df.make(query::plan<Hist<1, float>>("goodjet_sumpt", 185, 15, 200))
          .fill(goodjet_sumpt)
          .book(cut_goodjet);

  TCanvas c;
  h_sumpt_goodjet->Draw();
  c.SaveAs("task_7.png");
}

int main(int argc, char **argv) {
  int nthreads = 0;
  if (argc == 2) {
    nthreads = strtol(argv[1], nullptr, 0);
  }
  auto tic = std::chrono::steady_clock::now();
  task(nthreads);
  auto toc = std::chrono::steady_clock::now();
  std::chrono::duration<double> elapsed_seconds = toc - tic;
  std::cout << "used threads = " << nthreads
            << ", elapsed time = " << elapsed_seconds.count() << "s"
            << std::endl;
}
@endcpp
@image html task_7.png