# vim: set noexpandtab tabstop=4:
HAL_MAKEFILE="""include env.make
FLAGS=-I.
FLAGS+=-I${HAL_PATH}
FLAGS+=-I${HAL_PATH}/drivers
FLAGS+=-I${XBD_PATH}/xbd_af

ifdef HAL_T_PATH
FLAGS+=-I${HAL_T_PATH}
FLAGS+=-I${HAL_T_PATH}/drivers
endif

FLAGS+=-MD

# Disable asserts
FLAGS+=-DNDEBUG

CC+=${FLAGS}

BUILDDIR=build

SRCS += $(wildcard ${HAL_PATH}/*.c ${HAL_PATH}/drivers/*.c)
ifdef HAL_T_PATH
SRCS += $(wildcard ${HAL_T_PATH}/*.c ${HAL_T_PATH}/drivers/*.c)
endif
SRCS_ASM = $(wildcard ${HAL_PATH}/*.s ${HAL_PATH}/drivers/*.s)

OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cpp=%.o)
OBJS := $(OBJS:%.s=%.o)
OBJS := $(OBJS:%.S=%.o)
OBJS := $(patsubst ${HAL_PATH}%,${BUILDDIR}%,${OBJS})
OBJS_ASM := $(SRCS_ASM:%.s=%.o)
OBJS_ASM := $(patsubst ${HAL_PATH}%,${BUILDDIR}%,${OBJS_ASM})
ifdef $HAL_T_PATH
OBJS := $(patsubst ${HAL_T_PATH}%,${BUILDDIR}%,${OBJS})
endif


all: ${OBJS} ${OBJS_ASM}

${OBJS}: Makefile env.make

${BUILDDIR}:
	@echo MKDIR
	@echo ${OBJS}|xargs dirname|xargs mkdir -p

${BUILDDIR}/%.o: ${HAL_PATH}/%.c |${BUILDDIR}
	@echo "CC  ${<}";
	@${CC} -o ${@} -c ${<}

${BUILDDIR}/%.o: ${HAL_PATH}/%.s |${BUILDDIR}
	@echo "CC  ${<}";
	@${CC} -o ${@} -c ${<}

clean:
	@echo CLEAN
	@rm -f ${OBJS} ${DEPS}

.PHONY: clean
ifneq (${MAKECMDGOALS},clean)
DEPS := $(OBJS:%.o=%.d)
-include ${DEPS} __dummy__
endif"""

MAKEFILE="""include env.make
FLAGS+=-I.
FLAGS+=-L${HAL_PATH}
FLAGS+=-I${HAL_PATH}
FLAGS+=-I${HAL_PATH}/drivers
FLAGS+=-I${XBD_PATH}/xbd_af
FLAGS+=-I${XBD_PATH}/xbd_app
FLAGS+=-I${IMPL_PATH}
FLAGS+=-I${XBD_PATH}/xbd_op/${OP}
FLAGS+=-I${ALGOPACK_PATH}/include
#FLAGS+=-I${ALGOPACK_PATH}/../external/libopencm3/include
FLAGS+=$(foreach dep,${DEP_PATHS}, -I${dep})

FLAGS+=-MD

# Disable asserts
FLAGS+=-DNDEBUG

CC+=${FLAGS}
#CC+=-L${ALGOPACK_PATH}/../external/libopencm3/lib
CXX+=${FLAGS}
#CXX+=-L${ALGOPACK_PATH}/../external/libopencm3/lib

BUILDDIR=build

SRCS += $(wildcard ${XBD_PATH}/xbd_af/*.c ${XBD_PATH}/xbd_app/*.c)
SRCS += $(wildcard ${XBD_PATH}/xbd_op/${OP}/*.c )
SRCS += $(shell find ${IMPL_PATH} -name "*.c" -or -name "*.cpp" -or -name "*.[Ss]")
SRCS += $(foreach dep,${DEP_PATHS}, \
    $(shell find ${dep} -name "*.c" -or -name "*.cpp" -or -name "*.[Ss]"))

OBJS := $(SRCS:%.c=%.o)
OBJS := $(OBJS:%.cpp=%.o)
OBJS := $(OBJS:%.s=%.o)
OBJS := $(OBJS:%.S=%.o)
OBJS := $(patsubst ${HAL_PATH}%,${HAL_OBJS}%,${OBJS})
OBJS := $(patsubst ${XBD_PATH}%,${BUILDDIR}/xbd%,${OBJS})
OBJS := $(patsubst ${ALGOPACK_PATH}%,${BUILDDIR}/impl%,${OBJS})

OBJS += $(shell find ${HAL_OBJS} -name "*.o")

all: xbdprog.bin xbdprog.hex

${OBJS}: Makefile env.make

${BUILDDIR}:
	@echo MKDIR
	@echo ${OBJS}|xargs dirname|xargs mkdir -p

${BUILDDIR}/xbd/%.o: ${XBD_PATH}/%.c |${BUILDDIR}
	@echo "CC  ${<}";
	@${CC} -o ${@} -c ${<}

${BUILDDIR}/impl/%.o: ${ALGOPACK_PATH}/%.c |${BUILDDIR}
	@echo "CC  ${<}";
	@${CC} -o ${@} -c ${<}

${BUILDDIR}/impl/%.o: ${ALGOPACK_PATH}/%.s |${BUILDDIR}
	@echo "CC  ${<}";
	@${CC} -o ${@} -c ${<}

${BUILDDIR}/impl/%.o: ${ALGOPACK_PATH}/%.cpp |${BUILDDIR}
	@echo "CXX  ${<}";
	@${CXX} -o ${@} -c ${<}

xbdprog.bin: ${OBJS}
	@echo "LINK";
	@${CC} -o ${@} ${OBJS}

xbdprog.hex: xbdprog.bin
	@echo "POSTLINK";
	@${POSTLINK}

clean:
	@echo CLEAN
	@rm -rf xbdprog.bin xbdprog.hex build

.PHONY: clean
ifneq (${MAKECMDGOALS},clean)
DEPS := $(OBJS:%.o=%.d)
-include ${DEPS} __dummy__
endif"""

O_H="""#ifndef ${o}_H
#define ${o}_H

#include "${op}.h"
${op_macros}
#define ${o}_PRIMITIVE "${p}"
#define ${o}_IMPLEMENTATION ${op}_IMPLEMENTATION
#define ${o}_VERSION ${op}_VERSION

#endif"""

OP_H="""#ifndef ${op}_H
#define ${op}_H

${api_h}

#ifdef __cplusplus
extern "C" {
#endif
${opi_prototypes}
#ifdef __cplusplus
}
#endif

${op_macros}

#define ${op}_IMPLEMENTATION "${i}"
#ifndef ${opi}_VERSION
#define ${opi}_VERSION "-"
#endif
#define ${op}_VERSION ${opi}_VERSION

#endif"""





