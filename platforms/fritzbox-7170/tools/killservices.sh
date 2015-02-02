#!/bin/sh
killall telefon
killall upnpd
killall ctlmgr
killall pbd
killall voipd
killall multid
killall dsld
rmmod kdsldmod
rmmod usbahcicore 
rmmod capi_codec 
rmmod Piglet 
rmmod isdn_fbox_fon4
