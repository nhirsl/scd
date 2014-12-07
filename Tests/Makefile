SUBDIRS := Driver

all: subdirs

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done

beautify:
	for n in $(SUBDIRS); do cd $$n; make beautify; done

