
CXX = clang++ -std=c++20
CFLAGS = -Og -g3
CXXFLAGS = $(CFLAGS) -Wall -Wextra
LDFLAGS = -ldl -lm  -lreadline
target = run

src = $(wildcard *.cpp)
obj = $(src:.cpp=.o)
dep = $(obj:.o=.d)


$(target): $(obj)
	$(CXX) -o $@ $^ $(LDFLAGS)

-include $(dep)

# rule to generate a dep file by using the C preprocessor
# (see man cpp for details on the -MM and -MT options)
%.d: %.cpp
	$(CXX) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $(DBGFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f $(obj) $(target)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: rebuild
rebuild: clean cleandep $(target)