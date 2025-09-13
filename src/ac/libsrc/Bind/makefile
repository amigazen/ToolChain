# makefile for midi.library bindings

all: midi.lib midil.lib midi-lattice.lib


# Aztec C small code/data
midi.lib: midi_lib.fd
    bind midi_lib.fd -omidi.lib -rAztec


# Aztec C large code/data
midil.lib: midi_lib.fd
    bind midi_lib.fd -omidil.lib -rAztecL


# Lattice C
midi-lattice.lib: midi_lib.fd
    bind midi_lib.fd -omidi-lattice.lib -rLattice


