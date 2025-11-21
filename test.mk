O = out/clang-release
$O/%.o: %.cc
@echo "Would compile $< to $@"

.PHONY: test
test: $(O)/src/researchproject/routing/queuegpsr/QueueGpsr.o
