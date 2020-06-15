# Acknowledgements 

## SMAPS Parser

Rolf Seuster wrote the original code that used SMAPS information
to calculate properly the memory shared between processes (to enable
calculation of `PSS`, or *Proportional Set Size*).

Nathalie Rauschmayr used that to develop one of the first versions
used for production and this implementation was then ported into
prmon.

## I/O Stats

Tony Limosani added the code to measure I/O stats, as part of the
ATLAS Athena offline software stack, that was then imported into
prmon.

## Prmon Registry

Grateful acknowledgement to <https://github.com/psalvaggio/cppregpattern>
which inspired the implementaion of the registry used for monitoring
components in `registry.h`.
