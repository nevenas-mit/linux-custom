savedcmd_lib/gen_crc32table := gcc -Wp,-MMD,lib/.gen_crc32table.d -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu11   -I ./scripts/include     -o lib/gen_crc32table lib/gen_crc32table.c   

source_lib/gen_crc32table := lib/gen_crc32table.c

deps_lib/gen_crc32table := \
  lib/../include/linux/crc32poly.h \

lib/gen_crc32table: $(deps_lib/gen_crc32table)

$(deps_lib/gen_crc32table):
