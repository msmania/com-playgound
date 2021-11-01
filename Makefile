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
SRCDIR=src
GTEST_SRC_DIR=D:\src\googletest
GTEST_BUILD_DIR=D:\src\googletest\build\$(ARCH)

CC=cl
RD=rd/s/q
RM=del/q
LINKER=link
TARGET_EXE=t.exe
TARGET_TEST=tests.exe
TARGET_DLL=z.dll

OBJS_EXE=\
	$(OBJDIR)\main.obj\

OBJS_TEST=\
	$(OBJDIR)\regutils.obj\
	$(OBJDIR)\tests.obj\

OBJS_DLL=\
	$(OBJDIR)\dll.res\
	$(OBJDIR)\dllmain.obj\
	$(OBJDIR)\regutils.obj\
	$(OBJDIR)\serverinfo.obj\

LIBS=\
	advapi32.lib\

LIBS_TEST=\
	advapi32.lib\
	gtest.lib\
	gtest_main.lib\

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
	/I"$(GTEST_SRC_DIR)\googletest\include"\
	/I"$(GTEST_SRC_DIR)\googlemock\include"\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/LIBPATH:"$(GTEST_BUILD_DIR)\lib\Release"\

all: $(OUTDIR)\$(TARGET_DLL) $(OUTDIR)\$(TARGET_EXE) $(OUTDIR)\$(TARGET_TEST)

$(OUTDIR)\$(TARGET_DLL): $(OBJS_DLL)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /DEF:$(SRCDIR)\dll.def /SUBSYSTEM:WINDOWS /DLL $(LIBS)\
		/PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET_EXE): $(OBJS_EXE)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /SUBSYSTEM:CONSOLE $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET_TEST): $(OBJS_TEST)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /SUBSYSTEM:CONSOLE $(LIBS_TEST) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{$(SRCDIR)}.rc{$(OBJDIR)}.res:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	rc /nologo /fo "$@" $<

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
