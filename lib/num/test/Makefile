TEST_SRCS = $(wildcard test*.c)
TEST_OBJS = $(TEST_SRCS:.c=.o)
RUNNERS = $(TEST_SRCS:%.c=runner_%)
RUN_TARGETS = $(TEST_SRCS:%.c=run_%)

.PHONY: test clean

# Without this make treats the runners as intermediate files of the
# runner_% -> run_% pattern chain and deletes them after every run, which
# relinks all three every time and makes SEED=<n> ./runner_test_N replay
# impossible.
.SECONDARY: $(RUNNERS)

test: $(RUN_TARGETS)
	@echo "All tests completed in $(DIR)."

run_%: runner_%
	@echo "running $< $(DIR)"
	@./$<

runner_%: %.o $(DBG_FULL_FILE)
	@echo "building test $@ $(DIR)"
	@gcc -o $@ $^ $(FLAGS) $(FLAGS_DBG) $(FLAGS_EXE) $(FLAGS_EXTRA)

%.o: %.c
	@echo "building test object $@ $(DIR)"
	@gcc -o $@ $< $(FLAGS) $(FLAGS_DBG) $(FLAGS_CMP) $(FLAGS_EXTRA)

-include $(TEST_SRCS:.c=.d)

clean:
	rm -rf $(TEST_OBJS) $(RUNNERS) $(TEST_SRCS:.c=.d)
