SRCS=exp.cpp trs.cpp proc.cpp smt.cpp algebra.cpp template.cpp poly.cpp termord.cpp problem.cpp deprem.cpp graph.cpp reach.cpp
MAIN_SRC=main.cpp
TEST_SRC=test.cpp
TGT=$(PWD)/natt++

CLANGPP=clang++ -std=c++23 -Wfatal-errors -ftemplate-backtrace-limit=0 -Wno-switch
GPP=g++ -std=c++23 -Wfatal-errors

CPP=${GPP}
BUILD_CPP=${CPP} -O3
SANITIZE_CPP=${CPP} -O1 -fsanitize=address,alignment,undefined -fno-omit-frame-pointer
DEBUG_CPP=${CPP} -O0 -ggdb3 -fsanitize=address,undefined

DEPEND=_depend
BUILD=_build
SANITIZE=_sanitize
DEBUG=_debug

ALL_SRCS=${SRCS} ${MAIN_SRC} ${TEST_SRC}

DEPS=$(ALL_SRCS:%.cpp=$(DEPEND)/%.d)

OBJS=$(SRCS:%.cpp=$(BUILD)/%.o)
MAIN=$(MAIN_SRC:%.cpp=$(BUILD)/%.o)
TEST=$(TEST_SRC:%.cpp=$(BUILD)/%.o)

SANITIZE_OBJS=$(SRCS:%.cpp=$(SANITIZE)/%.o)
SANITIZE_MAIN=$(MAIN_SRC:%.cpp=$(SANITIZE)/%.o)
SANITIZE_TEST=$(TEST_SRC:%.cpp=$(SANITIZE)/%.o)

DEBUG_OBJS=$(SRCS:%.cpp=$(DEBUG)/%.o)
DEBUG_MAIN=$(MAIN_SRC:%.cpp=$(DEBUG)/%.o)
DEBUG_TEST=$(TEST_SRC:%.cpp=$(DEBUG)/%.o)

${TGT}: ${OBJS} ${MAIN}
	${BUILD_CPP} $^ -o $@

sanitize: ${SANITIZE_OBJS} ${SANITIZE_MAIN}
	${SANITIZE_CPP} $^ -o $@

debug: ${DEBUG_OBJS} ${DEBUG_MAIN}
	${DEBUG_CPP} $^ -o $@

tester: ${DEBUG_OBJS} ${DEBUG_TEST}
	${SANITIZE_CPP} $^ -o $@

test: tester
	./tester

error: debug syntax_errors/*.ari
	for f in syntax_errors/*.ari; do ./debug $$f; done

$(DEPEND)/%.d: %.cpp
	@mkdir -p $(@D)
	${CPP} -MM $< > $@.base
	(echo -n $(BUILD)/; cat $@.base) > $@
	(echo -n $(DEBUG)/; cat $@.base) >> $@
	(echo -n $(SANITIZE)/; cat $@.base) >> $@

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	${BUILD_CPP} -c $< -o $@

$(SANITIZE)/%.o: %.cpp
	@mkdir -p $(@D)
	${SANITIZE_CPP} -c $< -o $@

$(DEBUG)/%.o: %.cpp
	@mkdir -p $(@D)
	${DEBUG_CPP} -c $< -o $@

# TPDB 
TPDB=~/TPDB-ARI
TIMEOUT=15

tpdb_result: $(TGT) $(TPDB)
	rm -f $@
	time sh -c '\
		out=$(abspath $@);\
		cd $(TPDB)/TRS_Standard;\
		for f in */*.ari;\
		do\
			(echo -n $$f:\ ; timeout $(TIMEOUT) $(TGT) -q $$f; if [ $$? -eq 124 ]; then echo TIMEOUT; fi) | tee -a $$out;\
		done'
	grep -c 'YES\|NO' $@

tpdb_negative: $(TGT)
	rm -f $@
	out=$(abspath $@);\
	cd $(TPDB)/TRS_Standard;\
	while read f; do \
		echo -n $$f:\ ;\
		(timeout $(TIMEOUT) $(TGT) -q $$f; if [ $$? -eq 124 ]; then echo TIMEOUT; fi) | tee -a $$out;\
		if grep -q YES $$out; then echo WRONG!; exit 1; fi;\
	done < "$(PWD)/tpdb_neg.list"
	grep -c NO $@

.PHONY: clean test tpdb-negative

clean:
	rm -rf $(DEPEND) $(BUILD) $(SANITIZE) $(DEBUG) $(TGT) sanitize tester debug

-include ${DEPS}
