# Compiler
CXX = g++
CXXFLAGS = -g -std=gnu99 -Wall -pedantic -pthread

# Single target to run complie and get all binaries
shell: test_client.x test_server.x

# Compiles and produces individual binary file
%.x:  %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Removes (cleans) all binaries with .x extension
clean:
	rm -rf *.x