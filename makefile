CC	= gcc
CPP	= g++
CFLAGS	= `pkg-config --cflags gtk+-2.0` -O3 -w
DEST	= /usr/bin/KABURAGI
LDFLAGS	= `pkg-config --libs gtk+-2.0` -lm -lz -lpng
OBJS = anti_alias.o application.o bezier.o brush_core.o brushes.o cell_renderer_widget.o clip_board.o color.o common_tools.o display.o display_filter.o draw_window.o filter.o golomb_table.o history.o iccbutton.o image_read_write.o ini_file.o input.o labels.o layer.o layer_blend.o layer_set.o layer_window.o lcms_wrapper.o main.o memory_stream.o menu.o navigation.o pattern.o preference.o preview_window.o printer.o reference_window.o save.o script.o selection_area.o slide.o smoother.o spin_scale.o text_layer.o texture.o tlg.o tlg6_bit_stream.o tlg6_encode.o tool_box.o transform.o utils.o vector.o vector_brushes.o widgets.o lua/lapi.o lua/lauxlib.o lua/lbaselib.o lua/lbitlib.o lua/lcode.o lua/lcorolib.o lua/lctype.o lua/ldblib.o lua/ldebug.o lua/ldo.o lua/ldump.o lua/lfunc.o lua/lgc.o lua/linit.o lua/liolib.o lua/llex.o lua/lmathlib.o lua/lmem.o lua/loadlib.o lua/lobject.o lua/lopcodes.o lua/loslib.o lua/lparser.o lua/lstate.o lua/lstring.o lua/lstrlib.o lua/ltable.o lua/ltablib.o lua/ltm.o lua/lua.o lua/luac.o lua/lundump.o lua/lvm.o lua/lzio.o lcms/cmscam02.o lcms/cmscgats.o lcms/cmscnvrt.o lcms/cmserr.o lcms/cmsgamma.o lcms/cmsgmt.o lcms/cmshalf.o lcms/cmsintrp.o lcms/cmsio0.o lcms/cmsio1.o lcms/cmslut.o lcms/cmsmd5.o lcms/cmsmtrx.o lcms/cmsnamed.o lcms/cmsopt.o lcms/cmspack.o lcms/cmspcs.o lcms/cmsplugin.o lcms/cmsps2.o lcms/cmssamp.o lcms/cmssm.o lcms/cmstypes.o lcms/cmsvirt.o lcms/cmswtpnt.o lcms/cmsxform.o libtiff/tif_aux.o libtiff/tif_close.o libtiff/tif_codec.o libtiff/tif_color.o libtiff/tif_compress.o libtiff/tif_dir.o libtiff/tif_dirinfo.o libtiff/tif_dirread.o libtiff/tif_dirwrite.o libtiff/tif_dumpmode.o libtiff/tif_error.o libtiff/tif_extension.o libtiff/tif_fax3.o libtiff/tif_fax3sm.o libtiff/tif_flush.o libtiff/tif_getimage.o libtiff/tif_jbig.o libtiff/tif_jpeg.o libtiff/tif_jpeg_12.o libtiff/tif_luv.o libtiff/tif_lzma.o libtiff/tif_lzw.o libtiff/tif_next.o libtiff/tif_ojpeg.o libtiff/tif_open.o libtiff/tif_packbits.o libtiff/tif_pixarlog.o libtiff/tif_predict.o libtiff/tif_print.o libtiff/tif_read.o libtiff/tif_strip.o libtiff/tif_swab.o libtiff/tif_thunder.o libtiff/tif_tile.o libtiff/tif_unix.o libtiff/tif_version.o libtiff/tif_warning.o libtiff/tif_write.o libtiff/tif_zip.o libjpeg/jaricom.o libjpeg/jcapimin.o libjpeg/jcapistd.o libjpeg/jcarith.o libjpeg/jccoefct.o libjpeg/jccolor.o libjpeg/jcdctmgr.o libjpeg/jchuff.o libjpeg/jcinit.o libjpeg/jcmainct.o libjpeg/jcmarker.o libjpeg/jcmaster.o libjpeg/jcomapi.o libjpeg/jcparam.o libjpeg/jcprepct.o libjpeg/jcsample.o libjpeg/jctrans.o libjpeg/jdapimin.o libjpeg/jdapistd.o libjpeg/jdarith.o libjpeg/jdatadst.o libjpeg/jdatasrc.o libjpeg/jdcoefct.o libjpeg/jdcolor.o libjpeg/jddctmgr.o libjpeg/jdhuff.o libjpeg/jdinput.o libjpeg/jdmainct.o libjpeg/jdmarker.o libjpeg/jdmaster.o libjpeg/jdmerge.o libjpeg/jdpostct.o libjpeg/jdsample.o libjpeg/jdtrans.o libjpeg/jerror.o libjpeg/jfdctflt.o libjpeg/jfdctfst.o libjpeg/jfdctint.o libjpeg/jidctflt.o libjpeg/jidctfst.o libjpeg/jidctint.o libjpeg/jmemansi.o libjpeg/jmemmgr.o libjpeg/jquant1.o libjpeg/jquant2.o libjpeg/jutils.o MikuMikuGTK+/annotation.o MikuMikuGTK+/application.o MikuMikuGTK+/asset_model.o MikuMikuGTK+/bone.o MikuMikuGTK+/camera.o MikuMikuGTK+/control.o MikuMikuGTK+/debug_drawer.o MikuMikuGTK+/effect_engine.o MikuMikuGTK+/face.o MikuMikuGTK+/grid.o MikuMikuGTK+/hash_functions.o MikuMikuGTK+/hash_table.o MikuMikuGTK+/history.o MikuMikuGTK+/ik.o MikuMikuGTK+/joint.o MikuMikuGTK+/keyframe.o MikuMikuGTK+/light.o MikuMikuGTK+/load.o MikuMikuGTK+/load_image.o MikuMikuGTK+/material.o MikuMikuGTK+/model.o MikuMikuGTK+/model_helper.o MikuMikuGTK+/model_label.o MikuMikuGTK+/morph.o MikuMikuGTK+/motion.o MikuMikuGTK+/parameter.o MikuMikuGTK+/pmd_model.o MikuMikuGTK+/pmx_model.o MikuMikuGTK+/pose.o MikuMikuGTK+/program.o MikuMikuGTK+/project.o MikuMikuGTK+/quaternion.o MikuMikuGTK+/render_engine.o MikuMikuGTK+/rigid_body.o MikuMikuGTK+/scene.o MikuMikuGTK+/shadow_map.o MikuMikuGTK+/soft_body.o MikuMikuGTK+/system_depends.o MikuMikuGTK+/technique.o MikuMikuGTK+/text_encode.o MikuMikuGTK+/texture.o MikuMikuGTK+/texture_draw_helper.o MikuMikuGTK+/ui.o MikuMikuGTK+/ui_label.o MikuMikuGTK+/vertex.o MikuMikuGTK+/vmd_keyframe.o MikuMikuGTK+/vmd_motion.o MikuMikuGTK+/world.o MikuMikuGTK+/libguess/guess.o
CPP_OBJS = MikuMikuGTK+/bullet.o MikuMikuGTK+/tbb.o
ASM = nasm-2.07/nasm
ASM_OBJS = colorfill.o cpuid.o make_alpha_from_key.o tlg5.o tlg6_chroma.o tlg6_golomb.o

TARGET	= KABURAGI

.SUFFIXES: .nas .o

.nas.o:
	$(ASM) -f elf64 -o $<

.cpp.o:
	$(CPP) $(CFLAGS) -o 

all:		$(TARGET)

$(TARGET):	$(OBJS)
		tar zxvf nasm-2.07.tar.gz
		cd nasm-2.07
		./configure
		make
		cd ../
		$(ASM_OBJS)
		$(CC) $(OBJS) $(ASM_OBJS) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
		rm -f *.o *~ $(TARGET)
		rm -r nasm-2.07

install:	$(TARGET)
		mkdir -p $(DEST)
		cp -p ./$(TARGET) $(DEST)/$(TARGET)
		cp -p ./application.ini $(DEST)/application.ini
		cp -p ./brushes.ini $(DEST)/brushes.ini
		cp -p ./vector_brushes.ini $(DEST)/vector_brushes.ini
		cp -p ./common_tools.ini $(DEST)/common_tools.ini
		cp -p ./japanese.lang $(DEST)/japanese.lang
		cp -r ./brushes $(DEST)/brushes
		cp -r ./etc $(DEST)/etc
		cp -r ./image $(DEST)/image
		cp -r ./lib $(DEST)/lib
		cp -r ./pattern $(DEST)/pattern
		cp -r ./script $(DEST)/script
		cp -r ./share $(DEST)/share
		cp -r ./stamp $(DEST)/stamp
		cp -r ./texture $(DEST)/texture
