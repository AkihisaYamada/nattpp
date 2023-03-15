SRCS=exp.cpp term.cpp main.cpp
CPP=g++ -O0 -ggdb3 -std=c++20 -Wfatal-errors
BUILD=_build
OBJS=$(SRCS:%.cpp=$(BUILD)/%.o)
DEPS=$(OBJS:%.o=%.d)
TGT=./terma

${TGT}: ${OBJS}
	${CPP} $^ -o $@

test: ${TGT} test.ari
	for f in syntax_errors/*.ari; do ${TGT} $$f; done
	${TGT} test.ari

$(BUILD)/%.d: %.cpp
	@mkdir -p $(@D)
	(echo -n $(BUILD)/; ${CPP} -MM $<) > $@

$(BUILD)/%.o: %.cpp
	${CPP} -c $< -o $@

.PHONY: clean test

clean:
	rm -rf $(BUILD)

-include ${DEPS}
