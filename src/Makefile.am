bin_PROGRAMS = hdricapture

hdricapture_SOURCES = \
    main.cpp \
	camera.cpp \
	chromedsphere.cpp \
	hdribuilder.cpp \
	rgbe.cpp

noinst_HEADERS = \
	camera.h \
	chromedsphere.h \
	hdribuilder.h \
	rgbe.h

hdricapture_CXXFLAGS = \
	$(OPENCV_CFLAGS) \
	$(BOOST_CPPFLAGS) \
	$(LCMS_CFLAGS)

hdricapture_LDADD = \
	$(OPENCV_LIBS) \
	$(BOOST_SYSTEM_LIBS) \
	$(BOOST_FILESYSTEM_LIBS) \
	$(BOOST_CHRONO_LIBS) \
	$(BOOST_THREAD_LIBS) \
	$(LCMS_LIBS)
