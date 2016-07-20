DOCKER=docker
REGISTRY=docker.kbt.io
NAME=$(REGISTRY)/php-geos/build-test

all: alpine trisquel

push: alpine-push trisquel-push

alpine:
	$(DOCKER) build -t $(NAME):alpine - < Dockerfile.alpine

alpine-push:
	$(DOCKER) push $(NAME):alpine

trisquel:
	$(DOCKER) build -t $(NAME):trisquel - < Dockerfile.trisquel

trisquel-push:
	$(DOCKER) push $(NAME):trisquel
