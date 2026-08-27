/* Test preprocessor boolean expressions */

#if defined(__SASC) || defined(__MAXON__)
const char* compiler = "SAS/C or MAXON";
#else
const char* compiler = "other";
#endif

#if defined(__SASC) && defined(__AMIGA__)
const char* sasc_amiga = "SAS/C on Amiga";
#else
const char* sasc_amiga = "not SAS/C on Amiga";
#endif

int main() {
    return 0;
}
