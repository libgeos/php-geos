DOCKER=docker
REGISTRY=docker.kbt.io
NAME=$(REGISTRY)/php-geos/build-test

all: alpine

push: alpine-push

alpine:
	$(DOCKER) build -t $(NAME):alpine - < Dockerfile.alpine

alpine-push:
	$(DOCKER) push $(NAME):alpine
