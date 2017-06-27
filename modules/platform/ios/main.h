#ifndef main_h
#define main_h

typedef void (*MainThreadFunc)(void *);
void performInMainThread(MainThreadFunc func, void *param);
void getScreenSize(int *width, int *height);

#endif
