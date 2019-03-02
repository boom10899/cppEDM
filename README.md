## Empirical Dynamic Modeling (EDM)
---
A C++ implementation of EDM tools. See the [EDM introduction](http://deepeco.ucsd.edu/nonlinear-dynamics-research/edm/). 

Functionality includes:
* Simplex projection (Sugihara and May 1990)
* Sequential Locally Weighted Global Linear Maps (S-map) (Sugihara 1994)
* Multivariate embeddings (Dixon et. al. 1999)
* Convergent cross mapping (Sugihara et. al. 2012)

[//]: # (* Multiview embedding (Ye and Sugihara 2016))

A C++11 standard compiler and the [Eigen C++ template library](http://eigen.tuxfamily.org/) are needed to build the library by running "make" in the src/ directory.  This copies libEDM.a into the lib/ directory, where it can be linked to user applications.  EDM functions are accessed from the libEDM.a library through the API described in the [documentation](doc/cppEDM.pdf) or in [Common.h](src/Common.h).

---
### References
Sugihara G. and May R. 1990.  Nonlinear forecasting as a way of distinguishing 
chaos from measurement error in time series. Nature, 344:734–741.

Sugihara G. 1994. Nonlinear forecasting for the classification of natural 
time series. Philosophical Transactions: Physical Sciences and 
Engineering, 348 (1688) : 477–495.

Dixon, P. A., M. Milicich, and G. Sugihara, 1999. Episodic fluctuations in larval supply. Science 283:1528–1530.

Sugihara G., May R., Ye H., Hsieh C., Deyle E., Fogarty M., Munch S., 2012.
Detecting Causality in Complex Ecosystems. Science 338:496-500.

Ye H., and G. Sugihara, 2016. Information leverage in interconnected 
ecosystems: Overcoming the curse of dimensionality. Science 353:922–925.
