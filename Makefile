SRCS=exp.cpp main.cpp
CPP=g++ -O0 -ggdb3 -std=c++20 -Wfatal-errors
BUILD=_build
OBJS=$(SRCS:%.cpp=$(BUILD)/%.o)
DEPS=$(OBJS:%.o=%.d)

a.exe: ${OBJS}
	${CPP} $^ -o $@

$(BUILD)/%.d: %.cpp
	@mkdir -p $(@D)
	(echo -n $(BUILD)/; ${CPP} -MM $<) > $@

$(BUILD)/%.o: %.cpp
	${CPP} -c $< -o $@

.PHONY: clean test

clean:
	rm -rf $(BUILD)

-include ${DEPS}
