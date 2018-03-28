ifeq ($(OS),Windows_NT)
	PLATFORM = win32
	ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
		ARCH = amd64
	else
		ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
			ARCH = amd64
		endif
		ifeq ($(PROCESSOR_ARCHITECTURE),x86)
			ARCH = x86
		endif
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		PLATFORM = linux
		UNAME_P := $(shell uname -p)
		ifeq ($(UNAME_P),x86_64)
			ARCH = amd64
		endif
		ifneq ($(filter %86,$(UNAME_P)),)
			ARCH = x86
		endif
		ifneq ($(filter arm%,$(UNAME_P)),)
			ARCH = arm
		endif
	endif
	ifeq ($(UNAME_S),Darwin)
		PLATFORM = darwin
		ARCH = amd64
	endif
endif
