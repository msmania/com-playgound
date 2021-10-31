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

CC=cl
RD=rd/s/q
RM=del/q
LINKER=link
TARGET_EXE=t.exe
TARGET_DLL=z.dll

OBJS_EXE=\
	$(OBJDIR)\main.obj\

OBJS_DLL=\
	$(OBJDIR)\dll.res\
	$(OBJDIR)\dllmain.obj\

LIBS=\

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

LFLAGS=\
	/NOLOGO\
	/DEBUG\

all: $(OUTDIR)\$(TARGET_DLL) $(OUTDIR)\$(TARGET_EXE)

$(OUTDIR)\$(TARGET_DLL): $(OBJS_DLL)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /DEF:$(SRCDIR)\dll.def /SUBSYSTEM:WINDOWS /DLL $(LIBS)\
		/PDB:"$(@R).pdb" /OUT:$@ $**

$(OUTDIR)\$(TARGET_EXE): $(OBJS_EXE)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) /SUBSYSTEM:CONSOLE $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

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
