//go:build !cgo

#include "textflag.h"

// Win64 passes the first four float arguments only in X0-X3, which runtime callbacks never read.
TEXT floatStubCommon<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ X0, ·floatScratch+0(SB)
	MOVQ X1, ·floatScratch+8(SB)
	MOVQ X2, ·floatScratch+16(SB)
	MOVQ X3, ·floatScratch+24(SB)
	LEAQ ·floatTargets(SB), R10
	MOVQ (R10)(AX*8), AX
	JMP  AX

TEXT floatStub0<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $0, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub1<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $1, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub2<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $2, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub3<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $3, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub4<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $4, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub5<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $5, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub6<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $6, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub7<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $7, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub8<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $8, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub9<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $9, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub10<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $10, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub11<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $11, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub12<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $12, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub13<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $13, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub14<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $14, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub15<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $15, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub16<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $16, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub17<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $17, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub18<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $18, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub19<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $19, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub20<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $20, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub21<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $21, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub22<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $22, AX
	JMP  floatStubCommon<>(SB)

TEXT floatStub23<>(SB), NOSPLIT|NOFRAME, $0-0
	MOVQ $23, AX
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
