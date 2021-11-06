!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSEIF "$(PLATFORM)"=="arm64"
ARCH=arm64
!ELSEIF "$(PLATFORM)"=="arm"
ARCH=arm
!ELSE
ARCH=x86
!ENDIF

OUTDIR=bin\$(ARCH)
OBJDIR=obj\$(ARCH)
GENDIR=gen\$(ARCH)
SRCDIR=src
GTEST_SRC_DIR=D:\src\googletest
GTEST_BUILD_DIR=D:\src\googletest\build\$(ARCH)

CC=cl
RD=rd/s/q
RM=del/q
LINKER=link
TARGET_EXE=t.exe
TARGET_SERVER=s.exe
TARGET_DLL=z.dll

OBJS_EXE=\
	$(OBJDIR)\main.obj\
	$(OBJDIR)\regutils.obj\
	$(OBJDIR)\shared.obj\
	$(OBJDIR)\tests.obj\
	$(OBJDIR)\uuids.obj\

OBJS_DLL=\
	$(OBJDIR)\dll.res\
	$(OBJDIR)\dllmain.obj\
	$(OBJDIR)\factory.obj\
	$(OBJDIR)\marshalable.obj\
	$(OBJDIR)\regutils.obj\
	$(OBJDIR)\serverinfo.obj\
	$(OBJDIR)\shared.obj\
	$(OBJDIR)\uuids.obj\

OBJS_SERVER=\
	$(OBJDIR)\exe.res\
	$(OBJDIR)\factory.obj\
	$(OBJDIR)\marshalable.obj\
	$(OBJDIR)\regutils.obj\
	$(OBJDIR)\serverinfo.obj\
	$(OBJDIR)\servermain.obj\
	$(OBJDIR)\shared.obj\
	$(OBJDIR)\uuids.obj\

LIBS=\
	advapi32.lib\
	gtest.lib\
	gtest_main.lib\
	ole32.lib\
	shell32.lib\
	shlwapi.lib\
	user32.lib\

CFLAGS=\
	/nologo\
	/c\
	/std:c++17\
	/Od\
	/W4\
	/Zi\
	/Zc:wchar_t\
	/EHsc\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/I"$(GENDIR)"\
	/I"$(GTEST_SRC_DIR)\googletest\include"\
	/I"$(GTEST_SRC_DIR)\googlemock\include"\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/LIBPATH:"$(GTEST_BUILD_DIR)\lib\Release"\

MIDL_FLAGS=\
	/nologo\
	/define_guids\
	/dlldata unused_dlldata.c\
	/proxy unused_proxy.c\
	/iid uuids.c\
	/h interfaces.h\
	/out $(GENDIR)\

all: $(OUTDIR)\$(TARGET_DLL) $(OUTDIR)\$(TARGET_SERVER) $(OUTDIR)\$(TARGET_EXE)

$(OUTDIR)\$(TARGET_DLL): $(OBJS_DLL)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /DEF:$(SRCDIR)\dll.def /SUBSYSTEM:WINDOWS /DLL $(LIBS)\
		/PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET_EXE): $(OBJS_EXE)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /SUBSYSTEM:CONSOLE $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET_SERVER): $(OBJS_SERVER)
	@if exist $(OUTDIR)\$(TARGET_SERVER) $(OUTDIR)\$(TARGET_SERVER) --stop
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /SUBSYSTEM:WINDOWS $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{$(GENDIR)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{$(SRCDIR)}.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	rc /d $(ARCH) /nologo /fo "$@" $<

$(SRCDIR)\marshalable.cpp: $(GENDIR)\interfaces.h

$(SRCDIR)\dll.rc: $(GENDIR)\interfaces.h
$(SRCDIR)\exe.rc: $(GENDIR)\interfaces.h

$(GENDIR)\interfaces.h: $(SRCDIR)\interfaces.idl
	@if not exist $(GENDIR) mkdir $(GENDIR)
	midl $(MIDL_FLAGS) $?

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(GENDIR) $(RD) $(GENDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
