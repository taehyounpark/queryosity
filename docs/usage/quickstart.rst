Quickstart
===========

.. code-block:: cpp
   :caption: Opening a dataset

    ana::multithread::enable();
    auto df = ana::dataflow<Tree>( {"hww.root"}, "mini" );

.. code-block:: cpp
   :caption: Reading columns

    auto var = df.read<double>("column");

.. code-block:: cpp
   :caption: Defining new quantities

    auto one = df.constant(1);
    auto two = df.define([](int x){return x+1;})(one);

.. code-block:: cpp
   :caption: Applying selections

    using cut = ana::selection::cut;
    using weight = ana::selection::weight;

    auto incl = df.filter<cut>("incl",[](){return true;})();
    auto doubled = incl.filter<weight>("doubled")(two);

.. code-block:: cpp
   :caption: Booking aggregations

    // Hist<1,float> : user-implemented
    auto hist = df.book<WeightedAverage>().fill(one).at(doubled);
    hist.result();  // 100
