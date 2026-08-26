*
* crt/ac_start.s - process entry for AC + cclib.library
*
* Must be first on the Blink FROM line.  A68k / Metacomco SECTION model
* (no SAS/C LinkerDB, _BSSBAS, or near-data A4).  BSS is zeroed by
* LoadSeg; C globals start clear.
*
* Assemble: A68k -ocrt/ac_start.o crt/ac_start.s
*

	SECTION	1,CODE

	XREF	_ac_crt_entry

* First instruction of the first code hunk is the AmigaDOS entry point.
	jsr	_ac_crt_entry
* ac_crt_entry tears down via dos Exit and should not return.
	rts

	END
