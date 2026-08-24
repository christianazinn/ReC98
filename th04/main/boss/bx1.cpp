extern unsigned char mugetsu_phase2_mode;

void pascal near mugetsu_phase2_next(
	explosion_type_t explosion_type, int next_end_hp
);

// The eleven of Mugetsu's fourteen procs that are called across one of the
// three object boundaries MUGETSU_TEXT's C++ half is split at, and are
// therefore not `static` the way their neighbours in the same file are. The
// split itself is documented in th04/main/boss/bx1_gath.cpp; in short, the two
// pose drivers must be compiled without th04/main/bullet/bullet.hpp in the
// translation unit, and the `-a2` table parities of mugetsu_1812A(),
// mugetsu_1821E() and mugetsu_update() then decide where the other two
// boundaries go. Every one of these calls is near, inside MUGETSU_TEXT and
// inside `main_03`, so publishing them changed no call site's length.
//
// th04/main/boss/bx1_gath.cpp:
void near mugetsu_18044(void);
// th04/main/boss/bx1_pose.cpp:
unsigned char near mugetsu_180BB(void);
unsigned char near mugetsu_1812A(void);
unsigned char near mugetsu_1821E(void);
// th04/main/boss/bx1_ptn.cpp:
void near mugetsu_18314(void);
void near mugetsu_1838A(void);
void near mugetsu_1845E(void);
void near mugetsu_184AC(void);
void near mugetsu_18556(void);
void near mugetsu_185E4(void);
void near mugetsu_18655(void);
bool near mugetsu_186B9(void);
