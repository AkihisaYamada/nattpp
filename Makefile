SRCS=exp.cpp trs.cpp srs.cpp proc.cpp smt.cpp algebra.cpp poly.cpp problem.cpp
MAIN_SRC=main.cpp
TEST_SRC=test.cpp
TGT=terma

CPP=g++ -O3 -std=c++20 -Wfatal-errors
DEBUG_CPP=g++ -O0 -ggdb3 -std=c++20 -Wfatal-errors

DEPEND=_depend
BUILD=_build
DEBUG=_debug

ALL_SRCS=${SRCS} ${MAIN_SRC} ${TEST_SRC}

DEPS=$(ALL_SRCS:%.cpp=$(DEPEND)/%.d)
OBJS=$(SRCS:%.cpp=$(BUILD)/%.o)
MAIN=$(MAIN_SRC:%.cpp=$(BUILD)/%.o)
TEST=$(TEST_SRC:%.cpp=$(BUILD)/%.o)
DEBUG_OBJS=$(SRCS:%.cpp=$(DEBUG)/%.o)
DEBUG_MAIN=$(MAIN_SRC:%.cpp=$(DEBUG)/%.o)
DEBUG_TEST=$(TEST_SRC:%.cpp=$(DEBUG)/%.o)

${TGT}: ${OBJS} ${MAIN}
	${CPP} $^ -o $@

run: ${TGT}
	./${TGT} test.ari

debug: ${DEBUG_OBJS} ${DEBUG_MAIN}
	${DEBUG_CPP} $^ -o $@

tester: ${DEBUG_OBJS} ${DEBUG_TEST}
	${CPP} $^ -o $@

test: tester
	./tester

error: debug syntax_errors/*.ari
	for f in syntax_errors/*.ari; do ./debug $$f; done

$(DEPEND)/%.d: %.cpp
	@mkdir -p $(@D)
	${CPP} -MM $< > $@.base
	(echo -n $(BUILD)/; cat $@.base) > $@
	(echo -n $(DEBUG)/; cat $@.base) >> $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	${CPP} -c $< -o $@

$(DEBUG)/%.o: %.cpp
	@mkdir -p $(@D)
	${DEBUG_CPP} -c $< -o $@

.PHONY: clean test

clean:
	rm -rf $(DEPEND) $(BUILD) $(DEBUG)

-include ${DEPS}
