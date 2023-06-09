# Compiler
CXX = g++
CXXFLAGS = -Wall -pedantic -pthread

# Single target to run complie and get all binaries
shell: chat_server.x chat_client.x

# Compiles and produces individual binary file
%.x:  %.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

# Removes (cleans) all binaries with .x extension
clean:
	rm -rf *.x