MODE ?= Debug
GCOV ?= 0
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
MAKE = /c/MinGW/msys/1.0/bin/make.exe
endif

all: $(BUILD_DIR)/nwmng

$(BUILD_DIR)/nwmng: $(BUILD_DIR)/Makefile 
	@echo "Building $@"
	@cd $(BUILD_DIR) && $(MAKE) && cd ../..

$(BUILD_DIR)/Makefile: FORCE
	@echo "Building $@"
	@mkdir -p ${BUILD_DIR}
	@cd ${BUILD_DIR} && cmake \
		-G "Unix Makefiles" \
		-DDBG=$(DBG) \
		-DGCOV=$(GCOV) \
		-DCMAKE_BUILD_TYPE=$(MODE) \
		..

clean:
	rm -rf $(BUILD_DIR)

reb:
	$(MAKE) clean
	$(MAKE) -j12

info:
	@echo "*******************************************************************"
	@echo "MODE=[Debug/Release] to build the debug/release version"
	@echo "*******************************************************************"

FORCE:
.PHONY: clean FORCE all info reb
