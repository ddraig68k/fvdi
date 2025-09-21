#!make -f
#
# This software is licensed under the GNU General Public License.
# Please, see LICENSE.TXT for further information.
#

TARGET = gfxvga.sys

all: $(TARGET)

top_srcdir = ../..

OPTS    = -mshort -O2 -fomit-frame-pointer -std=c99
include $(top_srcdir)/CONFIGVARS
WARNINGS = -Wall -W -Wunused -Wundef -Wstrict-prototypes -Wmissing-prototypes -Wnested-externs
CFLAGS  = $(CPUOPTS) $(OPTS) $(WARNINGS) -I$(top_srcdir)/include -I$(top_srcdir)/drivers/include

SINCSRC = \
	$(top_srcdir)/drivers/bitplane/1_expand.inc

SSRC = \
	../common/c_common.s \
	../common/common.s \
	../common/clip.s 

CSRC = \
    ../common/colours.c \
	gfxvga.c \
	init.c \
	printf.c \
	gfxvga_spec.c \
	gfxvga_scr.c \
	gfxvga_pal.c \
	gfxvga_line.c \
	gfxvga_fill.c \
	gfxvga_mouse.c \
	gfxvga_blit.c \
	gfxvga_exp.c

# handle DevPac -> gas conversions
# defines SSRC_GNU and SINCSRC_GNU variables
include $(top_srcdir)/utility/conv2gas/RULES

SINCSRC_GNU += \
	$(top_srcdir)/include/types.inc \
	$(top_srcdir)/include/macros.gnu \
	$(top_srcdir)/include/vdi.gnu \
	$(top_srcdir)/drivers/include/pixelmac.gnu

# Engine binary objects
ifeq ($(CPU),v4e)
OBJECTS = $(SSRC_GNU_CF:.s=.o)
else
OBJECTS = $(SSRC_GNU:.s=.o)
endif
OBJECTS += $(CSRC:.c=.o)

$(OBJECTS): $(SINCSRC_GNU)

$(TARGET): $(OBJECTS) $(LIBFILES)
	$(AM_V_LD)$(LD) -o $@ $(OBJECTS) $(LDFLAGS) -Map=gfxvga.map $(LIBS)

include $(top_srcdir)/DEPENDENCIES

strip:
	$(STRIP) --strip-all $(TARGET)

clean:: clean_gnu
	$(RM) $(OBJECTS) $(TARGET) gfxvga.map

install::
	mkdir -p $(DESTDIR)/gemsys
	cp -a $(TARGET) $(DESTDIR)/gemsys/$(TARGET)
	cp -a fvdi.sys "$(DESTDIR)/fvdigvga.sys"

