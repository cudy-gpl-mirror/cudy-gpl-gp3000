P7ZIP = p7zip_16.02

all: p7zip

p7zip:
	$(MAKE) -C $(P7ZIP)

clean:
	$(MAKE) -C $(P7ZIP) clean

romfs:
	$(ROMFSINST) $(P7ZIP)/bin/7za /bin/7za

.PHONY: all p7zip clean romfs
