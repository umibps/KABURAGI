CC		= gcc
CPP		= g++
CFLAGS		= `pkg-config --cflags bullet tbb gtk+-2.0 gtkglext-1.0 assimp` -O3 -w
DEST		= /usr/bin/KABURAGI
PARENT		= /home/
NAME		= $(shell whoami)
FILE_NAME	= /Desktop/KABURAGI.desktop
FILE_NAME_JA	= /デスクトップ/KABURAGI.desktop
TARGET_PATH	= $(PARENT)$(NAME)$(FILE_NAME)
TARGET_PATH_JA	= $(PARENT)$(NAME)$(FILE_NAME_JA)
LDFLAGS		= `pkg-config --libs gtk+-2.0 gtkglext-1.0 bullet tbb assimp glew` -lm -lz -lpng -lstdc++
OBJS = anti_alias.o application.o bezier.o bit_stream.o brush_core.o brushes.o cell_renderer_widget.o clip_board.o color.o common_tools.o display.o display_filter.o draw_window.o filter.o golomb_table.o history.o iccbutton.o image_read_write.o ini_file.o input.o labels.o layer.o layer_blend.o layer_set.o layer_window.o lcms_wrapper.o main.o memory_stream.o menu.o navigation.o pattern.o preference.o preview_window.o printer.o reference_window.o save.o script.o selection_area.o slide.o smoother.o spin_scale.o text_layer.o texture.o tlg.o tlg6_bit_stream.o tlg6_encode.o tool_box.o transform.o utils.o vector.o vector_brushes.o widgets.o lua/lapi.o lua/lauxlib.o lua/lbaselib.o lua/lbitlib.o lua/lcode.o lua/lcorolib.o lua/lctype.o lua/ldblib.o lua/ldebug.o lua/ldo.o lua/ldump.o lua/lfunc.o lua/lgc.o lua/linit.o lua/liolib.o lua/llex.o lua/lmathlib.o lua/lmem.o lua/loadlib.o lua/lobject.o lua/lopcodes.o lua/loslib.o lua/lparser.o lua/lstate.o lua/lstring.o lua/lstrlib.o lua/ltable.o lua/ltablib.o lua/ltm.o lua/lua.o lua/luac.o lua/lundump.o lua/lvm.o lua/lzio.o lcms/cmscam02.o lcms/cmscgats.o lcms/cmscnvrt.o lcms/cmserr.o lcms/cmsgamma.o lcms/cmsgmt.o lcms/cmshalf.o lcms/cmsintrp.o lcms/cmsio0.o lcms/cmsio1.o lcms/cmslut.o lcms/cmsmd5.o lcms/cmsmtrx.o lcms/cmsnamed.o lcms/cmsopt.o lcms/cmspack.o lcms/cmspcs.o lcms/cmsplugin.o lcms/cmsps2.o lcms/cmssamp.o lcms/cmssm.o lcms/cmstypes.o lcms/cmsvirt.o lcms/cmswtpnt.o lcms/cmsxform.o libtiff/tif_aux.o libtiff/tif_close.o libtiff/tif_codec.o libtiff/tif_color.o libtiff/tif_compress.o libtiff/tif_dir.o libtiff/tif_dirinfo.o libtiff/tif_dirread.o libtiff/tif_dirwrite.o libtiff/tif_dumpmode.o libtiff/tif_error.o libtiff/tif_extension.o libtiff/tif_fax3.o libtiff/tif_fax3sm.o libtiff/tif_flush.o libtiff/tif_getimage.o libtiff/tif_jbig.o libtiff/tif_jpeg.o libtiff/tif_jpeg_12.o libtiff/tif_luv.o libtiff/tif_lzma.o libtiff/tif_lzw.o libtiff/tif_next.o libtiff/tif_ojpeg.o libtiff/tif_open.o libtiff/tif_packbits.o libtiff/tif_pixarlog.o libtiff/tif_predict.o libtiff/tif_print.o libtiff/tif_read.o libtiff/tif_strip.o libtiff/tif_swab.o libtiff/tif_thunder.o libtiff/tif_tile.o libtiff/tif_unix.o libtiff/tif_version.o libtiff/tif_warning.o libtiff/tif_write.o libtiff/tif_zip.o libjpeg/jaricom.o libjpeg/jcapimin.o libjpeg/jcapistd.o libjpeg/jcarith.o libjpeg/jccoefct.o libjpeg/jccolor.o libjpeg/jcdctmgr.o libjpeg/jchuff.o libjpeg/jcinit.o libjpeg/jcmainct.o libjpeg/jcmarker.o libjpeg/jcmaster.o libjpeg/jcomapi.o libjpeg/jcparam.o libjpeg/jcprepct.o libjpeg/jcsample.o libjpeg/jctrans.o libjpeg/jdapimin.o libjpeg/jdapistd.o libjpeg/jdarith.o libjpeg/jdatadst.o libjpeg/jdatasrc.o libjpeg/jdcoefct.o libjpeg/jdcolor.o libjpeg/jddctmgr.o libjpeg/jdhuff.o libjpeg/jdinput.o libjpeg/jdmainct.o libjpeg/jdmarker.o libjpeg/jdmaster.o libjpeg/jdmerge.o libjpeg/jdpostct.o libjpeg/jdsample.o libjpeg/jdtrans.o libjpeg/jerror.o libjpeg/jfdctflt.o libjpeg/jfdctfst.o libjpeg/jfdctint.o libjpeg/jidctflt.o libjpeg/jidctfst.o libjpeg/jidctint.o libjpeg/jmemansi.o libjpeg/jmemmgr.o libjpeg/jquant1.o libjpeg/jquant2.o libjpeg/jutils.o MikuMikuGtk+/annotation.o MikuMikuGtk+/application.o MikuMikuGtk+/asset_model.o MikuMikuGtk+/bone.o MikuMikuGtk+/camera.o MikuMikuGtk+/control.o MikuMikuGtk+/debug_drawer.o MikuMikuGtk+/effect_engine.o MikuMikuGtk+/face.o MikuMikuGtk+/grid.o MikuMikuGtk+/hash_functions.o MikuMikuGtk+/hash_table.o MikuMikuGtk+/history.o MikuMikuGtk+/ik.o MikuMikuGtk+/joint.o MikuMikuGtk+/keyframe.o MikuMikuGtk+/light.o MikuMikuGtk+/load.o MikuMikuGtk+/load_image.o MikuMikuGtk+/material.o MikuMikuGtk+/model.o MikuMikuGtk+/model_helper.o MikuMikuGtk+/model_label.o MikuMikuGtk+/morph.o MikuMikuGtk+/motion.o MikuMikuGtk+/parameter.o MikuMikuGtk+/pmd_model.o MikuMikuGtk+/pmx_model.o MikuMikuGtk+/pose.o MikuMikuGtk+/program.o MikuMikuGtk+/project.o MikuMikuGtk+/quaternion.o MikuMikuGtk+/render_engine.o MikuMikuGtk+/rigid_body.o MikuMikuGtk+/scene.o MikuMikuGtk+/shadow_map.o MikuMikuGtk+/soft_body.o MikuMikuGtk+/system_depends.o MikuMikuGtk+/technique.o MikuMikuGtk+/text_encode.o MikuMikuGtk+/texture.o MikuMikuGtk+/texture_draw_helper.o MikuMikuGtk+/ui.o MikuMikuGtk+/ui_label.o MikuMikuGtk+/utils.o MikuMikuGtk+/vertex.o MikuMikuGtk+/vmd_keyframe.o MikuMikuGtk+/vmd_motion.o MikuMikuGtk+/world.o MikuMikuGtk+/libguess/guess.o MikuMikuGtk+/bullet.o MikuMikuGtk+/tbb.o
TARGET	= KABURAGI

.SUFFIXES: .cpp .o

.cpp.o:
	$(CPP) $(CFLAGS) -c $< -o $*.o

all:		$(TARGET)

$(TARGET):	$(OBJS)
		$(CC) $(OBJS) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
		rm -f *.o *~ $(TARGET)

install:	$(TARGET)
		mkdir -p $(DEST)
		cp -p ./$(TARGET) $(DEST)/$(TARGET)
		cp -p ./application.ini $(DEST)/application.ini
		cp -p ./brushes.ini $(DEST)/brushes.ini
		cp -p ./vector_brushes.ini $(DEST)/vector_brushes.ini
		cp -p ./common_tools.ini $(DEST)/common_tools.ini
		cp -r ./lang $(DEST)/lang
		cp -r ./brushes $(DEST)/brushes
		cp -r ./etc $(DEST)/etc
		cp -r ./image $(DEST)/image
		cp -r ./lib $(DEST)/lib
		cp -r ./pattern $(DEST)/pattern
		cp -r ./script $(DEST)/script
		cp -r ./share $(DEST)/share
		cp -r ./stamp $(DEST)/stamp
		cp -r ./texture $(DEST)/texture
		cp -p ./application.desktop $(DEST)/KABURAGI.desktop
		chmod +x $(DEST)/KABURAGI.desktop

shortcut:	$(TARGET)
		-cp -p ./application.desktop $(TARGET_PATH)
		-chmod +x $(TARGET_PATH)
		-cp -p ./application.desktop $(TARGET_PATH_JA)
		-chmod +x $(TARGET_PATH_JA)

