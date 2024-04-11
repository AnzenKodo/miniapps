MOD_NAME := $(shell go list -m)

build:
	go build -ldflags="-X 'main.MOD_NAME=$(MOD_NAME)'" .

clean:
	rm gfs
