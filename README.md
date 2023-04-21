# Features
An abstract interface library for columnar data analysis in C++.
- Computation of arbitrarily complex/nested quantities of interest.
- Coherent application of selections and/or statistical weights to entries.
- Customizable output of analysis results.
- Multi-threaded processing of the dataset.

# Quick start
The following example performs proton-proton collision data analysis using the [CERN ROOT library](https://root.cern/).
```
int main() {

  ana::multithread::enable();

  auto data = ana::analysis<RAnalysis::Tree>();

  auto n_lep = data.read<int>("nlep", "nlep");

  auto pl1 = data.read<TLorentzVector>("l1p4");
  auto pl2 = data.read<TLorentzVector>("l1p4");
  auto pl3 = data.read<TLorentzVector>("l1p4");
  auto pl4 = data.read<TLorentzVector>("l1p4");
  auto m4l = data.define<TLorentzVector>("m4l",
    [] (
      ana::observable<TLorentzVector> pl1, 
      ana::observable<TLorentzVector> pl2, 
      ana::observable<TLorentzVector> pl3, 
      ana::observable<TLorentzVector> pl4 
    ) {
      return (pl1+pl2+pl3+pl4).M();
    }
  );

  return 0;

}

dileptonInvariantMassSpectrum = data.count<RAnalysis::Histogram<1,float>>("dileptonInvariantMass", 100,)

```

# Benchmarks

| Command | `pandas` | `ana`
| --- | --- |
| filter + count | List all new or modified files |
| sum | Show file differences that haven't been staged |