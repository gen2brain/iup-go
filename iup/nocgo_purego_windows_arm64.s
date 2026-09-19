//go:build !cgo

#include "textflag.h"

// Windows on arm64 passes the first eight float arguments only in V0-V7, which runtime callbacks never read.
TEXT floatStubCommon<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD  $·floatScratch(SB), R10
	FMOVD F0, 0(R10)
	FMOVD F1, 8(R10)
	FMOVD F2, 16(R10)
	FMOVD F3, 24(R10)
	FMOVD F4, 32(R10)
	FMOVD F5, 40(R10)
	FMOVD F6, 48(R10)
	FMOVD F7, 56(R10)
	MOVD  $·floatTargets(SB), R10
	MOVD  (R10)(R9<<3), R10
	JMP   (R10)

TEXT floatStub0<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $0, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub1<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $1, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub2<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $2, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub3<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $3, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub4<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $4, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub5<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $5, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub6<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $6, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub7<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $7, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub8<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $8, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub9<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $9, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub10<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $10, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub11<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $11, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub12<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $12, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub13<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $13, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub14<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $14, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub15<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $15, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub16<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $16, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub17<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $17, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub18<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $18, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub19<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $19, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub20<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $20, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub21<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $21, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub22<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $22, R9
	JMP  floatStubCommon<>(SB)

TEXT floatStub23<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVD $23, R9
	JMP  floatStubCommon<>(SB)

DATA ·floatStubs+0(SB)/8, $floatStub0<>(SB)
DATA ·floatStubs+8(SB)/8, $floatStub1<>(SB)
DATA ·floatStubs+16(SB)/8, $floatStub2<>(SB)
DATA ·floatStubs+24(SB)/8, $floatStub3<>(SB)
DATA ·floatStubs+32(SB)/8, $floatStub4<>(SB)
DATA ·floatStubs+40(SB)/8, $floatStub5<>(SB)
DATA ·floatStubs+48(SB)/8, $floatStub6<>(SB)
DATA ·floatStubs+56(SB)/8, $floatStub7<>(SB)
DATA ·floatStubs+64(SB)/8, $floatStub8<>(SB)
DATA ·floatStubs+72(SB)/8, $floatStub9<>(SB)
DATA ·floatStubs+80(SB)/8, $floatStub10<>(SB)
DATA ·floatStubs+88(SB)/8, $floatStub11<>(SB)
DATA ·floatStubs+96(SB)/8, $floatStub12<>(SB)
DATA ·floatStubs+104(SB)/8, $floatStub13<>(SB)
DATA ·floatStubs+112(SB)/8, $floatStub14<>(SB)
DATA ·floatStubs+120(SB)/8, $floatStub15<>(SB)
DATA ·floatStubs+128(SB)/8, $floatStub16<>(SB)
DATA ·floatStubs+136(SB)/8, $floatStub17<>(SB)
DATA ·floatStubs+144(SB)/8, $floatStub18<>(SB)
DATA ·floatStubs+152(SB)/8, $floatStub19<>(SB)
DATA ·floatStubs+160(SB)/8, $floatStub20<>(SB)
DATA ·floatStubs+168(SB)/8, $floatStub21<>(SB)
DATA ·floatStubs+176(SB)/8, $floatStub22<>(SB)
DATA ·floatStubs+184(SB)/8, $floatStub23<>(SB)
GLOBL ·floatStubs(SB), NOPTR|RODATA, $192
