

void sdcard_init();
void sdcard_cleanup();
void serve_file_from_sdcard(struct netconn *newconn, char *path);
void sdcard_test_file();
