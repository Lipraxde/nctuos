
GCC_LIB := $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

USER_LDFLAGS = -T user/user.ld

USER_SRCFILES := user/shell.c

USER_OBJS = user/shell.o

$(OBJDIR)/user/%.o: user/%.c
	$(CC) $(CFLAGS) -nostdinc -O0 -c -o $@ $<

$(OBJDIR)/user/%.o: user/%.S
	$(CC) $(CFLAGS) -nostdinc -c -o $@ $<

$(OBJDIR)/user/userprog: $(USER_OBJS) lib/libnctuos.a
	@echo + ld user/userprog
	$(LD) $(USER_LDFLAGS) $(USER_OBJS) -Llib/ -lnctuos $(GCC_LIB) -o $@
	$(OBJDUMP) -S $@ > $@.asm
	$(NM) -n $@ > $@.sym
