ifndef _version
_version = 1
XBD_version:
	$(xbxdir)/tools/xbd-version $(xbxdir) | cmp -s - $(xbxdir)/embedded/xbd/xbd_af/XBD_version.h || $(xbxdir)/tools/xbd-version $(xbxdir) > $(xbxdir)/embedded/xbd/xbd_af/XBD_version.h
endif
