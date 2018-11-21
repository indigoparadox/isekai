# Microsoft Visual C++ generated build script - Do not modify

PROJ = ISEKAI
DEBUG = 1
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
PROJPATH = T:\ISEKAI\
USEMFC = 0
CC = cl
CPP = cl
CXX = cl
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC = B64.C       
FIRSTCPP =             
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /Zi /AM /Od /D "_DEBUG" /D "USE_SYNCBUFF" /D "USE_FILE_CACHE" /D "WIN16" /D "DEBUG" /D "EZXML_NOMMAP" /FR /GA /Fd"ISEKAI.PDB"
CFLAGS_R_WEXE = /nologo /W3 /AM /O1 /D "NDEBUG" /D "USE_SYNCBUFF" /D "USE_FILE_CACHE" /D "WIN16" /FR /GA 
LFLAGS_D_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /NOLOGO /NOD /PACKC:61440 /STACK:10240 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = oldnames libw mlibcew commdlg.lib olecli.lib olesvr.lib shell.lib 
LIBS_R_WEXE = oldnames libw mlibcew commdlg.lib olecli.lib olesvr.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = ISEKAI.DEF
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = B64.SBR \
		CALLBACK.SBR \
		CHANNEL.SBR \
		CHUNKER.SBR \
		CLIENT.SBR \
		CONNECT.SBR \
		EZXML.SBR \
		GRAPHICS.SBR \
		HASHMAP.SBR \
		MAIN.SBR \
		MOBILE.SBR \
		PROTO.SBR \
		SCAFFOLD.SBR \
		SERVER.SBR \
		TILEMAP.SBR \
		VECTOR.SBR \
		BSTRLIB.SBR \
		XML.SBR \
		HSDECODE.SBR \
		HSENCODE.SBR \
		SYNCBUFF.SBR \
		IRC.SBR \
		WIN16U.SBR \
		WIN16G.SBR \
		WIN16I.SBR


B64_DEP = t:\isekai\src\b64.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h


CALLBACK_DEP = t:\isekai\src\callback.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\client.h \
	t:\isekai\src\connect.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\graphics.h \
	t:\isekai\src\channel.h \
	t:\isekai\src\tilemap.h \
	t:\isekai\src\ezxml.h \
	t:\isekai\src\mobile.h \
	t:\isekai\src\chunker.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hs/hsdecode.h \
	t:\isekai\src\hs/\hs_com.h \
	t:\isekai\src\hs/\hs_conf.h \
	t:\isekai\src\hs/hsencode.h \
	t:\isekai\src\server.h \
	t:\isekai\src\proto.h \
	t:\isekai\src\ui.h \
	t:\isekai\src\input.h \
	t:\isekai\src\datafile.h


CHANNEL_DEP = t:\isekai\src\channel.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\client.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\tilemap.h \
	t:\isekai\src\ezxml.h \
	t:\isekai\src\graphics.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\datafile.h \
	t:\isekai\src\mobile.h \
	t:\isekai\src\server.h \
	t:\isekai\src\connect.h \
	t:\isekai\src\proto.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\chunker.h \
	t:\isekai\src\tinypy/tinypy.h \
	t:\isekai\src\duktape/duktape.h \
	t:\isekai\src\duktape/dukhelp.h


CHUNKER_DEP = t:\isekai\src\chunker.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hs/hsdecode.h \
	t:\isekai\src\hs/\hs_com.h \
	t:\isekai\src\hs/\hs_conf.h \
	t:\isekai\src\hs/hsencode.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\hashmap.h


CLIENT_DEP = t:\isekai\src\client.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\connect.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\graphics.h \
	t:\isekai\src\channel.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\tilemap.h \
	t:\isekai\src\ezxml.h \
	t:\isekai\src\mobile.h \
	t:\isekai\src\chunker.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hs/hsdecode.h \
	t:\isekai\src\hs/\hs_com.h \
	t:\isekai\src\hs/\hs_conf.h \
	t:\isekai\src\hs/hsencode.h \
	t:\isekai\src\server.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\proto.h \
	t:\isekai\src\input.h \
	t:\isekai\src\datafile.h \
	t:\isekai\src\ui.h \
	t:\isekai\src\tinypy/tinypy.h


CONNECT_DEP = t:\isekai\src\connect.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\ipc/syncbuff.h


EZXML_DEP = t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\ezxml.h


GRAPHICS_DEP = t:\isekai\src\graphics.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\ref.h


HASHMAP_DEP = t:\isekai\src\hashmap.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\ref.h


MAIN_DEP = t:\isekai\src\input.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\client.h \
	t:\isekai\src\server.h \
	t:\isekai\src\connect.h \
	t:\isekai\src\channel.h \
	t:\isekai\src\ui.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\graphics.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\callback.h


MOBILE_DEP = t:\isekai\src\mobile.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\client.h \
	t:\isekai\src\proto.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\server.h \
	t:\isekai\src\chunker.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hs/hsdecode.h \
	t:\isekai\src\hs/\hs_com.h \
	t:\isekai\src\hs/\hs_conf.h \
	t:\isekai\src\hs/hsencode.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\tinypy/tinypy.h


PROTO_DEP = t:\isekai\src\proto.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\client.h \
	t:\isekai\src\server.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\chunker.h


SCAFFOLD_DEP = t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\wdirent.h


SERVER_DEP = t:\isekai\src\server.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\connect.h \
	t:\isekai\src\client.h \
	t:\isekai\src\channel.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\proto.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\chunker.h


TILEMAP_DEP = t:\isekai\src\tilemap.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\ezxml.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\graphics.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\mobile.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\client.h


VECTOR_DEP = t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\bstrlib/bstrlib.h


BSTRLIB_DEP = t:\isekai\src\bstrlib\bstrlib.h


XML_DEP = t:\isekai\src\datafile.h \
	t:\isekai\src\tilemap.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\mobile.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hashmap.h


HSDECODE_DEP = t:\isekai\src\hs\hsdecode.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\hs\hs_com.h \
	t:\isekai\src\hs\hs_conf.h


HSENCODE_DEP = t:\isekai\src\hs\hsencode.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\hs\hs_com.h \
	t:\isekai\src\hs\hs_conf.h


SYNCBUFF_DEP = t:\isekai\src\ipc\syncbuff.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\vector.h


IRC_DEP = t:\isekai\src\proto.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\client.h \
	t:\isekai\src\server.h \
	t:\isekai\src\ref.h \
	t:\isekai\src\chunker.h \
	t:\isekai\src\callback.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\hashmap.h \
	t:\isekai\src\b64.h \
	t:\isekai\src\hs/hsdecode.h \
	t:\isekai\src\hs/\hs_com.h \
	t:\isekai\src\hs/\hs_conf.h \
	t:\isekai\src\hs/hsencode.h \
	t:\isekai\src\datafile.h \
	t:\isekai\src\tilemap.h \
	t:\isekai\src\mobile.h


WIN16U_DEP = t:\isekai\src\graphics.h \
	t:\isekai\src\bstrlib/bstrlib.h \
	t:\isekai\src\scaffold.h \
	t:\isekai\src\vector.h \
	t:\isekai\src\ref.h


WIN16G_DEP = 

WIN16I_DEP = 

all:	$(PROJ).EXE $(PROJ).BSC

B64.OBJ:	SRC\B64.C $(B64_DEP)
	$(CC) $(CFLAGS) $(CCREATEPCHFLAG) /c SRC\B64.C

CALLBACK.OBJ:	SRC\CALLBACK.C $(CALLBACK_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\CALLBACK.C

CHANNEL.OBJ:	SRC\CHANNEL.C $(CHANNEL_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\CHANNEL.C

CHUNKER.OBJ:	SRC\CHUNKER.C $(CHUNKER_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\CHUNKER.C

CLIENT.OBJ:	SRC\CLIENT.C $(CLIENT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\CLIENT.C

CONNECT.OBJ:	SRC\CONNECT.C $(CONNECT_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\CONNECT.C

EZXML.OBJ:	SRC\EZXML.C $(EZXML_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\EZXML.C

GRAPHICS.OBJ:	SRC\GRAPHICS.C $(GRAPHICS_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\GRAPHICS.C

HASHMAP.OBJ:	SRC\HASHMAP.C $(HASHMAP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\HASHMAP.C

MAIN.OBJ:	SRC\MAIN.C $(MAIN_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\MAIN.C

MOBILE.OBJ:	SRC\MOBILE.C $(MOBILE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\MOBILE.C

PROTO.OBJ:	SRC\PROTO.C $(PROTO_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\PROTO.C

SCAFFOLD.OBJ:	SRC\SCAFFOLD.C $(SCAFFOLD_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\SCAFFOLD.C

SERVER.OBJ:	SRC\SERVER.C $(SERVER_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\SERVER.C

TILEMAP.OBJ:	SRC\TILEMAP.C $(TILEMAP_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\TILEMAP.C

VECTOR.OBJ:	SRC\VECTOR.C $(VECTOR_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\VECTOR.C

BSTRLIB.OBJ:	SRC\BSTRLIB\BSTRLIB.C $(BSTRLIB_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\BSTRLIB\BSTRLIB.C

XML.OBJ:	SRC\DATAFILE\XML.C $(XML_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\DATAFILE\XML.C

HSDECODE.OBJ:	SRC\HS\HSDECODE.C $(HSDECODE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\HS\HSDECODE.C

HSENCODE.OBJ:	SRC\HS\HSENCODE.C $(HSENCODE_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\HS\HSENCODE.C

SYNCBUFF.OBJ:	SRC\IPC\SYNCBUFF.C $(SYNCBUFF_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\IPC\SYNCBUFF.C

IRC.OBJ:	SRC\PROTO\IRC.C $(IRC_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\PROTO\IRC.C

WIN16U.OBJ:	SRC\UI\WIN16U.C $(WIN16U_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\UI\WIN16U.C

WIN16G.OBJ:	SRC\GRAPHICS\WIN16G.C $(WIN16G_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\GRAPHICS\WIN16G.C

WIN16I.OBJ:	SRC\INPUT\WIN16I.C $(WIN16I_DEP)
	$(CC) $(CFLAGS) $(CUSEPCHFLAG) /c SRC\INPUT\WIN16I.C


$(PROJ).EXE::	B64.OBJ CALLBACK.OBJ CHANNEL.OBJ CHUNKER.OBJ CLIENT.OBJ CONNECT.OBJ \
	EZXML.OBJ GRAPHICS.OBJ HASHMAP.OBJ MAIN.OBJ MOBILE.OBJ PROTO.OBJ SCAFFOLD.OBJ SERVER.OBJ \
	TILEMAP.OBJ VECTOR.OBJ BSTRLIB.OBJ XML.OBJ HSDECODE.OBJ HSENCODE.OBJ SYNCBUFF.OBJ \
	IRC.OBJ WIN16U.OBJ WIN16G.OBJ WIN16I.OBJ $(OBJS_EXT) $(DEFFILE)
	echo >NUL @<<$(PROJ).CRF
B64.OBJ +
CALLBACK.OBJ +
CHANNEL.OBJ +
CHUNKER.OBJ +
CLIENT.OBJ +
CONNECT.OBJ +
EZXML.OBJ +
GRAPHICS.OBJ +
HASHMAP.OBJ +
MAIN.OBJ +
MOBILE.OBJ +
PROTO.OBJ +
SCAFFOLD.OBJ +
SERVER.OBJ +
TILEMAP.OBJ +
VECTOR.OBJ +
BSTRLIB.OBJ +
XML.OBJ +
HSDECODE.OBJ +
HSENCODE.OBJ +
SYNCBUFF.OBJ +
IRC.OBJ +
WIN16U.OBJ +
WIN16G.OBJ +
WIN16I.OBJ +
$(OBJS_EXT)
$(PROJ).EXE
$(MAPFILE)
c:\msvc\lib\+
c:\msvc\mfc\lib\+
$(LIBS)
$(DEFFILE);
<<
	link $(LFLAGS) @$(PROJ).CRF
	$(RC) $(RESFLAGS) $@


run: $(PROJ).EXE
	$(PROJ) $(RUNFLAGS)


$(PROJ).BSC: $(SBRS)
	bscmake @<<
/o$@ $(SBRS)
<<
