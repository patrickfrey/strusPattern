#!/bin/sh

set -e

OS=$(uname -s)

case $OS in
	Linux)
		# boost
		# cd ~
		# mkdir Downloads
		# cd Downloads
		# curl http://heanet.dl.sourceforge.net/project/boost/boost/1.57.0/boost_1_57_0.tar.bz2 -O
		# tar xvfj boost_1_57_0.tar.bz2
		# cd boost_1_57_0
		# ./bootstrap.sh
		# sudo ./b2 install -j 2 --prefix=/opt
		# cd ..
		# export BOOST_ROOT=/opt/boost_1_57_0
		sudo apt-get update -qq
		sudo apt-get install -y \
			cmake \
			libtre-dev \
			ragel \
		;;
		sudo add-apt-repository -y ppa:kojoley/boost
		sudo apt-get -y install libboost-{date-time,thread,system,filesystem,regex}1.58{-dev,.0}

	Darwin)
		brew update
		if test "X$CC" = "Xgcc"; then
			brew install gcc48 --enable-all-languages || true
			brew link --force gcc48 || true
		fi
		brew install \
			cmake \
			boost \
			gettext \
			tre \
			ragel \
			|| true
		# make sure cmake finds the brew version of gettext
		brew link --force gettext || true
		brew link leveldb || true
		brew link snappy || true
		# rebuild leveldb to use gcc-4.8 ABI on OSX (libstc++ differs
		# from stdc++, leveldb uses std::string in API functions, C
		# libraries like gettext and snappy and even boost do not 
		# have this problem)
		if test "X$CC" = "Xgcc"; then
			brew reinstall leveldb --cc=gcc-4.8
		fi
		;;
	
	*)
		echo "ERROR: unknown operating system '$OS'."
		;;
esac

