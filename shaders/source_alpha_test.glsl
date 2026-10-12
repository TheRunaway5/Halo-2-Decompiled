#ifndef H2_SOURCE_ALPHA_TEST_GLSL
#define H2_SOURCE_ALPHA_TEST_GLSL
// Source byte-alpha comparison. Keep rounding before conversion/comparison;
// alpha itself and the eventual color output remain unchanged.
bool sourceAlphaTestPass(float alpha,uint comparison,uint reference) {
    int value=int(round(alpha*255.0));
    int threshold=int(reference);
    if(comparison==0x200u)return false;
    if(comparison==0x201u)return value<threshold;
    if(comparison==0x202u)return value==threshold;
    if(comparison==0x203u)return value<=threshold;
    if(comparison==0x204u)return value>threshold;
    if(comparison==0x205u)return value!=threshold;
    if(comparison==0x206u)return value>=threshold;
    return comparison==0x207u;
}
#endif
