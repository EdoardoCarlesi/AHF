extern int ThisTask;
extern int NTask;

extern struct Cpu
{
  char name[4];

} *cpu;

extern int *SizeDispl;
extern int *SizeHaloes;
extern int *SizeDisplStructHalo;
extern int *SizeHaloesStructHalo;

void init_cpu_struct(void);
void generate_url_for_tasks(void);
void gather_halo_structures(void);
void init_comm_structures(void);
void free_comm_structures(void);
void copy_halo_url(char*);
