Coherent data analysis in C++.

# Features
- Multithreaded processing of the dataset.
- Declarative computation of column values.
- Easy bookkeeping of selections.
- Arbitrary counting operations and their results.
- Propagation of systematic variations through an analysis.

# Idea

A clear _abstraction_ layer between the analyzer and the dataset to specify the dataset transformation procedures involved is instrumental in not only ensuring the reproducibility and technical robustness of the analysis, but also for it to be consistently extendable as the study matures.

- An `analysis` entity represents the dataset to be processed.
- An operation performed on it results in a `delayed` action representing the operation, but only executed when its result is requested.
  - These nodes can also interact with one another such that new operations can be done in the context of existing ones.
- A node can be systematically `varied` for which an alternate definition of the action is included in the analysis.
  - They are transparently propagated through any other nodes that the varied node(s) participate it.

# Prerequisites
- C++17 compiler
- CMake 3.24 or newer

# Walkthrough

The following example uses an implementation of the interface for the [CERN ROOT framework](https://root.cern/) to illustrate a conceptual demonstration of physics collision data analysis reconstructing the Higgs boson transverse momentum in a simulated $H\rightarrow WW^{\ast}\rightarrow e\nu\mu\nu$ dataset. See [here](https://github.com/taehyounpark/RAnalysis) for the implementation and example code and [here](https://opendata.cern.ch/record/700) for the publicly-available dataset.

## 0. Opening the dataset
Any data structure that can be represented as a (per-row) $\times$ (column-value) layout is supported. The initialization of an *analysis* proceeds as:
```cpp
// provide or default to maximum number of thread count
ana::multithread::enable();  

// Tree : ana::input::dataset<Tree> (i.e. user-implemented)
auto data = ana::analysis<Tree>();

// constructor arguments of Tree
data.open({"hww.root"}, "mini");  
```

## 1. Accessing quantities of interest
### 1.1 Reading columns in the dataset
Existing columns in the dataset can be accessed by supplying their types and names.
```cpp
// see: ana::input::reader<T>, ana::column::reader<T>  (i.e. user-implementable)
auto mc_weight = data.read<float>("mcWeight");
auto el_sf = data.read<float>("scaleFactor_ELE");
auto mu_sf = data.read<float>("scaleFactor_MUON");

// - arbitrary data types can be implemented
auto lep_pt_MeV = data.read<ROOT::RVec<float>>("lep_pt");
auto lep_eta = data.read<ROOT::RVec<float>>("lep_eta");

// etc. etc.
```
Performing operations on `analysis<Dataset>` returns a `delayed<Action>` (in this case of the columns).

### 1.2 Computing new quantities
### Simplest way
Mathematical binary and unary operations available for the the underlying data types are supported:
```cpp
auto GeV = ana.constant(1000.0);
auto lep_pt = lep_pt_MeV / GeV;  // ROOT::RVec<float> / double
// etc. for all other magnitudes

auto lep_eta_max = hww.constant(2.4);
auto lep_pt_sel = lep_pt[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
// etc. for all other lep_X
```
### Custom functions
As a superset of the above as well as to access non-trivial methods of the underlying data, any function (namely, lambda expression) can be provided.
```cpp
// see below for l1p4 and l2p4 definitions
auto dilepP4 = data.define([](TLV const& p4, TLV const& q4) {return (p4+q4);})(l1p4,l2p4);
```
The separation of defining the function body separately from its input arguments enables "recycling" of common functions.
```
auto get_pt = hww.define([](TLV const& p4){return p4.Pt();});
auto l1pt = get_pt(l1p4);
auto l2pt = get_pt(l2p4);
```
### Custom classes
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
  // - ana::observable<T> enables access to convertible/inherited types
  virtual TLV evaluate(ana::observable<RVecD> pt, ana::observable<RVecD> eta, ana::observable<RVecD> phi, ana::observable<RVecD> es) const override {
    TLV p4;
    p4.SetPtEtaPhiE(pt->at(m_index),eta->at(m_index),phi->at(m_index),es->at(m_index));
    return p4;
  }

// important: it is up to implementation to ensure thread-safety
protected:
  unsigned int m_index;
  // int* g_modifiable_global_var;  // <- bad idea
};

// ...

auto l1p4 = hww.define<NthP4>(0)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);
```
The computation graph formed out of these columns are guaranteed to be
- Recursion-free: the grammar forbids this by construction.
- No-copy: unless a conversion is required, their values are never copied when used as input values for other columns.
- "Lazy": the value of each column is computed at most once per entry and only if needed (see selections and counters below).

## 2. Applying selections
### 2.1 Cut versus weight
Filtering entries in a dataset is done via a __selection__, which can be either a boolean or floating-point value that chooses to ignore or assign statistical significance to an entry, respectively.

The simplest way to define a selection would be to simply provide the value of the column that actually corresponds to the cut decision or weight value, which is done by
```cpp
using cut = ana::selection::cut;
using weight = ana::selection::weight;

auto n_lep_sel = hww.define([](ROOT::RVec<float> const& lep){return lep.size();})(lep_pt_sel);
auto n_lep_req = data.constant<int>(2);
auto cut_2l = data.filter<weight>("weight")(mc_weight * el_sf * mu_sf)\
                  .filter<cut>("2l")(n_lep_sel == n_lep_req);
                   // final cut = (true) && (n_lep == 2)
                   // final weight = (mc_weight * el_sf * mu_sf) * (1.0)
```
Since that the two types are non-overlapping, any combination of `<cut>` or `<weight>` can be applied in sequence. Selections must be chained from one another to compound them, whereas the `analysis` object always represents the inclusive, unweighted dataset.

### 2.2 Branching out & channels
Each selection is associated with an identifier _name_, which need not be unique. Also, multiple selections can be applied from a single selection to form "branches", but these selections in separate branches need not be mutually exclusive from one another.

This approach can accommodate any arbitrary selection structure. Should the analyzer wish to resolve any ambiguities in the names of selections in different branches, it is easily done by a replacement of `filter` call with `channel` at some selection after the branching point, such that the _path_ of a selection includes the upstream selection to form a unique string.
```cpp
auto nlep_req = hww.constant(2);
auto cut_2los = cut_2l.filter<cut>("2los", [](const RVecF& lep_charge){return lep_charge.at(0)+lep_charge.at(1)==0;})(lep_Q);

// note: channel designation when branching out
// - different-flavour leptons
auto cut_2ldf = cut_2los.channel<cut>("2ldf", [](const ROOT::RVec<int>& lep_type){return lep_type.at(0)+lep_type.at(1)==24;})(lep_type);
// - same-flavour leptons
auto cut_2lsf = cut_2los.channel<cut>("2lsf", [](const ROOT::RVec<int>& lep_type){return (lep_type.at(0)+lep_type.at(1)==22)||(lep_type.at(0)+lep_type.at(1)==26);})(lep_type);

// same cuts at different branches
auto mll_cut = hww.constant(60.0);
auto cut_2ldf_sr = cut_2ldf.filter<cut>("sr")(mll < mll_cut);  // 2ldf/sr
auto cut_2lsf_sr = cut_2lsf.filter<cut>("sr")(mll < mll_cut);  // 2lsf/sr
auto cut_2ldf_wwcr = cut_2ldf.filter<cut>("wwcr")(mll > mll_cut);  // 2ldf/cr
auto cut_2lsf_wwcr = cut_2lsf.filter<cut>("wwcr")(mll > mll_cut);  // 2lsf/cr
```

The decision (pass/fail and weight value) of each selection is lazily evaluated based on for an entry only if all upstream selections have passed and when neede (see counters below).

## 3. Counting entries
### 3.1 Booking counters and accessing their results
A __counter__ is an arbitrary action performed for each entry:
- Perform the action only if its booked selection passed the cut, with knowledge of its weight.
- (Optional) receive the values from input columns to be "filled" with.

The aggregated results of this operation comprises the final output that analyzers extract from the dataset; as such, its logic is fully open to implementation via `ana::counter::logic<Result(Columns...)>`.
```cpp
// Histogram<1,float> : ana::counter::logic<std::shared_ptr<TH1F>(float)> (i.e. user-immplementable)
auto pth_2los = data.book<Histogram<1,float>>("pth",100,0,400).fill(pth).at(cut_2los);
// for each entry:
  // if (cut_2los.passed_cut()) { 
  //   pth_hist->Fill(pth, cut_2los.get_weight());
  // }
```
Accessing the result of any counter triggers the dataset processing to retrieve the needed results at once:
```cpp
// triggers dataset processing
pth_2los_res.result();  // -> std::shared_ptr<TH1>
```
### 3.3 Modular `counter`-`selection`-`counter` interactions
The `fill` and `at` operations that counters accept to enter columns and selections are completely flexible, aside from one restriction that the former must precede the latter:
- A counter can be filled with any set of columns any number of times.
- A counter can be booked at any set of selections at a time.
```cpp
// fill the histogram with pT of both leptons
auto l1n2_pt_hist = data.book<Histogram<1,float>>("l1n2_pt",20,0,100).fill(l1pt).fill(l2pt);

// make histograms for 2ldf signal & control regions
auto l1n2_pt_hists_2ldf = l1n2_pt_hist.at(cut_2ldf_sr, cut_2ldf_wwcr);

// make histograms for 2lsf signal & control regions
auto l1n2_pt_hists_2lsf = l1n2_pt_hist.at(cut_2lsf_sr, cut_2lsf_wwcr);
```
When a counter is booked at multiple selections such as the above, the result at any specific selection can be later accessed by specifying the path.
```cpp
l1n2_pt_hist_2ldf_sr = l1n2_pt_hists_2ldf["2ldf/sr"].result();
l1n2_pt_hist_2ldf_wwcr = l1n2_pt_hists_2ldf["2ldf/wwcr"].result();
```

### 3.4 Non-redundancy of counter operations

As the final consequence of the lazy-nature of the actions, each counter operation is performed once per entry only when needed as the following:
1. The counter will not perform any action unless its booked selection has passed.
2. The selection will only evaluate its cut decision only if it knows all of its upstream selections have passed, otherwise return fail.
3. If a certain column was defined in an analysis but ultimately not needed for any computation of the selection result and/or the counter operation above, it is never computed.

In the above example:
- The filled columns ($p_\text{T}^H$ and $p_\text{T}^{\ell_2}$) require at least 2 elements in the input lepton vectors, without protection against otherwise.
- The counters, nevertheless, were (correctly) booked only under a $n_\ell = 2$ selection.
- Therefore, the computation of dilepton quantities are never triggered for entries that would haven thrown an exception.

### 3.5 Bonus: dumping out results

The organization and manipulation of the results are, primarily, left completely up to the analyzer's discretion. Alternatively, a `ana::counter::summary<T>` class can also be implemented to be used by a `ana::output::dump<T>` helper function for convenience.
```cpp
// Folder : ana::counter::summary<Folder> (i.e. user-implementable)
// - make a folder with the path of all selections booked
// - and write the histogram in each folder
auto out_file = TFile::Open("hww_hists.root","recreate");
// ana::output::dump<T> is a helper function
ana::output::dump<Folder>(pth_2los_res, out_file);
ana::output::dump<Folder>(l1n2_pt_hists, out_file);
delete out_file;
```
![pth_hists](images/hww_hists.png)

## 4. Systematic variations

In general, the above approaches to construct an "analysis graph" (set of columns, selections, counters in effect) means that an arbitrary change one of its components usually must be performed by an independently created `analysis` instance.

Under a stricter definition that a __systematic variation__ constitutes a __change in a column__ that propagates to affect the outcome of an exact set of selection and counters, the variation from the nominal computation graph to another can be concretely defined and all be processed within a single analysis graph, which offers the following benefits:

- Coupled graphs guarantee that each variation and only the variation is in effect between the nominal and varied results.
- Eliminate the performance overhead associated with repeated dataset processing.

### 4.1 Varying a column

Any column can be varied via an definition of the same type, which translates to:
- `reader` of the dataset can be varied to be one of a different column name, as long as they are of the same data type.
- `constant` can be changed from any value to another.
- `equation` can be evaluated using function with the same signature and return type.
- `definition` can be instantiated with a different set of constructor arguments.
  - If the user-implemented class has a custom method to be called prior to the dataset processing, the in-memory instances can also be manually accessed.
 
```cpp
// use a different scale factor
auto el_sf = hww.read<float>("scaleFactor_ELE").vary("sf_var","scaleFactor_PILEUP");
// (non-sensical, purely for illustration)

// change mll cut to compare signal efficiencies
auto mll_cut = hww.constant(55.0).vary("mll_tight",50).vary("mll_loose",60);

// change the energy scale by +/-1%
auto Escale = hww.define([](RVecD E){return E;}).vary("lp4_up",[](RVecD E){return E*1.01;}).vary("lp4_dn",[](RVecD E){return E*0.99;});
// apply it to leptons
auto lep_pt_sel = Escale(lep_pt)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
auto lep_E_sel = Escale(lep_E)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
```
This transforms the returned `delayed` into a `varied`, which simply means that multiple variations of the action exist.

### 4.2 Propagation of variations through selections and counters

Aside from vary each column as desired, the rest of the analysis interface remains the unchanged without requiring any further treatment:
- Any column definitions whose input columns contain variations will be varied correspondingly.
- Any selections and counters evaluated out varied column will be varied correspondingly.

The propagation of variations that may (or may not) exist in different sets of columns participating in a downstream action occur "transparently", meaning:
- If two input columns each have a variation with the same name, they are considered together.
- If one column has a variation while the other doesn't, then the nominal is used from the latter.

```cpp
auto l1p4 = data.define<NthP4>(0)(lep_pt, lep_eta, lep_phi, lep_E);
l1p4.has_variation("lp4_up");  // true

// ...

auto cut_2l = data.filter<weight>("weight")(mc_weight * el_sf * mu_sf)\
                  .filter<cut>("2l")(n_lep_sel == n_lep_req);
cut_2l.has_variation("sf_var");  // true

// ...

auto pth_2ldf_vars = data.book<Histogram<1,float>>("pth",100,0,200).fill(pth).at(cut_2ldf);
pth_2ldf_vars.has_variation("lp4_up"); // true
pth_2ldf_vars.has_variation("sf_var"); // true

// additional nominal() & variation access
auto pth_nom_hist = pth_hists.nominal().result();
auto pth_var_hist = pth_hists["lp4_up"].result();
```
![pth_varied](images/pth_varied.png)

The access of multiple systematic variations and selections via their names and paths are compatible with each other
```
auto mll_vars = data.book<Histogram<1,float>>("pth",100,0,200).fill(mll).at(cut_2ldf_sr, cut_2ldf_wwcr);

auto mll_nom_2ldf_sr = mll_vars.nominal()["2ldf/sr"].result();
auto mll_var_2ldf_wwcr = mll_vars["mll_tight"]["2ldf/wwcr"].result();
```

# Known issues

- :warning: Math operators not working with GCC, works with Clang.
- :x: (PyROOT) `delayed` and `varied` manipulations not working.
