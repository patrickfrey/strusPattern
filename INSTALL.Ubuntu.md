Ubuntu 16.04 on x86_64, i686
----------------------------

# Build system
Cmake with gcc or clang. Here in this description we build with 
gcc >= 4.9 (has C++11 support). Build with C++98 is possible.

# Prerequisites
Install packages with 'apt-get'/aptitude.

## Required packages
	boost-all >= 1.57
	snappy-dev leveldb-dev libuv-dev
	libtre-dev ragel
	hyperscan >= 5.1

# Install hyperscan from sources
	git clone https://github.com/intel/hyperscan.git
	cd hyperscan
	git checkout tags/v5.1.1
	mkdir build
	cd build
	cmake -DBUILD_SHARED_LIBS=1 ..
	make
	make install

# Strus prerequisite packages to build and install before
	strusBase strus strusAnalyzer strusTrace strusModule

# Fetch sources
	git clone https://github.com/patrickfrey/strusPattern
	cd strusPattern
	git submodule update --init --recursive
	git submodule foreach --recursive git checkout master
	git submodule foreach --recursive git pull

# Configure build and install strus prerequisite packages with GNU C/C++
	for strusprj in strusBase strus strusAnalyzer strusTrace strusModule
	do
	git clone https://github.com/patrickfrey/$strusprj $strusprj
	cd $strusprj
	cmake -DCMAKE_BUILD_TYPE=Release -DLIB_INSTALL_DIR=lib .
	make
	make install
	cd ..
	done

# Configure build and install strus prerequisite packages with Clang C/C++
	for strusprj in strusBase strus strusAnalyzer strusTrace \
		strusModule
	do
	git clone https://github.com/patrickfrey/$strusprj $strusprj
	cd $strusprj
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" .
	make
	make install
	cd ..
	done

# Configure with GNU C/C++
	cmake -DCMAKE_BUILD_TYPE=Release .

# Configure with Clang C/C++
	cmake -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++" .

# Build
	make

# Run tests
	make test

# Install
	make install

