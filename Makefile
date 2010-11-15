pidgin-knotification.so: pidgin-knotification.cc
	$(CXX) -shared -DPIC -fPIC $(CXXFLAGS) `pkg-config --cflags --libs QtDBus purple` $(LDFLAGS) pidgin-knotification.cc -o pidgin-knotification.so

clean:
	$(RM) pidgin-knotification.so
