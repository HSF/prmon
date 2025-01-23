// Copyright (C) 2018-2025 CERN
// License Apache2 - see LICENCE file

// Perform CPU burn maths calculations for a number of iterations
double burn(unsigned long iterations);

// Loop over the CPU burner until interval has expired (argument is in
// *milliseconds*)
double burn_for(float ms_interval);
