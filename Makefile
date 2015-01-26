SUBDIRS = src benchmark preprocess image-retrieval

.PHONY: build, clean, $(SUBDIRS)

build: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	for dir in $(SUBDIRS); do ($(MAKE) -C $$dir clean); done
