DOCKER=docker
REGISTRY=geos-docker.osgeo.org
NAME=$(REGISTRY)/geos/php-build-test

BUILDOPTS=--build-arg http_proxy=$${http_proxy}

.PHONY: help
help:
	@echo "Available targets:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

all: alpine trisquel ## Build all docker images

push: alpine-push trisquel-push ## Push all docker images

alpine: ## Build the alpine docker image
	$(DOCKER) build $(BUILDOPTS) -t $(NAME):alpine - < Dockerfile.alpine

alpine-push: ## Push the alpine docker image
	$(DOCKER) push $(NAME):alpine

trisquel: ## Build the trisquel docker image
	$(DOCKER) build $(BUILDOPTS) -t $(NAME):trisquel - < Dockerfile.trisquel

trisquel-push: ## Push the trisquel docker image
	$(DOCKER) push $(NAME):trisquel

login: ## Login into the docker registry
	$(DOCKER) login $(NAME)
