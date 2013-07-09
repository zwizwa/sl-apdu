sl-apdu
=======

ISO7816 APDU protocol analyzer for Saleae Logic

This project is not ready for general use.  The ISO7816 parser
currently uses a *hardcoded* ATR/PTS handshake for a specific SIM
card.

It could probably be replaced by the code in:
http://bb.osmocom.org/trac/wiki/SIMtrace/Firmware
git://git.gnumonks.org/openpcd.git

Though, the ISO7816 parser in the openpcd project is mixed with
AT91SAM7-specific code.  With some refactoring both modules could use
the same code.


Uses APDU splitter from:
http://bb.osmocom.org/trac/wiki/SIMtrace
git://git.osmocom.org/simtrace.git

