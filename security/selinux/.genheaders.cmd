savedcmd_security/selinux/genheaders := gcc -Wp,-MMD,security/selinux/.genheaders.d -Wall -Wmissing-prototypes -Wstrict-prototypes -O2 -fomit-frame-pointer -std=gnu11   -I ./scripts/include -I./security/selinux/include    -o security/selinux/genheaders security/selinux/genheaders.c   

source_security/selinux/genheaders := security/selinux/genheaders.c

deps_security/selinux/genheaders := \
  security/selinux/include/classmap.h \
  security/selinux/include/initial_sid_to_string.h \

security/selinux/genheaders: $(deps_security/selinux/genheaders)

$(deps_security/selinux/genheaders):
