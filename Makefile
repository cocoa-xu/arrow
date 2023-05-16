ifndef MIX_APP_PATH
	MIX_APP_PATH=$(shell pwd)
endif

PRIV_DIR = $(MIX_APP_PATH)/priv
NIF_SO = $(PRIV_DIR)/arrow_nif.so
C_SRC = $(shell pwd)/c_src
ifdef CMAKE_TOOLCHAIN_FILE
	CMAKE_CONFIGURE_FLAGS=-D CMAKE_TOOLCHAIN_FILE="$(CMAKE_TOOLCHAIN_FILE)"
endif

CMAKE_BUILD_TYPE ?= Release
DEFAULT_JOBS ?= 1
CMAKE_ARROW_BUILD_DIR = $(MIX_APP_PATH)/cmake_arrow
CMAKE_ARROW_OPTIONS ?= ""
MAKE_BUILD_FLAGS ?= -j$(DEFAULT_JOBS)

.DEFAULT_GLOBAL := build

build: $(NIF_SO)
	@echo > /dev/null

$(NIF_SO):
	@ mkdir -p "$(PRIV_DIR)"
	@ if [ ! -f "${NIF_SO}" ]; then \
		mkdir -p "$(CMAKE_ARROW_BUILD_DIR)" && \
		cd "$(CMAKE_ARROW_BUILD_DIR)" && \
		cmake --no-warn-unused-cli \
			-D CMAKE_BUILD_TYPE="$(CMAKE_BUILD_TYPE)" \
			-D C_SRC="$(C_SRC)" \
			-D MIX_APP_PATH="$(MIX_APP_PATH)" \
			-D PRIV_DIR="$(PRIV_DIR)" \
			-D ERTS_INCLUDE_DIR="$(ERTS_INCLUDE_DIR)" \
			$(CMAKE_CONFIGURE_FLAGS) $(CMAKE_ARROW_OPTIONS) "$(shell pwd)" && \
		make "$(MAKE_BUILD_FLAGS)" && \
		cp "$(CMAKE_ARROW_BUILD_DIR)/arrow_nif.so" "$(NIF_SO)" ; \
	fi
