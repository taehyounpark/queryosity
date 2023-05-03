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
- C++17 standard compiler (tested with Clang 14 and GCC 11)
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
Existing *columns* in the dataset can be accessed by supplying their types and names.
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
The `analysis<Dataset>` actions always returns a `delayed<Action>` for some action, which represents the action having been booked, but is only executed at a later point when needed.

### 1.2 Computing new quantities
### Simplest way
Mathematical binary and unary operations supported by the underlying data types are passed through:
```cpp
// - ROOT::RVec<float> support division by double
auto GeV = ana.constant(1000.0);
auto lep_pt = lep_pt_MeV / GeV;
// etc. for all other magnitudes

// - operations need not be atomic, commposite ones are okay
auto lep_eta_max = hww.constant(2.4);
auto lep_pt_sel = lep_pt[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
// etc. for all other lep_X
```
### Custom functions
As a superset of the above approach as well as to access non-trivial methods of the underlying data, custom lambda expressions can be provided.
```cpp
// (see below for l1p4 and l2p4 definitions)
auto dilepP4 = data.define([](TLV const& p4, TLV const& q4) {return (p4+q4);})(l1p4,l2p4);
// - first argument is the functor
// - second set are the input columns
```
Note the modularity of defining the function body versus its input arguments, which enables recycling of common definitions.
```
// evaluate two different quantities out of a common definition
auto get_pt = hww.define([](TLV const& p4){return p4.Pt();});
auto l1pt = get_pt(l1p4);
auto l2pt = get_pt(l2p4);
```
### Custom classes
For more complicated definitions, they can be explicitly specified by implementing the `ana::column::defintion<Ret(Args...)>` base class.
```cpp
using TLV = TLorentzVector;
using RVecD = ROOT::RVec<double>;
class NthP4 : public ana::column::definition<TLV(RVecD, RVecD, RVecD, RVecD)>
// - define an ith TLorenzVector out of (pt,eta,phi,e) vectors
{
public:
  NthP4(unsigned int index, double scale=1.0) : 
    ana::column::definition<TLV(RVecD, RVecD, RVecD, RVecD)>(),
    m_index(index)
  {}
  virtual ~NthP4() = default;

  // implement this
  // - ana::observable<T> enables access to convertible/inherited types
  virtual TLV evaluate(ana::observable<RVecD> pt, ana::observable<RVecD> eta, ana::observable<RVecD> phi, ana::observable<RVecD> es) const override {
    TLV p4;
    p4.SetPtEtaPhiE(pt->at(m_index),eta->at(m_index),phi->at(m_index),es->at(m_index));
    return p4*m_scale;
  }

protected:
  unsigned int m_index;
  double m_scale;
  // - it is up to implementation to ensure thread-safety
  // int* g_modifiable_global_var;  // <- bad idea
};

// ...

auto l1p4 = hww.define<NthP4>(0)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);
// - first set of arguments is now the class constructor
// - second set remains the input columns
```
The computation graph formed out of these columns are guaranteed to be
- Recursion-free: the grammar forbids this by construction.
- Non-redundant: the value of a column is computed at most once per entry.
- No-copy: unless a conversion is required, their values are never copied when used as input values for other columns.

## 2. Applying selections
### 2.1 Cut versus weight
Filtering entries in a dataset is done via a *selection*, which is either a boolean decision for floating-point value that chooses to ignore or assign statistical significance to an entry, respectively.

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
Note that the two types are non-overlapping, so any mixed sequence of `<cut>` or `<weight>` can be made. Selections applied in sequence this way must be chained from their respective upstream selection, whereas the `analysis` object always represents the inclusive dataset.

### 2.2 Branching out & channels
Each selection is associated with an identifier _name_, but they need not be unique. Also, multiple selections can be applied from a common point to form "branches", but selections in separate branches also need not be mutually exclusive from one another (although it will the case in the example below coincidentally)

This approach can accommodate any arbitrary selection structure possible. Should the analyzer wish to resolve any ambiguities in the names of selections in different branches, this is easily done by a replacement of `filter` calls with `channel` at some at or after the branching point, such that the _path_ of a selection includes the upstream selection.
```cpp
// using complicated selection expressions to evaluate selection
auto nlep_req = hww.constant(2);
auto cut_2los = incl.filter<cut>("2l")(nlep_sel == nlep_req).filter<cut>("2los", [](const RVecF& lep_charge){return lep_charge.at(0)+lep_charge.at(1)==0;})(lep_Q);

// note: channel designation to branch out from here
// - different-flavour leptons, path = "2los/2ldf"
auto cut_2ldf = cut_2los.channel<cut>("2ldf", [](const ROOT::RVec<int>& lep_type){return lep_type.at(0)+lep_type.at(1)==24;})(lep_type);
// - same-flavour leptons, path = "2los/2ldf"
auto cut_2lsf = cut_2los.channel<cut>("2lsf", [](const ROOT::RVec<int>& lep_type){return (lep_type.at(0)+lep_type.at(1)==22)||(lep_type.at(0)+lep_type.at(1)==26);})(lep_type);

// even though multiple selections with the same name are applied,
// their paths remain unique from each other
auto mll_cut = hww.constant(80.0), met_cut = hww.constant(30.0);
auto cut_2ldf_sr = cut_2ldf.filter<cut>("sr")(mll > mll_cut);  // 2ldf/sr
auto cut_2lsf_sr = cut_2lsf.filter<cut>("sr")(met > met_cut);  // 2lsf/sr
auto cut_2ldf_cr = cut_2ldf.filter<cut>("cr")(mll > mll_cut);  // 2ldf/cr
auto cut_2lsf_cr = cut_2lsf.filter<cut>("cr")(met > met_cut);  // 2lsf/cr
```

As was the case for column definitions, the decisions (pass/fail and weight value) of a selection is not redundantly computed for an entry if an upstream selection is already determined to have failed.

## 3. Counting entries
### 3.1 Booking counters and accessing their results
A __counter__ is an arbitrary action performed for each entry:
- Perform the action only if its booked selection passed the cut, with knowledge of its weight.
- (Optional) receive the values from input columns to be "filled" with.

The aggregated results of this operation comprises the final output that analyzers would want to extract from the dataset; as such, any logic is open to implementable by the end-user via `ana::column::logic<Result(Columns...)>`.
```cpp
// Histogram<1,float> : ana::counter::logic<std::shared_ptr<TH1F>(float)> (i.e. user-immplementable)
auto pth_2los = data.book<Histogram<1,float>>("pth",100,0,400).fill(pth).at(cut_2los);
// what essentially happens:
  // if (cut_2los.passed_cut()) { 
  //   pth_hist->Fill(pth, cut_2los.get_weight());
  // }
```
Accessing the result of any counter triggers all the delayed actions that have been booked up to that point in the analysis:
```cpp
// trigger dataset processing
pth_2los_res.result();  // -> std::shared_ptr<TH1>
```
### 3.3 Expanded `counter`-`selection`-`counter` interactions
The above is the most trivial use-case, but the `fill` and `at` operations that counters accept to receive columns and selections are completely flexible, aside from grammatical single restriction that the former must precede the latter:
- A counter can be filled with any set of columns any number of times.
- A counter can be booked at any set of separate selections at a time.
```cpp
// - fill the histogram with pT of both leptons
auto l1n2_pt_hist = data.book<Histogram<1,float>>("l1n2_pt",20,0,100).fill(l1pt).fill(l2pt);

// - make histograms for 2ldf signal & control regions
auto l1n2_pt_hists_2ldf = l1n2_pt_hist.at(cut_2ldf_sr, cut_2ldf_cr);

// - make histograms for 2lsf signal & control regions
auto l1n2_pt_hists_2lsf = l1n2_pt_hist.at(cut_2lsf_sr, cut_2lsf_cr);
```
The modularity allows extensions of the analysis to occur naturally and additively instead of requiring an existing call be altered.

When a counter is booked at multiple selections like the above, the result at any specific selection can be later accessed by specifying the path.
```cpp
l1n2_pt_hist_2ldf_sr = l1n2_pt_hists_2ldf["2ldf/sr"].result();
l1n2_pt_hist_2ldf_cr = l1n2_pt_hists_2ldf["2ldf/cr"].result();
```

### 3.4 Non-redundancy of counter operations
As a corollary of the column and selection computation non-redundancy, the counter operations are performed exactly and only when needed. In the above example:
- The filled columns ($p_\text{T}^H$ and $p_\text{T}^{\ell_2}$) require at least 2 elements in the input lepton vectors, without protections against otherwise.
- The counters, nevertheless, were (correctly) booked only under a $n_\ell = 2$ selection.
- Therefore, the computation of dilepton quantities are never be triggered for entries that would haven otherwise thrown an exception.

In summary, the counting operation is exactly as safe as what the user specifies it to be.

### 3.5 Bonus: dumping out results

How the counter results are organized/manipulated the obtained results remains completely up to the analyzer. Alternatively, a `ana::counter::summary<T>` class can also be implemented to be used by a `ana::output::dump<T>` helper function for convenience.
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

Any column can be varied via an alternative argument of their instantiation, which translates to:
- `reader` of the dataset can be varied to be one of a different column name, as long as they are of the same data type.
- `constant` can be changed from any value to another.
- `equation` can be evaluated with any other function with the same signature and return type.
- `definition` can be created with a different set of constructor arguments.
  - A more advanced way to manually access and manipulate the in-memory instances is also possible (e.g. to call some method function added into the implemented class).


```cpp
// use a different scale factor
auto el_sf = hww.read<float>("scaleFactor_ELE").vary("sf_var","scaleFactor_PILEUP");
// (non-sensical, purely for illustration)

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

// ...

// additional nominal() & variation access
auto pth_nom_hist = pth_hists.nominal().result();
auto pth_var_hist = pth_hists["lp4_up"].result();
```
![pth_varied](images/pth_varied.png)

# Known issues

- :warning: Math operators not working with GCC, works with Clang.
- :x: (PyROOT) `delayed` and `varied` manipulations not working.