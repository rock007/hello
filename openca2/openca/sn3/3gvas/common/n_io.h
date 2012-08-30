
#ifndef N_IO_HEAD_DEFINE_
#define N_IO_HEAD_DEFINE_

extern int n_file_copy(const char* s_srcfname, const char* s_dstfname);
extern int n_file_move(const char* s_srcfname, const char* s_dstfname);
extern int n_file_del(const char* s_fname);

extern int n_dir_create(const char* s_path);
typedef int (* func_ndir_findfile_callback)(const char* s_fname, void* p_param);
extern int n_dir_findfile(const char* s_path, func_ndir_findfile_callback p_func, void* p_param);

#ifdef WIN32
inline int linux_system_call(const char* s_data) { return 0;}
#else
extern int linux_system_call(const char* s_data);
#endif
/*
int fun_findfile(const char* s_fname, void* p_param)
{
printf("filename is %s\n", s_fname);
return 0;
}

int main()
{
n_dir_create("c:/nzq_working/dircreate");
n_file_copy("c:/nzq_working/db.log", "c:/nzq_working/dircreate/db.log");
n_file_del("c:/nzq_working/dircreate/db.log");
n_file_copy("c:/nzq_working/db.log", "c:/nzq_working/dircreate/db.log");
n_dir_findfile("c:/nzq_working/dircreate/", fun_findfile, NULL);
n_file_del("c:/nzq_working/dircreate/db.log");
}

*/
#endif
