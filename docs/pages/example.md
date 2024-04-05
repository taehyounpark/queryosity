@page example Examples
@tableofcontents

@section example-hello-world Hello World

- Example row (`v` may be empty):

@code{.json}
[
  { "x": 98.47054757472436, "v": [ 190.07135783114677, 14.80181202905574, 2.8667177988676418 ], "w": 1.0, }
]
@endcode

- Select entries with `v.size()` and `x > 100.0`.
- Fill histogram with `v[0]` weighted by `w`.

@cpp
#include <fstream>
#include <sstream>
#include <vector>

#include "queryosity.h"
#include "queryosity/hist.h"
#include "queryosity/json.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

int main() {
  dataflow df(multithread::enable(10));

  std::ifstream data("data.json");
  auto [x, v, w] = df.read(
      dataset::input<json>(data), dataset::column<double>("x"),
      dataset::column<std::vector<double>>("v"), dataset::column<double>("w"));

  auto zero = df.define(column::constant(0));
  auto v0 = v[zero];

  auto sel =
      df.weight(w)
          .filter(column::expression(
              [](std::vector<double> const &v) { return v.size(); }))(v)
          .filter(column::expression([](double x) { return x > 100.0; }))(x);

  auto h_x0_w = df.get(query::output<h1d>(linax(20, 0.0, 200.0)))
                    .fill(v0)
                    .at(sel)
                    .result();

  std::ostringstream os;
  os << *h_x0_w;
  std::cout << os.str() << std::endl;
}
@endcpp
@out
histogram(regular(20, 0, 200, options=underflow | overflow))
                ┌────────────────────────────────────────────────────────────┐
[-inf,   0) 0   │                                                            │
[   0,  10) 455 │███████████████████████████████████████████████████████████ │
[  10,  20) 432 │████████████████████████████████████████████████████████    │
[  20,  30) 368 │███████████████████████████████████████████████▊            │
[  30,  40) 359 │██████████████████████████████████████████████▌             │
[  40,  50) 309 │████████████████████████████████████████▏                   │
[  50,  60) 249 │████████████████████████████████▎                           │
[  60,  70) 208 │███████████████████████████                                 │
[  70,  80) 175 │██████████████████████▊                                     │
[  80,  90) 141 │██████████████████▎                                         │
[  90, 100) 99  │████████████▉                                               │
[ 100, 110) 82  │██████████▋                                                 │
[ 110, 120) 79  │██████████▎                                                 │
[ 120, 130) 58  │███████▌                                                    │
[ 130, 140) 40  │█████▏                                                      │
[ 140, 150) 20  │██▋                                                         │
[ 150, 160) 27  │███▌                                                        │
[ 160, 170) 19  │██▌                                                         │
[ 170, 180) 20  │██▋                                                         │
[ 180, 190) 18  │██▍                                                         │
[ 190, 200) 7   │▉                                                           │
[ 200, inf) 29  │███▊                                                        │
                └────────────────────────────────────────────────────────────┘
@endout

@section example-basic Basic examples

@subsection example-stirling Custom column definition

- Compute the factorial of a number, @f$ n! = 1 \times 2 \times 3 \times ... \times n @f$.
  1. if @f$ n! @f$ is too small & can fit inside ull_t, use full calculation
  2. if @f$ n @f$ is large for approximation to be good enough, use it.

@cpp
#include <fstream>
#include <sstream>
#include <vector>

#include "queryosity.h"
#include "queryosity/hist.h"
#include "queryosity/json.h"

using dataflow = qty::dataflow;
namespace multithread = qty::multithread;
namespace dataset = qty::dataset;
namespace column = qty::column;
namespace query = qty::query;

using json = qty::json;
using h1d = qty::hist::hist<double>;
using linax = qty::hist::axis::regular;

using ull_t = unsigned long long;
auto factorial(ull_t n) {
  ull_t result = 1;
  while (n > 1)
    result *= n--;
  return result;
}

auto stirling = [](ull_t n) {
  return std::round(std::sqrt(2 * M_PI * n) * std::pow(n / std::exp(1), n));
};

class Factorial : public column::definition<double(ull_t, double, ull_t)> {
public:
  Factorial(ull_t threshold = 20) : m_threshold(threshold) {}
  virtual ~Factorial() = default;
  virtual double evaluate(column::observable<ull_t> n, column::observable<double> fast,
                 column::observable<ull_t> full) const override {
    return (n.value() >= std::min<ull_t>(m_threshold, 20)) ? fast.value()
                                                    : full.value();
  }
  void adjust_threshold(ull_t threshold) { m_threshold = threshold; }

protected:
  ull_t m_threshold;
};

int main() {
  dataflow df;

  std::ifstream data_json("data.json");
  auto n = df.load(dataset::input<json>(data_json))
               .read(dataset::column<ull_t>("n"));

  auto n_f_fast = df.define(column::expression(stirling))(n);
  auto n_f_full = df.define(column::expression(factorial))(n);

  ull_t n_threshold = 10;
  auto n_f_slow = df.define(
      column::expression([n_threshold](ull_t n, double fast, ull_t slow) -> double {
        return n >= std::min<ull_t>(n_threshold,20) ? fast : slow;
      }))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + t(fast) + t(slow)
  // :(

  auto n_f_best =
      df.define(column::definition<Factorial>(/*20*/))(n, n_f_fast, n_f_full);
  // time elapsed = t(n) + { t(n_fast) if n >= 10, t(n_slow) if n < 10 }
  // :)

  // advanced: access per-thread instance
  dataflow::node::invoke([n_threshold](Factorial *n_f) { n_f->adjust_threshold(n_threshold); },
                         n_f_best);
}

@endcpp

@subsection example-column-series Column as a series

@cpp
// single column
// (sel: (varied) lazy selection)
auto x = ds.read(dataset::column<double>("x"));
auto x_arr = df.get(column::series(x)).at(sel).result(); // std::vector<double>

// single column at multiple selections
// (sel_a/b/c: (varied) lazy selections)
auto [x_arr_a, x_arr_b, x_arr_c] = df.get(column::series(x)).at(sel_a, sel_b, sel_c);

// multiple columns at a selection
auto y = ds.read(dataset::column<int>("y"));
auto z = ds.read(dataset::column<std::string>("z"));
auto [y_arr, z_arr] = sel.get(column::series(y, z));
@endcpp

@subsection example-selection-yield Yield at a selection

@cpp
// single selection
auto all = df.filter(column::constant(true));
auto yield_tot = def.get(selection::yield(all));
unsigned long long yield_tot_entries =
    yield_tot.entries;                    // number of entries passed
double yield_tot_value = yield_tot.value; // sum(weights)
double yield_tot_error = yield_tot.error; // sqrt(sum(weights squared))

// multiple selections 
// (sel_a/b/c: (varied) lazy selections)
auto [yield_a, yield_b, yield_c] =
    df.get(selection::yield(sel_a, sel_b, sel_c));
@endcpp

@section example-hep More examples

[HepQuery](https://github.com/taehyounpark/queryosity-hep) provides the extensions for ROOT TTree datasets and ROOT `TH1`-based outputs.

@subsection example-hep-hww ROOT TTree

- Simulated ggF HWW* events: [ATLAS open data](https://opendata.cern.ch/record/3825).

1. Apply the MC event weight.
2. Select entries for which there are exactly two opposite-sign leptons in the event.
3. Separate into different/same-flavour channels for electrons and muons.
4. Require @f$m_{\ell\ell} > 10(12)\,\mathrm{GeV}@f$ for different(same)-flavour channel.
5. Merge channels to form flavour-inclusive opposite-sign region post-@f$m_{\ell\ell}@f$ cut.
6. In each region, plot the distribution of @f$p_{\mathrm{T}}^H = \left| \mathbf{p}_{\mathrm{T}}^{\ell\ell} + \mathbf{p}_{\mathrm{T}}^{\mathrm{miss}} \right|@f$.
	- Scale electron(muon) energy scale by @f$\pm 1(2)\,\%@f$ as systematic variations.

@cpp
#include "HepQuery/Hist.h"
#include "HepQuery/Tree.h"

#include "queryosity.h"

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

// compute the nth-leading four-momentum out of (pt, eta, phi, m) arrays
class NthP4 : public column::definition<P4(VecF, VecF, VecF, VecF)> {

public:
  NthP4(unsigned int index) : m_index(index) {}
  virtual ~NthP4() = default;

  virtual P4 evaluate(column::observable<VecF> pt, column::observable<VecF> eta,
                      column::observable<VecF> phi,
                      column::observable<VecF> es) const override {
    return P4(pt->at(m_index), eta->at(m_index), phi->at(m_index),
              es->at(m_index));
  }

protected:
  const unsigned int m_index;
};

int main() {

  // load dataset (not enough events to multithread)
  std::vector<std::string> tree_files{"hww.root"};
  std::string tree_name = "mini";
  dataflow df(multithread::disable());
  auto ds = df.load(dataset::input<HepQ::Tree>(tree_files, tree_name));

  // weights
  auto mc_weight = ds.read(dataset::column<float>("mcWeight"));
  auto mu_SF = ds.read(dataset::column<float>("scaleFactor_MUON"));
  auto el_SF = ds.read(dataset::column<float>("scaleFactor_ELE"));
  // leptons
  auto [lep_pT, lep_eta, lep_phi, lep_E, lep_Q, lep_type] = ds.read(
      dataset::column<VecF>("lep_pt"), dataset::column<VecF>("lep_eta"),
      dataset::column<VecF>("lep_phi"), dataset::column<VecF>("lep_E"),
      dataset::column<VecF>("lep_charge"), dataset::column<VecUI>("lep_type"));
  // missing transverse energy
  auto [met, met_phi] = ds.read(dataset::column<float>("met_et"),
                                    dataset::column<float>("met_phi"));
  // units
  auto MeV = df.define(column::constant<float>(1000.0));
  lep_pT = lep_pT / MeV;
  lep_E = lep_E / MeV;
  met = met / MeV;

  // select electrons
  auto el_sel = lep_type == df.define(column::constant(11));
  auto el_pT_nom = lep_pT[el_sel];
  auto el_eta = lep_eta[el_sel];
  auto el_phi = lep_phi[el_sel];
  auto el_E_nom = lep_E[el_sel];
  auto el_Q = lep_Q[el_sel];
  auto el_type = lep_type[el_sel];
  // select muons
  auto mu_sel = lep_type == df.define(column::constant(13));
  auto mu_pT_nom = lep_pT[mu_sel];
  auto mu_eta = lep_eta[mu_sel];
  auto mu_phi = lep_phi[mu_sel];
  auto mu_E_nom = lep_E[mu_sel];
  auto mu_Q = lep_Q[mu_sel];
  auto mu_type = lep_type[mu_sel];

  // vary the energy scale by +/-1(2)% for electrons(muons)
  auto el_scale = df.vary(column::expression([](VecF const &E) { return E; }),
                          {{"el_up", [](VecF const &E) { return E * 1.01; }},
                           {"el_dn", [](VecF const &E) { return E * 0.99; }}});
  auto mu_scale = df.vary(column::expression([](VecF const &E) { return E; }),
                          {{"mu_up", [](VecF const &E) { return E * 1.02; }},
                           {"mu_dn", [](VecF const &E) { return E * 0.98; }}});
  auto el_pT = el_scale(el_pT_nom);
  auto el_E = el_scale(el_E_nom);
  auto mu_pT = mu_scale(mu_pT_nom);
  auto mu_E = mu_scale(mu_E_nom);

  // re-concatenate into el+mu arrays
  auto concat = [](VecF const &v1, VecF const &v2) {
    return ROOT::VecOps::Concatenate(v1, v2);
  };
  auto el_mu_pT = df.define(column::expression(concat))(el_pT, mu_pT);
  auto el_mu_eta = df.define(column::expression(concat))(el_eta, mu_eta);
  auto el_mu_phi = df.define(column::expression(concat))(el_phi, mu_phi);
  auto el_mu_E = df.define(column::expression(concat))(el_E, mu_E);
  auto el_mu_Q = df.define(column::expression(concat))(el_Q, mu_Q);
  auto el_mu_type = df.define(column::expression(concat))(el_type, mu_type);

  // take sorted lepton arrays
  auto take = df.define(column::expression([](VecF const &v, VecUI const &is) {
    return ROOT::VecOps::Take(v, is);
  }));
  auto lep_indices = df.define(column::expression(
      [](VecF const &v) { return ROOT::VecOps::Argsort(v); }))(el_mu_pT);
  auto lep_pT_syst = take(el_mu_pT, lep_indices);
  auto lep_eta_syst = take(el_mu_eta, lep_indices);
  auto lep_phi_syst = take(el_mu_phi, lep_indices);
  auto lep_E_syst = take(el_mu_E, lep_indices);
  auto lep_Q_syst = take(el_mu_Q, lep_indices);
  auto lep_type_syst = take(el_mu_type, lep_indices);

  // apply acceptance selections
  auto lep_pT_min = df.define(column::constant(15.0));
  auto lep_eta_max = df.define(column::constant(2.4));
  auto lep_sel = (lep_eta_syst < lep_eta_max) &&
                 (lep_eta_syst > (-lep_eta_max)) && (lep_pT_syst > lep_pT_min);
  auto lep_pT_sel = lep_pT_syst[lep_sel];
  auto lep_E_sel = lep_E_syst[lep_sel];
  auto lep_eta_sel = lep_eta_syst[lep_sel];
  auto lep_phi_sel = lep_phi_syst[lep_sel];
  auto lep_Q_sel = lep_Q_syst[lep_sel];
  auto lep_type_sel = lep_type_syst[lep_sel];

  // compute (sub-)leading lepton four-momentum
  auto l1p4 = df.define(column::definition<NthP4>(0))(lep_pT_sel, lep_eta_sel,
                                                      lep_phi_sel, lep_E_sel);
  auto l2p4 = df.define(column::definition<NthP4>(1))(lep_pT_sel, lep_eta_sel,
                                                      lep_phi_sel, lep_E_sel);

  // compute dilepton invariant mass & higgs transverse momentum
  auto llp4 = l1p4 + l2p4;
  auto mll =
      df.define(column::expression([](const P4 &p4) { return p4.M(); }))(llp4);
  auto higgs_pT =
      df.define(column::expression([](const P4 &p4, float q, float q_phi) {
        TVector2 p2;
        p2.SetMagPhi(p4.Pt(), p4.Phi());
        TVector2 q2;
        q2.SetMagPhi(q, q_phi);
        return (p2 + q2).Mod();
      }))(llp4, met, met_phi);

  // compute number of leptons
  auto nlep_req = df.define(column::constant<unsigned int>(2));
  auto nlep_sel = df.define(column::expression(
      [](VecF const &lep) { return lep.size(); }))(lep_pT_sel);

  // apply MC event weight * electron & muon scale factors
  auto weighted = df.weight(mc_weight * el_SF * mu_SF);

  // require 2 opoosite-signed leptons
  auto cut_2l = weighted.filter(nlep_sel == nlep_req);
  auto cut_2los = cut_2l.filter(column::expression([](const VecF &lep_charge) {
    return lep_charge[0] + lep_charge[1] == 0;
  }))(lep_Q_sel);

  // branch out into differet/same-flavour channels
  auto cut_df = cut_2los.filter(column::expression([](const VecI &lep_type) {
    return lep_type[0] + lep_type[1] == 24;
  }))(lep_type_sel);
  auto cut_ee = cut_2los.filter(column::expression([](const VecI &lep_type) {
    return (lep_type[0] + lep_type[1] == 22);
  }))(lep_type_sel);
  auto cut_mm = cut_2los.filter(column::expression([](const VecI &lep_type) {
    return (lep_type[0] + lep_type[1] == 26);
  }))(lep_type_sel);

  // apply (different) cuts for each channel
  auto mll_min_df = df.define(column::constant(10.0));
  auto cut_df_presel = cut_df.filter(mll > mll_min_df);
  auto mll_min_sf = df.define(column::constant(12.0));
  auto cut_ee_presel = cut_ee.filter(mll > mll_min_sf);
  auto cut_mm_presel = cut_mm.filter(mll > mll_min_sf);

  // merge df+sf channels
  // evaluate the merged selection and apply it as a
  auto cut_2los_presel =
      cut_2los.filter(cut_df_presel || cut_ee_presel || cut_mm_presel);

  // make histograms
  auto [pTH_2los_presel, pTH_df_presel, pTH_ee_presel, pTH_mm_presel] =
      df.get(query::output<HepQ::Hist<1, float>>("pTH", 30, 0, 150))
          .fill(higgs_pT)
          .at(cut_2los, cut_df_presel, cut_ee_presel, cut_mm_presel);

  // plot results
  Double_t w = 1600;
  Double_t h = 1600;
  TCanvas c("c", "c", w, h);
  c.SetWindowSize(w + (w - c.GetWw()), h + (h - c.GetWh()));
  c.Divide(2, 2);
  c.cd(1);
  pTH_2los_presel.nominal()->SetTitle("2LOS");
  pTH_2los_presel.nominal()->SetLineColor(kBlack);
  pTH_2los_presel.nominal()->Draw("ep");
  pTH_2los_presel["el_up"]->SetLineColor(kRed);
  pTH_2los_presel["el_up"]->Draw("same hist");
  pTH_2los_presel["mu_dn"]->SetLineColor(kBlue);
  pTH_2los_presel["mu_dn"]->Draw("same hist");
  pTH_2los_presel.nominal()->Draw("same hist");
  c.cd(2);
  pTH_df_presel.nominal()->SetTitle("2LDF");
  pTH_df_presel.nominal()->SetLineColor(kBlack);
  pTH_df_presel.nominal()->Draw("ep");
  pTH_df_presel["el_up"]->SetLineColor(kRed);
  pTH_df_presel["el_up"]->Draw("same hist");
  pTH_df_presel["mu_dn"]->SetLineColor(kBlue);
  pTH_df_presel["mu_dn"]->Draw("same hist");
  pTH_df_presel.nominal()->Draw("same hist");
  c.cd(3);
  pTH_ee_presel.nominal()->SetTitle("2LSF (ee)");
  pTH_ee_presel.nominal()->SetLineColor(kBlack);
  pTH_ee_presel.nominal()->Draw("ep");
  pTH_ee_presel["el_up"]->SetLineColor(kRed);
  pTH_ee_presel["el_up"]->Draw("same hist");
  pTH_ee_presel["mu_dn"]->SetLineColor(kBlue);
  pTH_ee_presel["mu_dn"]->Draw("same hist");
  pTH_ee_presel.nominal()->Draw("same hist");
  c.cd(4);
  pTH_mm_presel.nominal()->SetTitle("2LSF (mm)");
  pTH_mm_presel.nominal()->SetLineColor(kBlack);
  pTH_mm_presel.nominal()->Draw("ep");
  pTH_mm_presel["el_up"]->SetLineColor(kRed);
  pTH_mm_presel["el_up"]->Draw("same hist");
  pTH_mm_presel["mu_dn"]->SetLineColor(kBlue);
  pTH_mm_presel["mu_dn"]->Draw("same hist");
  pTH_mm_presel.nominal()->Draw("same hist");
  c.SaveAs("pTH.png");

  return 0;
}
@endcpp

@image html pth.png

@subsection example-hep-daod ATLAS DAOD_PHYS

1. Apply the MC event weight.
2. Select for events with exactly 2 electrons with @f$p_{\mathrm{T}} > 10\,\mathrm{GeV}@f$ and @f$ \eta < 2.4 @f$.
3. Compute & plot their di-invariant mass, @f$ m_{ee} @f$.

@cpp
#include "HepQuery/Event.h"
#include "HepQuery/Hist.h"

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

  auto ds = df.load(dataset::input<HepQ::Event>(daodFiles, treeName));
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
      df.get(query::plan<HepQ::Hist<1, float>>("diElectronMass", 100, 0, 500))
          .fill(diElectronsMassGeV)
          .at(atLeastTwoSelectedElectrons);

  selectedElectronsPtHist->Draw();
  gPad->SetLogy();
  gPad->Print("mee.pdf");

  return 0;
}
@endcpp

@image html mee.png

@subsection example-hep-task7 IRIS-HEP ADL benchmark

- Collision dataset: [2012 CMS open data](http://opendata.cern.ch/record/6021) (16 GiB, 53 million events).
- Task 7: plot the scalar sum in each event of the @f$p_{\mathrm{T}}@f$ of jets with @f$p_{\mathrm{T}}>30\,\mathrm{GeV}@f$ that are not within @f$\Delta R < 0.4@f$ of any light lepton with @f$p_{\mathrm{T}}>10\,\mathrm{GeV}@f$.

@cpp
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>

#include "queryosity.h"

#include "TCanvas.h"
#include <ROOT/RVec.hxx>

#include "HepQuery/Hist.h"
#include "HepQuery/Tree.h"

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

class DRMinMaxSel
    : public column::definition<VecI(VecF, VecF, VecF, VecF, VecF)> {
public:
  DRMinMaxSel(float minDR, float pt2Min) : m_minDR(minDR), m_pt2Min(pt2Min) {}
  virtual ~DRMinMaxSel() = default;
  virtual VecI evaluate(column::observable<VecF> eta1,
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
  auto ds = df.load(dataset::input<HepQ::Tree>(tree_files, tree_name));

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
      column::expression([](VecF const &pts) { return pts > 30; }))(jets_pt);
  auto jets_mudr = df.define(column::definition<DRMinMaxSel>(0.4, 10.0))(
      jets_eta, jets_phi, mus_pt, mus_eta, mus_phi);
  auto jets_eldr = df.define(column::definition<DRMinMaxSel>(0.4, 10.0))(
      jets_eta, jets_phi, els_pt, els_eta, els_phi);
  auto goodjet_mask = jets_ptcut && jets_mudr && jets_eldr;
  auto goodjet_sumpt =
      df.define(column::expression([](VecI const &good, VecF const &pt) {
        return Sum(pt[good]);
      }))(goodjet_mask, jets_pt);

  auto cut_goodjet = df.filter(column::expression(
      [](VecI const &goodjet) { return Sum(goodjet); }))(goodjet_mask);

  auto h_sumpt_goodjet =
      df.get(query::output<HepQ::Hist<1, float>>("goodjet_sumpt", 185, 15, 200))
          .fill(goodjet_sumpt)
          .at(cut_goodjet);

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
@out
used threads = 1, elapsed time = 116.676s
used threads = 20, elapsed time = 9.06188s
@endout