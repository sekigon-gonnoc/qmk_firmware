#ifndef KEYBOARDS_ANGE64_H_
#define KEYBOARDS_ANGE64_H_

#include "quantum.h"

#define LAYOUT( \
    k01, k02, k03, k04, k05, k06, k07, k08, k09, k10, k11, k12, k13, k14, \
    k15, k16, k17, k18, k19, k20, k21, k22, k23, k24, k25, k26, k27, \
    k28, k29, k30, k31, k32, k33, k34, k35, k36, k37, k38, k39, k40,   \
    k41, k42, k43, k44, k45, k46, k47, k48, k49, k50, k51, k52, k53, \
    k54, k55, k56, k57, k58, k59, k60, k61, k62, k63, k64\
) \
{ \
    { k01, k13, k25, k37, k49, k61 }, \
    { k02, k14, k26, k38, k50, k62 }, \
    { k03, k15, k27, k39, k51, k63 }, \
    { k04, k16, k28, k40, k52, k64 }, \
    { k05, k17, k29, k41, k53, KC_NO }, \
    { k06, k18, k30, k42, k54, KC_NO }, \
    { k07, k19, k31, k43, k55, KC_NO }, \
    { k08, k20, k32, k44, k56, KC_NO }, \
    { k09, k21, k33, k45, k57, KC_NO }, \
    { k10, k22, k34, k46, k58, KC_NO }, \
    { k11, k23, k35, k47, k59, KC_NO }, \
    { k12, k24, k36, k48, k60, KC_NO } \
}
#endif /* KEYBOARDS_ANGE64_H_ */
