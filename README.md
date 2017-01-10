# AdaptiveLosslessPrediction
C++ impl of Adaptive Lossless Prediction based Image Compression (paper by Jovanovic and Lorentz)

This is a C++ implementation of a matrix minimization approach as described in the paper titled "Adaptive Lossless Prediction Based Image Compression" by R. Jovanovic and R. Lorentz <A HREF="http://mail.ipb.ac.rs/~rakaj/home/adaptivecomp.pdf">(PDF link)</A>.

The approach in the original paper was designed for grayscale pixels. Full color support is more complex, so the original simple minimization along one axis was not useful in producing small deltas for 3 axis RGB values. Instead, this implementation makes use of the MED predictor and adapts the minimal iteration order to minimum distance calculated as the sum of absolute values for each component. In addition, a second delta approach based on an initial quant to a table of 256 pixels was implemented, though the results were not impressive.
