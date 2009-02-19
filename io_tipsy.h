#ifndef IO_TIPSY_H
#define IO_TIPSY_H

#ifdef __cplusplus
extern "C" {
#endif

int load_standard_tipsy(char *fname);
int load_native_tipsy(char *fname);
int store_standard_tipsy(char *fname);
int store_native_tipsy(char *fname);

#ifdef __cplusplus
}
#endif

#endif

