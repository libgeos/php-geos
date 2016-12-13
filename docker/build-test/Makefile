DOCKER=docker
REGISTRY=docker.kbt.io
NAME=$(REGISTRY)/php-geos/build-test

BUILDOPTS=--build-arg http_proxy=$${http_proxy}

all: alpine trisquel

push: alpine-push trisquel-push

alpine:
	$(DOCKER) build $(BUILDOPTS) -t $(NAME):alpine - < Dockerfile.alpine

alpine-push:
	$(DOCKER) push $(NAME):alpine

trisquel:
	$(DOCKER) build $(BUILDOPTS) -t $(NAME):trisquel - < Dockerfile.trisquel

trisquel-push:
	$(DOCKER) push $(NAME):trisquel
