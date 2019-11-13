# Target library
lib := libuthread.a
objs := queue.o thread.o sem.o tps.o

CC = gcc
CFLAGS = -Wall -Werror -g

all: $(objs) $(lib)

ifneq ($(V),1)
Q = @
V = 0
endif

DEPFLAGS = -MMD -MF $(@:.o=.d)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)


lib: $(lib)
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -o $@ $^

%.x: %.o $(lib)
	@echo "LD	$@"
	@$(Q)$(CCFLAGS) -o $@ $<

%.o: %.c
	@echo "CC $@"
	$(Q)$(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

$(lib): $(objs)
	@echo "ar $@"
	$(Q)ar rcs $@ $^

clean:
	rm -f sem.o tps.o *.a *.d lib

.PHONY: clean $(lib)
