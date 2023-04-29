Coherent data analysis in C++.
- Multi-threaded processing of the dataset.
- Fast and intuitive computation of column values.
- Clear chain and/or branches of selections applied to entries.
- Systematic variations of an analysis computation performed simultaneously.
- Customizable handling & output of analysis results.

The purpose of this library is to provide a clear, ***_abstract_*** interface for users to implement the above features crucial for any rigorous dataset transformation procedures.

## Prerequisites
- C++17 standard compiler
- CMake 3.12 or newer

## Key concepts

- An `analysis` entity that represents the entire dataset.
- Performing an operation outputs a `delayed` node representing the booked action.
  - The processing of the dataset performing booked actions are triggered upon accessing the result of a node.
- Further operations can be done in the context of existing ones, which are provided as input arguments.
  - Some of these operations are called from the analysis, others from individual nodes.
  - The computation graph among nodes is recursion-free by construction.
- A node can be systematically `varied` for which an alternate definition of the action is included in the analysis.
  - The maximal union set of variations is ensured to be reflected in any nodes involving any input node(s) with variation(s).

## Applied walkthrough

The following example uses an implementation of the interface for the [CERN ROOT framework](https://root.cern/) to illustrate a conceptual demonstration of physics collision data analysis reconstructing the Higgs boson transverse momentum in a simulated $H\rightarrow WW^{\ast}\rightarrow e\nu\mu\nu$ dataset. See [here](https://github.com/taehyounpark/RAnalysis) for the implementation and [CERN Open Data Portal](https://opendata.cern.ch/record/700) for the dataset.

### 0. Opening the dataset
Any data structure that can be represented as a (per-row) $\times$ (column-value) layout is supported. The initialization of an *analysis* proceeds as:
```cpp
// provide or default to maximum number of thread count
ana::multithread::enable();  

// implements ana::input::dataset<CRTP>
auto data = ana::analysis<TreeData>();

// constructor arguments of TreeData
data.open("mini", {"hww_mc.root"});  
```

### 1. Accessing quantities of interest
#### 1.1 Reading columns in the dataset
Existing *columns* in the dataset can be accessed by supplying their types and names.
```cpp
// implements ana::input::dataset<CRTP>, ana::column::reader<T>
auto mc_weight = data.read<float>("mcWeight");
auto el_sf = data.read<float>("scaleFactor_ELE");
auto mu_sf = data.read<float>("scaleFactor_MUON");
auto n_lep = data.read<unsigned int>("lep_n");
auto lep_pts_MeV = data.read<ROOT::RVec<float>>("lep_pt");
auto lep_etas = data.read<ROOT::RVec<float>>("lep_eta");
auto lep_phis = data.read<ROOT::RVec<float>>("lep_phi");
auto lep_Es_MeV = data.read<ROOT::RVec<float>>("lep_E");
auto lep_Qs = data.read<ROOT::RVec<float>>("lep_charge");
auto lep_types = data.read<ROOT::RVec<unsigned int>>("lep_type");
auto met_MeV = data.read<float>("met_et");
auto met_phi = data.read<float>("met_phi");
```
#### 1.2 Computing new quantities
Arithmetic operators supported by the underlying data types can passed through the node:
```cpp
// ROOT::RVec<float> supports division
auto GeV = ana.constant<double>(1000.0);
auto lep_pts = lep_pts / GeV;
auto lep_Es = lep_pts / GeV;
auto met = met_MeV / GeV;
```
Custom C++ expressions out of existing ones can also be applied as lambda expressions:
```cpp
// see below for l1p4 and l2p4
auto dilepP4 = data.define([](TLorentzVector const& p4, TLorentzVector const& q4){return (p4+q4);})(l1p4,l2p4);
```
Fully custom definitions are equally-well supported by full class definitions:
```cpp
using RVecD = ROOT::RVec<double>;
class ScaledP4 : public ana::column::definition<TLorentzVector(RVecD, RVecD, RVecD, RVecD)>
{
public:
  ScaledP4(unsigned int index, double scale=1.0) : 
    ana::column::definition<TLorentzVector(RVecD, RVecD, RVecD, RVecD)>(),
    m_index(index)
  {}
  virtual ~ScaledP4() = default;
  virtual TLorentzVector evaluate(ana::observable<RVecD> pts, ana::observable<RVecD> etas, ana::observable<RVecD> phis, ana::observable<RVecD> es) const override {
    TLorentzVector p4;
    p4.SetPtEtaPhiE(pts->at(m_index),etas->at(m_index),phis->at(m_index),es->at(m_index));
    return p4;
  }
protected:
  unsigned int m_index;
  double m_scale;
};
```



### 2. Applying selections
#### 2.1 Cut versus weight
Filtering entries in a dataset is done via a *selection*, which is either a boolean decision for floating-point value that chooses to ignore or assigns a non-uniform statistical significance for each entry, respectively.
```cpp
using cut = ana::selection::cut;
using weight = ana::selection::weight;
auto cut2l = data.filter<cut>("2l", [](int nlep){return (nlep == 2);})(nlep)\
                 .filter<weight>("mc_weight")(mc_weight)\
                 .filter<weight>("el_sf")(el_sf)\
                 .filter<weight>("mu_sf")(mu_sf);
```
- Selections that are applied in sequence after the first can be chained from the nodes directly.
- Each filter operation requires an identifiable name, which is used to form the path of the full chain of selections applied.
#### 2.2 Branching out & channels
Distinct (not required to be mutually exclusive) chains of selections can branch out from a common point. Designating a particular selection as a *channel* marks that its name will be reflected as part of the path of downstream selections.
```cpp
// note: "channel" designation
auto cut2los = cut2l.channel<cut>("2los", [](ROOT::RVec<float> const& qs){return (qs.at(0) + qs.at(1) == 0);})(lep_charges);

// branching out from a common 2-lepton, opposite-sign cut
auto cut2ldf = cut2los.filter<cut>("2ldf", [](ROOT::RVec<int> const& flavours){return (flavours.at(0) + flavours.at(1) == 24);})(lep_types);
auto cut2lsf = cut2los.filter<cut>("2lsf", [](ROOT::RVec<int> const& flavours){return ((flavours.at(0) + flavours.at(1) == 22) || (lep_type.at(0) + lep_type.at(1) == 26));})(lep_types);
```
### 3. Counting entries
#### 3.1 Booking counters
A *counter* is an arbitrary action performed once per-entry:
- Only if a specified selection passes the cut, "count" the entry with its weight.
- (Optional) receive the values of other columns to be "filled" with.
```cpp
// 1-dimensional histogram:
// fill with the values of higgs pT
// make one for each at different & same-flavour channels
auto pth_hists = data.book<Histogram<1,float>>("pth",100,0,400).fill(a).at(cut2los, cut2ldf);
```

#### 3.2 Processing the dataset and accessing results
The result of each counter can be accessed by specifying the path of the booked selections
```cpp
// dataset processing is triggered
auto pth_2los = pth_hists["2los"].result();
// results are already available, obtained instantaneously
auto pth_2lsf = pth_hists["2los/2ldf"].result();
auto pth_2ldf = pth_hists["2los/2lsf"].result();
```

### 4. Systematic variations

#### 4.1 Varying a column/selection
*Any* column and selection node can be varied with an alternative definition of itself.
```cpp
// varying the four-momenta of leptons by scaling it up and down
auto l1p4 = data.define<ScaledP4>(0)(lep_pts, lep_etas, lep_phis, lep_Es)\
                     .vary("lp4_up",0,1.02)(lep_pts, lep_etas, lep_phis, lep_Es)\
                     .vary("lp4_down",0,0.98)(lep_pts, lep_etas, lep_phis, lep_Es);

// was a delayed<ScaledP4> before, now a varied<ScaledP4>
auto l2p4 = data.define<ScaledP4>(1)(lep_pts, lep_etas, lep_phis, lep_Es)\
                     .vary("lp4_up",1,1.02)(lep_pts, lep_etas, lep_phis, lep_Es)\
                     .vary("lp4_down",1,0.98)(lep_pts, lep_etas, lep_phis, lep_Es);
```
Note that the process is completely flexible, such that __any__ arguments involved in the node construction can be changed: logical consistency and correctness of the variation is the responsibility of the physicist.
```cpp
// this makes no sense
auto l1p4_nonsense = data.define<ScaledP4>(0,0.5)(lep_pts, lep_etas, lep_phis, lep_Es)\
                     .vary("lp4_up",1,1.0)(jet_pts, jet_etas, jet_phis, jet_Es)\
                     .vary("lp4_down",2,2.0)(lep_pts, jet_etas, lep_charges, lep_types);
```
There is no further treatment needed to handle the variations in nodes: they can be ensured to be transparently propagated through all downstream nodes.
```cpp
auto dilepP4 = data.define([](TLorentzVector const& p4, TLorentzVector const& q4){return (p4+q4);})(l1p4,l2p4);
// already has "lp4_up/down" from input columns
```
Interactions among non-overlapping variations in the nodes are handled by the nominal of the respective nodes. At each propagation, the resulting node will contain the union of all applicable variations for the action.
```cpp
// scale factors have their own variations
auto el_sf_applied = data.filter<weight>("el_sf")(el_sf)\
                         .vary("el_sf_up",[](double x){return x*1.05;})(el_sf)\
                         .vary("el_sf_up",[](double x){return x*0.95;})(el_sf);

auto el_sf_applied = data.filter<weight>("el_sf")(el_sf)\
                         .vary("el_sf_up",[](double x){return x*1.05;})(el_sf)\
                         .vary("el_sf_up",[](double x){return x*0.95;})(el_sf);
// ...

auto pth_hists = data.book<Histogram<1,float>>("pth",100,0,200).fill(pth).at(cut2ldf, cut2lsf);
// variations propagated through:
// fill(pth) -> lep_p4_up/dn
// at(cut2los, cut2ldf) -> el_sf_up/dn

// note: additional nominal() call to access the original result
auto pth_2ldf_nom = pth_hists.nominal()["2ldf"].result();

// note: additional hash key to access the varied result
auto pth_2ldf_lp4_up = pth_hists["lep_p4_up"]["2ldf"].result();
auto pth_2ldf_lp4_up = pth_hists["el_sf_up"]["2ldf"].result();
```