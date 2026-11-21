
#include $(KERNEL_DIR)/.config
CURRENT_DIR=$(shell pwd)
#RSDK_LIB_DIR=$(shell  echo $(PATH) | cut -f1 -d:)
PRE_INSTALL_LIB_DIR=$(CURRENT_DIR)/alsa_lib_tmp
SUBDIR_ALSA_LIB := alsa-lib-1.2.5
SUBDIR_ALSA_UTILS := alsa-utils-1.2.5
CFLAGS += -Os
SUB_EXECS =
ALSA_LIB = alsa_lib

all:$(ALSA_LIB)
	if [ ! -e ./$(SUBDIR_ALSA_UTILS)/Makefile ]; then \
		cd ./$(SUBDIR_ALSA_UTILS); \
		LD="$(CROSS_COMPILE)ld" CC="$(CROSS_COMPILE)gcc -fPIC" aclocal && autoreconf -f && automake --add-missing &&  ./configure --disable-alsamixer --disable-alsatest --disable-alsaconf --disable-alsaloop --disable-nls --disable-xmlto --build=i686-pc-linux-gnu --host=arm-linux --disable-alsamixer  --with-alsa-prefix=$(PRE_INSTALL_LIB_DIR)/lib  --with-alsa-inc-prefix=$(PRE_INSTALL_LIB_DIR)/include; \
	fi
	make -C $(SUBDIR_ALSA_UTILS)


alsa_lib:
	if [ ! -e ./$(SUBDIR_ALSA_LIB)/Makefile ]; then \
		cd ./$(SUBDIR_ALSA_LIB); \
		LD="$(CROSS_COMPILE)ld" CC="$(CROSS_COMPILE)gcc -fPIC" CFLAGS="-Os -I$(PRE_INSTALL_LIB_DIR)/include" LDFLAGS="-L$(PRE_INSTALL_LIB_DIR)/lib -lm" LIBS="-L$(PRE_INSTALL_LIB_DIR)/lib -lm" aclocal && autoreconf -f && automake --add-missing && ./configure --disable-python --build=i686-pc-linux-gnu --with-debug --host=arm-linux --enable-static=no --enable-shared=yes  --with-versioned=no --with-debug=yes; \
	fi
	make -C $(SUBDIR_ALSA_LIB)

	#rm -rf $(PRE_INSTALL_LIB_DIR)/include/alsa
	rm -rf $(PRE_INSTALL_LIB_DIR)
	mkdir $(PRE_INSTALL_LIB_DIR)
	mkdir $(PRE_INSTALL_LIB_DIR)/include
	mkdir $(PRE_INSTALL_LIB_DIR)/include/alsa
	mkdir $(PRE_INSTALL_LIB_DIR)/lib
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/use-case.h $(PRE_INSTALL_LIB_DIR)/include/alsa/use-case.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/control_external.h $(PRE_INSTALL_LIB_DIR)/include/alsa/control_external.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/pcm_external.h $(PRE_INSTALL_LIB_DIR)/include/alsa/pcm_external.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/pcm_extplug.h $(PRE_INSTALL_LIB_DIR)/include/alsa/pcm_extplug.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/pcm_ioplug.h $(PRE_INSTALL_LIB_DIR)/include/alsa/pcm_ioplug.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/pcm_rate.h $(PRE_INSTALL_LIB_DIR)/include/alsa/pcm_rate.h
	cp -fp ./$(SUBDIR_ALSA_LIB)/include/asoundlib.h $(PRE_INSTALL_LIB_DIR)/include/alsa/asoundlib.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/asoundef.h $(PRE_INSTALL_LIB_DIR)/include/alsa/asoundef.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/version.h $(PRE_INSTALL_LIB_DIR)/include/alsa/version.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/global.h $(PRE_INSTALL_LIB_DIR)/include/alsa/global.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/input.h $(PRE_INSTALL_LIB_DIR)/include/alsa/input.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/output.h $(PRE_INSTALL_LIB_DIR)/include/alsa/output.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/error.h $(PRE_INSTALL_LIB_DIR)/include/alsa/error.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/conf.h $(PRE_INSTALL_LIB_DIR)/include/alsa/conf.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/pcm.h $(PRE_INSTALL_LIB_DIR)/include/alsa/pcm.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/rawmidi.h $(PRE_INSTALL_LIB_DIR)/include/alsa/rawmidi.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/timer.h $(PRE_INSTALL_LIB_DIR)/include/alsa/timer.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/hwdep.h $(PRE_INSTALL_LIB_DIR)/include/alsa/hwdep.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/control.h $(PRE_INSTALL_LIB_DIR)/include/alsa/control.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/mixer.h $(PRE_INSTALL_LIB_DIR)/include/alsa/mixer.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/seq_event.h $(PRE_INSTALL_LIB_DIR)/include/alsa/seq_event.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/seq.h $(PRE_INSTALL_LIB_DIR)/include/alsa/seq.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/seqmid.h $(PRE_INSTALL_LIB_DIR)/include/alsa/seqmid.h
	cp -p ./$(SUBDIR_ALSA_LIB)/include/seq_midi_event.h $(PRE_INSTALL_LIB_DIR)/include/alsa/seq_midi_event.h
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.a
	#cp -p ./$(SUBDIR_ALSA_LIB)/src/.libs/libasound.a $(RSDK_LIB_DIR)/../lib/libasound.a

	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2.0.0
	cp -p ./$(SUBDIR_ALSA_LIB)/src/.libs/libasound.so.2.0.0 $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2.0.0
	ln -s libasound.so.2.0.0 $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2
	ln -s libasound.so.2.0.0 $(PRE_INSTALL_LIB_DIR)/lib/libasound.so


romfs:
	#$(ROMFSINST) ./$(SUBDIR_ALSA_UTILS)/alsaconf/alsaconf /bin/alsaconf
	#$(ROMFSINST) ./$(SUBDIR_ALSA_UTILS)/alsactl/alsactl /bin/alsactl
	$(ROMFSINST) ./$(SUBDIR_ALSA_UTILS)/aplay/aplay /bin/aplay
	$(ROMFSINST) ./$(SUBDIR_ALSA_UTILS)/amixer/amixer /bin/amixer
	$(ROMFSINST) -s /bin/aplay /bin/arecord
	$(ROMFSINST) ./$(SUBDIR_ALSA_LIB)/src/.libs/libasound.so.2.0.0 /lib/libasound.so.2
	#$(ROMFSINST) ./audio_src_16k_le.wav /bin/audio_src_16k_le.wav
	#$(ROMFSINST) ./silence1s.wav /bin/silence1s.wav
	#$(ROMFSINST) ./stop.wav /bin/stop.wav
	#$(ROMFSINST) ./alarm.mp3 /bin/alarm.mp3
	#$(ROMFSINST) ./stop.mp3 /bin/stop.mp3
	#$(ROMFSINST) ./error.mp3 /bin/error.mp3
	#$(ROMFSINST) ./avs_no_conn.mp3 /bin/avs_no_conn.mp3

	#$(ROMFSINST) ./audio_src_16k_be.pcm /bin/audio_src_16k_be.pcm
	$(ROMFSINST) ./sequential_32bit_ST.pcm /etc/sequential_32bit_ST.pcm
	$(ROMFSINST) ./sequential_32bit_ST.pcm /etc/sequential_32bit_ST2.pcm
	$(ROMFSINST) ./test.pcm /etc/test.pcm


	mkdir -p $(ROMFSDIR)/usr/share/alsa
	mkdir -p $(ROMFSDIR)/usr/share/alsa/cards
	mkdir -p $(ROMFSDIR)/usr/share/alsa/pcm
	mkdir -p $(ROMFSDIR)/usr/share/alsa/ctl
	#cp ./$(SUBDIR_ALSA_LIB)/src/conf/asound.conf $(DIR_ROMFS)/etc/asound.conf
	#cp ./$(SUBDIR_ALSA_LIB)/src/conf/asound.conf $(DIR_ROMFS)/home/root/.asoundrc
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/alsa.conf $(ROMFSDIR)/usr/share/alsa/alsa.conf
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/cards/aliases.conf $(ROMFSDIR)/usr/share/alsa/cards/aliases.conf
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/pcm/default.conf $(ROMFSDIR)/usr/share/alsa/ctl/default.conf
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/pcm/default.conf $(ROMFSDIR)/usr/share/alsa/pcm/default.conf
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/pcm/dmix.conf $(ROMFSDIR)/usr/share/alsa/pcm/dmix.conf
	cp ./$(SUBDIR_ALSA_LIB)/src/conf/pcm/dsnoop.conf $(ROMFSDIR)/usr/share/alsa/pcm/dsnoop.conf

clean:
	make -C $(SUBDIR_ALSA_LIB) distclean
	make -C $(SUBDIR_ALSA_UTILS) distclean
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.a
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2
	rm -f $(PRE_INSTALL_LIB_DIR)/lib/libasound.so.2.0.0


