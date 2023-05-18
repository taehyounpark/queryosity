Coherent data analysis in C++.

# Features
- Multithreaded processing of the dataset.
- Manipulation of any data types as column values.
- Arbitrary action execution and results retrieval.
- Propagation of systematic variations.

# Introduction

A clear _abstraction_ layer between the analyzer and the dataset to define dataset transformation procedures is instrumental in not only ensuring the technical robustness of an analysis, but also for its ease of reproducibility and extensibility as the project develops.

- The `analysis` entity represents the dataset to be analyzed.
- Any operation on it results in a `delayed` node representing the action to be performed.
  - The nodes can also interact with one another such that new operations can be defined in the context of existing ones.
- A node can be `varied`, meaning an alternate definition of the action is additionally included in the analysis.
  - They are propagated through any other nodes, such that the final results contain both the original and varied outcomes.

# Prerequisites
- C++17 compiler (tested with Clang 14 and GCC 11)
- CMake 3.24 or newer

# Walkthrough

The following example uses an implementation of the interface for the [CERN ROOT framework](https://root.cern/) to illustrate a conceptual demonstration of physics collision data analysis reconstructing the Higgs boson transverse momentum in a simulated $H\rightarrow WW^{\ast}\rightarrow e\nu\mu\nu$ dataset. See [here](https://github.com/taehyounpark/RAnalysis) for the implementation and example code and [here](https://opendata.cern.ch/record/700) for the publicly-available dataset.

## 0. Opening the dataset
Any data structure that can be represented as a (per-row) $\times$ (column-value) layout is supported. The initialization proceeds as:
```cpp
// enable/disable multithreading
ana::multithread::enable(/* 10 */);  // provide thread count (default: system maximum)

// Tree : ana::input::dataset<Tree> (i.e. user-implemented)
auto hww = ana::analysis<Tree>({"hww.root"}, "mini");
```

## 1. Accessing quantities of interest
### 1.1 Reading columns in the dataset
Existing columns in the dataset can be accessed by supplying their types and names.
```cpp
// Branch<T> : ana::column::reader<Branch<T>>  (i.e. user-implemented)
auto mc_weight = data.read<float>("mcWeight");
auto el_sf = data.read<float>("scaleFactor_ELE");
auto mu_sf = hww.read<float>("scaleFactor_MUON");

// supports arbitrary data types
using RVecF = ROOT::RVec<float>;
using RVecUI = ROOT::RVec<unsigned int>;
auto lep_pt_MeV = hww.read<RVecF>("lep_pt");
auto lep_eta = hww.read<RVecF>("lep_eta");
auto lep_phi = hww.read<RVecF>("lep_phi");
auto lep_E_MeV = hww.read<RVecF>("lep_E");
auto lep_Q = hww.read<RVecF>("lep_charge");
auto lep_type = hww.read<RVecUI>("lep_type");
auto met_MeV = hww.read<float>("met_et");
auto met_phi = hww.read<float>("met_phi");
```
Performing operations on `analysis<Dataset>` returns a `delayed<Action>` (in this case of the columns).

### 1.2 Computing new quantities
### Simple expressions
Mathematical binary and unary operations available for the underlying data types are supported:
```cpp
auto GeV = ana.constant(1000.0);
auto lep_pt = lep_pt_MeV / GeV;
// (lep_E, met, ...)

auto lep_eta_max = hww.constant(2.4);
auto lep_pt_sel = lep_pt[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
// (lep_eta_sel, lep_phi_sel, lep_E_sel, ...)
```
As a superset of the above as well as to access non-trivial methods of the underlying data, any function (namely, lambda expression) can be provided.
```cpp
// 1. define dilepton four-momentum
auto p4ll = hww.calculate([](TLV const& p4, TLV const& q4) {return (p4+q4);})(l1p4,l2p4);

// 2. define (dilepton+MET) transverse momentum
auto pth = hww.calculate(
  [](const TLV& p3, float q, float q_phi) {
    TVector2 p2; p2.SetMagPhi(p3.Pt(), p3.Phi());
    TVector2 q2; q2.SetMagPhi(q, q_phi);
    return (p2+q2).Mod();
  })(p4ll, met, met_phi);
```
__Bonus:__ defining the function body separately from its input arguments enables "recycling" of common functions.
```cpp
auto get_pt = hww.calculate([](TLV const& p4){return p4.Pt();});
auto l1pt = get_pt(l1p4);
auto l2pt = get_pt(l2p4);
```
### (Advanced) Custom definitions
Even more complicated computations be fully implemented through `ana::column::defintion<Return(Arguments...)>`.
```cpp
using TLV = TLorentzVector;
using RVecD = ROOT::RVec<double>;
class NthP4 : public ana::column::definition<TLV(RVecD, RVecD, RVecD, RVecD)>
// define an ith TLorenzVector out of (pt,eta,phi,e) vectors
{
public:
  NthP4(unsigned int index) : 
    ana::column::definition<TLV(RVecD, RVecD, RVecD, RVecD)>(),
    m_index(index)
  {}
  virtual ~NthP4() = default;

  // implement this
  virtual TLV evaluate(ana::observable<RVecD> pt, ana::observable<RVecD> eta, ana::observable<RVecD> phi, ana::observable<RVecD> es) const override {
    TLV p4;
    p4.SetPtEtaPhiE(pt->at(m_index),eta->at(m_index),phi->at(m_index),es->at(m_index));
    return p4;
  }
  // (ana::observable<T> handles conversion/inheritance between compatible types)

// important: it is up to implementation to ensure thread-safety
protected:
  unsigned int m_index;
  // int* g_modifiable_global_var;  // <- bad idea
};
```
The above methods offer a flexible way to define columns in the way and order that makes the most conceptual sense for a given analysis:
```cpp
// first- & second-leading lepton four-momenta
auto l1p4 = hww.define<NthP4>(0)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);
auto l2p4 = hww.define<NthP4>(1)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);

// dilepton four-momentum
auto p4ll = l1p4+l2p4;

// dilepton invariant mass
auto mll = hww.calculate([](const TLV& p4){return p4.M();})(p4ll);

// dilepton+MET(=higgs) transverse momentum
auto pth = hww.calculate(
  [](const TLV& p4, float q, float q_phi) {
    TVector2 p2; p2.SetMagPhi(p4.Pt(), p4.Phi());
    TVector2 q2; q2.SetMagPhi(q, q_phi);
    return (p2+q2).Mod();
  })(p4ll, met, met_phi);
```
The computation graph is guaranteed to be recursion-free, as the grammar forbids this by construction. Furthermore, unless a conversion is required, column values are never copied when used as inputs for others.

## 2. Applying selections
### 2.1 Cut versus weight
Filtering entries in a dataset is done via a __selection__, which can be either a boolean or floating-point value that chooses to ignore or assign statistical significance to an entry, respectively.

The simplest way to define a selection is to provide the column that corresponds to the selection value, such as the following:
```cpp
using cut = ana::selection::cut;
using weight = ana::selection::weight;

auto n_lep_sel = hww.calculate([](ROOT::RVec<float> const& lep){return lep.size();})(lep_pt_sel);
auto n_lep_req = hww.constant(2);

auto cut_2l = hww.filter<weight>("weight")(mc_weight * el_sf * mu_sf)\
                 .filter<cut>("2l")(n_lep_sel == n_lep_req);
                  // cut = (true) && (n_lep == 2)
                  // weight = (mc_weight * el_sf * mu_sf) * (1.0)
```
Any combination of `cut` or `weight` can be applied in sequence. Selections can be chained from one another to compound them, whereas the `analysis` will otherwise always start out from the inclusive, unweighted dataset.

### 2.2 Branching out & channels
Each selection is associated with an identifier _name_, which need not be unique. Also, multiple selections can be applied from a single selection to form "branches", but these selections in separate branches need not be mutually exclusive from one another.

This approach can accommodate any arbitrary selection structure. Should the analyzer wish to resolve any ambiguities in the names of selections in different branches that may arise, it is easily done by replacing a `filter` call with `channel` for any selection (or more) after the branching point, such that the _path_ of a selection includes the upstream selection to form a unique string.
```cpp
// opposite-sign leptons
auto cut_2los = cut_2l.filter<cut>("2los", [](const RVecI& lep_charge){return lep_charge.at(0)+lep_charge.at(1)==0;})(lep_Q);
// different-flavour leptons
auto cut_2ldf = cut_2los.channel<cut>("2ldf", [](const RVecUI& lep_type){return lep_type.at(0)+lep_type.at(1)==24;})(lep_type);
// same-flavour leptons
auto cut_2lsf = cut_2los.channel<cut>("2lsf", [](const RVecUI& lep_type){return (lep_type.at(0)+lep_type.at(1)==22)||(lep_type.at(0)+lep_type.at(1)==26);})(lep_type);

// same cuts at different branches
auto mll_cut = hww.constant(60.0);
auto cut_2ldf_sr = cut_2ldf.filter<cut>("sr")(mll < mll_cut);  // 2ldf/sr
auto cut_2lsf_sr = cut_2lsf.filter<cut>("sr")(mll < mll_cut);  // 2lsf/sr
auto cut_2ldf_wwcr = cut_2ldf.filter<cut>("wwcr")(mll > mll_cut);  // 2ldf/cr
auto cut_2lsf_wwcr = cut_2lsf.filter<cut>("wwcr")(mll > mll_cut);  // 2lsf/cr
```

## 3. Counting entries
### 3.1 Booking counters and accessing their results
A __counter__ is an action that is for each entry:
- If its "booked" selection passed the cut, with its weight.
- (Optional) receive the values from input columns to be "filled" with.

A full implementation of `ana::counter::logic<Result(Columns...)>` defines what (arbitrary) action is to be performed as the counting operation, its output result, and how they should be merged from multiple threads.
```cpp
// Histogram<1,float> : ana::counter::logic<std::shared_ptr<TH1>(float)> (i.e. user-implemented)
auto pth_2los = hww.book<Histogram<1,float>>("pth",100,0,400).fill(pth).at(cut_2los);
// for each entry:
  // if (cut_2los.passed_cut()) { 
  //   pth_hist->Fill(pth, cut_2los.get_weight());
  // }
```
Accessing the result of a counter triggers the dataset processing:
```cpp
pth_2los.result();  // -> std::shared_ptr<TH1>
```
Each `fill` and `at` call returns a new node with those operations applied, such that any counter can be:
- Filled with columns any number of times.
- Booked at any (set of) selection(s).
```cpp
// fill the histogram with pT of both leptons
auto l1n2_pt_hist = hww.book<Histogram<1,float>>("l1n2_pt",20,0,100).fill(l1pt).fill(l2pt);

// 2ldf signal & control regions
auto l1n2_pt_hists_2ldf = l1n2_pt_hist.at(cut_2ldf_sr, cut_2ldf_wwcr);

// for 2lsf signal & control regions
auto l1n2_pt_hists_2lsf = l1n2_pt_hist.at(cut_2lsf_sr, cut_2lsf_wwcr);
```
When a counter is booked at multiple selections such as the above, the booked node can output the result at any specific selection by its path:
```cpp
l1n2_pt_hist_2ldf_sr = l1n2_pt_hists_2ldf["2ldf/sr"].result();
l1n2_pt_hist_2ldf_wwcr = l1n2_pt_hists_2ldf["2ldf/wwcr"].result();
```
### 3.2 (Optional) "Dumping" results

It a counter is booked at numerous selections, it may be convenient to have a uniform way to write out the results across all selections at once instead of dealing with the bookeeping challenges. For example, an implementation of `ana::counter::summary<T>` (or any other user-defined method) can provide a skeleton to consistently perform across multiple counters and selections:
```cpp
// booked at multiple selections
auto pth_hists = hww.book<Histogram<1,float>>("pth",100,0,400).fill(pth).at(cut_2los, cut_2ldf, cut_2lsf);

// Folder : ana::counter::summary<Folder>
// write histogram at each folder of the selection path
auto out_file = TFile::Open("hww_hists.root","recreate");
ana::output::dump<Folder>(pth_hists, out_file, "hww");
delete out_file;
```
![pth_hists](images/hww_hists.png)

## Non-redundancy of `delayed` actions

Each action is performed once per entry only when needed as the following:
1. A counter will perform its action only if its booked selection has passed its cut.
2. A selection will evaluate its cut decision only if it all of its upstream selections have passed, and weight value only if the cut passes.
4. A column value will be evaluated only if it is needed for any of the above.

In the above example:
- The filled columns ($p_\text{T}^H$ and $p_\text{T}^{\ell_2}$) require at least 2 elements in the input lepton vectors, without protection against otherwise.
- The histograms were (correctly) booked under a $n_\ell = 2$ selection.
- The computation of dilepton quantities are never triggered for entries that would haven thrown an exception.

## 4. Systematic variations

In general, the above approach to construct columns, selections, counters means that any arbitrary change requires an independent instance of the analysis.

Under a stricter definition that a __systematic variation__ constitutes a __change in a column value that affects the outcome of a fixed set of selection and counters__, the variations can co-exist in a single computation graph to be processed at once, which offers the following benefits:

- Coupled instantiation of actions guarantee that each variation and only the variation is in effect between the nominal and varied results.
- Eliminate the runtime overhead associated with repeated dataset processing.

### 4.1 Varying a column

Any column can be varied via a definition of the same type, which translates to:
- `reader` of the dataset can be varied to be one of a different column name holding the same data type.
- `constant` can be changed from any value to another.
- `equation` can be evaluated with another function of the same signature and return type.
- `definition` can be constructed with another set of arguments (instance-access also available per-variation).
 
```cpp
// use a different scale factor
auto el_sf = hww.read<float>("scaleFactor_ELE").vary("sf_var","scaleFactor_PILEUP");
// (purely for illustration)

// change the energy scale by +/-2%
auto Escale = hww.calculate([](RVecD E){return E;}).vary("lp4_up",[](RVecD E){return E*1.02;}).vary("lp4_dn",[](RVecD E){return E*0.98;});
auto lep_pt_sel = Escale(lep_pt)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
auto lep_E_sel = Escale(lep_E)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
```
This results in a `varied` node, which now contain multiple variations of the `delayed` action.

### 4.2 Propagation of variations through selections and counters

The rest of the analysis interface remains exactly the same with respect to handling `delayed` and `varied` nodes:
- Any column evaluated from varied input columns containing will be varied correspondingly.
- Any selections and counters performed with varied columns will be varied correspondingly.

The propagation of variations that may or may not exist in different sets of `delayed` and `varied` actions occur "in lockstep" and "transparently", meaning:
- If two actions each have a variation with the same name, they are in effect together.
- If one action has a variation while another doesn't, then the nominal is used for the latter.

```cpp
auto l1p4 = hww.define<NthP4>(0)(lep_pt, lep_eta, lep_phi, lep_E);
auto l2p4 = hww.define<NthP4>(1)(lep_pt, lep_eta, lep_phi, lep_E);
l1p4.has_variation("lp4_up");  // true
l2p4.has_variation("lp4_up");  // true

// ...

auto cut_2l = hww.filter<weight>("weight")(mc_weight * el_sf * mu_sf)\
                 .filter<cut>("2l")(n_lep_sel == n_lep_req);
cut_2l.has_variation("sf_var");  // true

// ...

auto mll_vars = hww.book<Histogram<1,float>>("mll",50,0,100).fill(mll).at(cut_2los);
mll_vars.has_variation("lp4_up"); // true
mll_vars.has_variation("sf_var"); // true

// additional nominal() & variation access
auto mll_nom = mll_vars.nominal().result();
auto mll_var = mll_vars["lp4_up"].result();
```
![mll_varied](images/mll_varied.png)

Accessing multiple systematic variations at multiple selections is possibly by providing both their names and paths:
```cpp
auto mll_channels_vars = hww.book<Histogram<1,float>>("mll",50,0,200).fill(mll).at(cut_2ldf, cut_2lsf);
auto mll_2ldf_nom = mll_channels_vars.nominal()["2ldf"].result();
auto mll_2lsf_var = mll_channels_vars["lp4_up"]["2lsf"].result();
```

# Known issues

- :x: (PyROOT) `delayed` and `varied` manipulations not working (SFINAE).
