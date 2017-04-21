# HDHRProxyIPTV (Enhanced and Fixed)

HDHRProxyIPTV is an application for Windows that simulates the behavior of the server HDHomeRun© device
in relation to clients, that will act as proxy to receive the flow of channels from different sources to
a real tuner, as they could be multicast IPTV sources. The access protocol that has been implemented to 
receive these flows is the HTTP protocol. These flows are sent on demand to customers in the protocol 
they expect. You can consider this tool as one HDHomeRun Emulator.


Notes:

* The HDHRProxyIPTV project uses the original library of HDHomeRun, libhdhomerun, 
  (http://www.silicondust.com/support/downloads/linux/ -> libhdhomerun (source)), 
  specifically the version 20100213.
  (Now are included the library headers and .lib binary inside "\HDHRProxyIPTV\libs" subdirectory)

* The project can be compiled with Visual Studio 2013 & 2015 Community Edition.

* You can found one precompiled Windows binarie inside the project "\Release" subdirectory.

* Edit the file "HDHRProxyIPTV_MappingList.ini" for configure your environment.

* You can execute this program in a Linux environment using Wine.

* Tested programs to feed this proxy:
  - udpxy: http://sourceforge.net/projects/udpxy/
  - UDP-to-HTTP proxy (Windows): http://borpas.info/utils
  - minisatip: http://github.com/catalinii/minisatip
