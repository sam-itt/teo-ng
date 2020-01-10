#ifndef AFRONT_H
#define AFRONT_H

int afront_Init(const char *w_title, unsigned char j_support, 
                const char *alconfig_file, const char *keymap_file);
int afront_startGfx(int gfx_mode, int *windowed_mode, char *version_name);
void afront_Run(void);
void afront_Shutdown(void);

#endif //AFRONT_H
