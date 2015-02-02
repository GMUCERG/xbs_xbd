@echo off
setlocal
set XBD_VERSION_LOCATION=%XBXDIR%\embedded\xbd\xbd_af\XBD_version.h
set XBD_TMP=%XBXDIR%\embedded\xbd\xbd_af\XBD_version.h.tmp
set LC_ALL=C
set TZ=UTC
svn help 2>&1 > NUL
if ERRORLEVEL 1 exit %ERRORLEVEL%
if NOT EXIST .svn exit 1
FOR /F "usebackq tokens=1,2,3,4,5,6,7,8,9 delims=,-: " %%a IN (`svn info %XBXDIR%`) DO (
	if "%%a" == "Revision" echo #define XBX_REVISION "%%b" > %XBD_TMP%
	if "%%c" == "Date" ( 
		echo #define XBX_VERSION_DATE "%%d%%e%%f" >> %XBD_TMP%
		echo #define XBX_VERSION_TIME "%%g%%h" >> %XBD_TMP%
	)
)

IF NOT EXIST %XBD_VERSION_LOCATION% (
	move %XBD_TMP% %XBD_VERSION_LOCATION% > NUL
	goto END
)

FC %XBD_VERSION_LOCATION% %XBD_TMP% > NUL
IF ERRORLEVEL 1 (
	del %XBD_VERSION_LOCATION% > NUL
	move %XBD_TMP% %XBD_VERSION_LOCATION% > NUL
)
IF EXIST %XBD_TMP% del %XBD_TMP% > NUL
:END
