# README

There are three main steps:
- windowing: performing the windows values calculation based on the given parameter sets;
- experiment: performs k-cross fold validation and calculates the confusion matricies;
- comparing: compares the results from each different k-cross fold validation grouping by different parameters.

The windowing save and load files depending on the parameter set and the source.

The experiment depends on the parameter sets, the sources and the folding config.

The comparing depends on the previous steps.

# Code conduct

## Function declaration

[name]([...unmodified-variables], [...modified-variables]);