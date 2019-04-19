
$(OBJDIR)/lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -nostdinc -O0 -c -o $@ $<

$(OBJDIR)/lib/%.o: lib/%.S
	$(CC) $(CFLAGS) -nostdinc -c -o $@ $<

$(OBJDIR)/lib/libnctuos.a: $(LIB_OBJS)
	@echo + ar $@
	$(AR) r $@ $(LIB_OBJS)
