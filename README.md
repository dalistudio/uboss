# uboss

lua 默认编译不带 -fPIC ，所以会提示：
elocation R_X86_64_32 against `luaO_nilobject_' 

can not be used when making a shared object;

  recompile with -fPIC /usr/local/lib/liblua.a: could not read symbols: Bad value

所以在编译 lua 的时候需要自己加上 -fPIC ，如：
make MYCFLAGS=-fPIC linux

===========================================
编译 动态链接库
src目录修改 Makefile 
LUA_SO=liblua.so
ALL_T= $(LUA_A) $(LUA_T) $(LUAC_T) $(LUA_SO)
$(LUA_SO): $(CORE_O) $(LIB_O)
$(CC) -o $@ -shared $? -ldl -lm

上级目录 Makefile
TO_LIB= liblua.a liblua.so

==============================================


