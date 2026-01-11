SHELL := /bin/bash

ORTOOLS_ROOT ?= $(CURDIR)/.local/or-tools
BUILD_DIR := build
BIN := $(BUILD_DIR)/CP-SAT

DROPBOX_URLS := \
  https://www.dropbox.com/scl/fi/y739yds3givrzuoao1b0u/C1.zip?rlkey=z22clyzvalloof4335cdosbea&dl=1 \
  https://www.dropbox.com/scl/fi/geoymn3ndrmp2rbkrrv6o/C3.zip?rlkey=bkee32fj67mlgg664vfm1jrlx&dl=1 \
  https://www.dropbox.com/scl/fi/254dl7d1vqh7o3fj5zxpg/C10.zip?rlkey=2ugfkzuo7tzro0kb1whplammv&dl=1 \
  https://www.dropbox.com/scl/fi/8v8d5mhf01vbvs29n9bh4/R3.zip?rlkey=9haa9ryoykfno81jy3u15q438&dl=1 \
  https://www.dropbox.com/scl/fi/57790j528scwlngdfiz1q/R10.zip?rlkey=owiwvl25j6h03qoi0i54705gf&dl=1 \
  https://www.dropbox.com/scl/fi/irs32pobjzxs9t6arym8o/sparse_corr.zip?rlkey=fsl6y7p2z2asg5ugc8e152xl2&dl=1 \
  https://www.dropbox.com/scl/fi/f9sznsnp78g5lgcn0ws77/sparse_rand.zip?rlkey=nh7bjqwvh7etd3v4y6jtcrrf3&dl=1

.PHONY: all build run download unzip ortools clean help

all: build

build:
	@if [[ -z "$(ORTOOLS_ROOT)" ]]; then \
		echo "ORTOOLS_ROOT is not set. Example:"; \
		echo "  export ORTOOLS_ROOT=/path/to/or-tools"; \
		exit 1; \
	fi
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DORTOOLS_ROOT="$(ORTOOLS_ROOT)"
	cmake --build $(BUILD_DIR) -j$$(nproc)

run: build
	./$(BIN) --no-download

download:
	@for url in $(DROPBOX_URLS); do \
		file=$$(basename "$$url" | cut -d'?' -f1); \
		echo "Downloading $$file..."; \
		curl -L -o "$$file" "$$url"; \
	done

unzip:
	@for z in *.zip; do \
		echo "Unzipping $$z..."; \
		unzip -o "$$z"; \
	done

ortools:
	@ORTOOLS_ROOT="$(ORTOOLS_ROOT)" bash ./scripts/build_ortools.sh

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Targets:"
	@echo "  make build            Build CP-SAT (requires ORTOOLS_ROOT)"
	@echo "  make run              Run the solver"
	@echo "  make download          Download instance archives"
	@echo "  make unzip             Unzip downloaded archives"
	@echo "  make clean             Remove built binary"
