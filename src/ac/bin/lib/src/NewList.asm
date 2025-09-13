	INCLUDE "exec/types.i"
	INCLUDE	"exec/lists.i"

	SECTION	NewList,CODE
_NewList:
	move.l	4(sp),a0
        NEWLIST a0
	rts
	XDEF	_NewList
	END
