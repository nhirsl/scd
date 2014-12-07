SUBDIRS =  HelloWorld SimpleCharacterDriver Docs Tests Programs

all: subdirs

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done

beautify:
	for n in $(SUBDIRS); do cd $$n; make beautify; cd ..; done

install:
	cd SimpleCharacterDriver/scripts; ./scd_load.sh; cd ../../

