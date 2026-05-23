del /Q *.hex
del /Q *.BK-1
del /Q *.LJD
del /Q *.jscope

rd /Q /S APP\\Objects
rd /Q /S APP\\Listings
rd /Q /S APP\\build
rd /Q /S APP\\Docs
del /Q APP\\*.bak
del /Q APP\\*.dep
del /Q APP\\*.BK-1
del /Q APP\\*.LJD
del /Q APP\\*.scvd
del /Q APP\\JLink*
del /Q APP\\project.uvgui.*

rd /Q /S BOOT\\Objects
rd /Q /S BOOT\\Listings
rd /Q /S BOOT\\build
rd /Q /S BOOT\\Docs
del /Q BOOT\\*.bak
del /Q BOOT\\*.dep
del /Q BOOT\\*.BK-1
del /Q BOOT\\*.LJD
del /Q BOOT\\*.scvd
del /Q BOOT\\JLink*
del /Q BOOT\\project.uvgui.*