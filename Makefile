# Compiler
CXX = g++
CXXFLAGS = -g -Wall -pedantic -pthread

# Single target to run complie and get all binaries
shell: server.x client.x

# Compiles and produces individual binary file
%.x:  %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Removes (cleans) all binaries with .x extension
clean:
	rm -rf *.x