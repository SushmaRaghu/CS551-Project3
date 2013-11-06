/* Miscellaneous system calls.				Author: Kees J. Bot
 *								31 Mar 2000
 * The entry points into this file are:
 *   do_reboot: kill all processes, then reboot system
 *   do_getsysinfo: request copy of PM data structure  (Jorrit N. Herder)
 *   do_getprocnr: lookup process slot number  (Jorrit N. Herder)
 *   do_getepinfo: get the pid/uid/gid of a process given its endpoint
 *   do_getsetpriority: get/set process priority
 *   do_svrctl: process manager control
 */

#define brk _brk

#include "pm.h"
#include <minix/callnr.h>
#include <signal.h>
#include <sys/svrctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <minix/com.h>
#include <minix/config.h>
#include <minix/sysinfo.h>
#include <minix/type.h>
#include <minix/vm.h>
#include <string.h>
#include <machine/archtypes.h>
#include <lib.h>
#include <assert.h>
#include "mproc.h"
#include "param.h"
#include "kernel/proc.h"
#include <stdlib.h>

struct utsname uts_val = {
    "Minix",		/* system name */
    "noname",		/* node/network name */
    OS_RELEASE,		/* O.S. release (e.g. 1.5) */
    OS_VERSION,		/* O.S. version (e.g. 10) */
    "xyzzy",		/* machine (cpu) type (filled in later) */
#if defined(__i386__)
    "i386",		/* architecture */
#elif defined(__arm__)
    "arm",		/* architecture */
#else
#error			/* oops, no 'uname -mk' */
#endif
};

static char *uts_tbl[] = {
    uts_val.arch,
    NULL,			/* No kernel architecture */
    uts_val.machine,
    NULL,			/* No hostname */
    uts_val.nodename,
    uts_val.release,
    uts_val.version,
    uts_val.sysname,
    NULL,			/* No bus */			/* No bus */
};

#if ENABLE_SYSCALL_STATS
unsigned long calls_stats[NCALLS];
#endif

/*===========================================================================*
 *				do_sysuname				     *
 *===========================================================================*/
int do_sysuname()
{
    /* Set or get uname strings. */
    
    int r;
    size_t n;
    char *string;
#if 0 /* for updates */
    char tmp[sizeof(uts_val.nodename)];
    static short sizes[] = {
        0,	/* arch, (0 = read-only) */
        0,	/* kernel */
        0,	/* machine */
        0,	/* sizeof(uts_val.hostname), */
        sizeof(uts_val.nodename),
        0,	/* release */
        0,	/* version */
        0,	/* sysname */
    };
#endif
    
    if ((unsigned) m_in.sysuname_field >= _UTS_MAX) return(EINVAL);
    
    string = uts_tbl[m_in.sysuname_field];
    if (string == NULL)
        return EINVAL;	/* Unsupported field */
    
    switch (m_in.sysuname_req) {
        case _UTS_GET:
            /* Copy an uname string to the user. */
            n = strlen(string) + 1;
            if (n > m_in.sysuname_len) n = m_in.sysuname_len;
            r = sys_vircopy(SELF, (phys_bytes) string,
                            mp->mp_endpoint, (phys_bytes) m_in.sysuname_value,
                            (phys_bytes) n);
            if (r < 0) return(r);
            break;
            
#if 0	/* no updates yet */
        case _UTS_SET:
            /* Set an uname string, needs root power. */
            len = sizes[m_in.sysuname_field];
            if (mp->mp_effuid != 0 || len == 0) return(EPERM);
            n = len < m_in.sysuname_len ? len : m_in.sysuname_len;
            if (n <= 0) return(EINVAL);
            r = sys_vircopy(mp->mp_endpoint, (phys_bytes) m_in.sysuname_value,
                            SELF, (phys_bytes) tmp, (phys_bytes) n);
            if (r < 0) return(r);
            tmp[n-1] = 0;
            strcpy(string, tmp);
            break;
#endif
            
        default:
            return(EINVAL);
    }
    /* Return the number of bytes moved. */
    return(n);
}


/*===========================================================================*
 *				do_getsysinfo			       	     *
 *===========================================================================*/
int do_getsysinfo()
{
    vir_bytes src_addr, dst_addr;
    size_t len;
    
    /* This call leaks important information. In the future, requests from
     * non-system processes should be denied.
     */
    if (mp->mp_effuid != 0)
    {
        printf("PM: unauthorized call of do_getsysinfo by proc %d '%s'\n",
               mp->mp_endpoint, mp->mp_name);
        sys_sysctl_stacktrace(mp->mp_endpoint);
        return EPERM;
    }
    
    switch(m_in.SI_WHAT) {
        case SI_PROC_TAB:			/* copy entire process table */
            src_addr = (vir_bytes) mproc;
            len = sizeof(struct mproc) * NR_PROCS;
            break;
#if ENABLE_SYSCALL_STATS
        case SI_CALL_STATS:
            src_addr = (vir_bytes) calls_stats;
            len = sizeof(calls_stats);
            break;
#endif
        default:
            return(EINVAL);
    }
    
    if (len != m_in.SI_SIZE)
        return(EINVAL);
    
    dst_addr = (vir_bytes) m_in.SI_WHERE;
    return sys_datacopy(SELF, src_addr, who_e, dst_addr, len);
}

/*===========================================================================*
 *				do_getprocnr			             *
 *===========================================================================*/
int do_getprocnr()
{
    register struct mproc *rmp;
    static char search_key[PROC_NAME_LEN+1];
    int key_len;
    int s;
    
    /* This call should be moved to DS. */
    if (mp->mp_effuid != 0)
    {
        /* For now, allow non-root processes to request their own endpoint. */
        if (m_in.pid < 0 && m_in.namelen == 0) {
            mp->mp_reply.PM_ENDPT = who_e;
            mp->mp_reply.PM_PENDPT = NONE;
            return OK;
        }
        
        printf("PM: unauthorized call of do_getprocnr by proc %d\n",
               mp->mp_endpoint);
        sys_sysctl_stacktrace(mp->mp_endpoint);
        return EPERM;
    }
    
#if 0
    printf("PM: do_getprocnr(%d) call from endpoint %d, %s\n",
           m_in.pid, mp->mp_endpoint, mp->mp_name);
#endif
    
    if (m_in.pid >= 0) {			/* lookup process by pid */
        if ((rmp = find_proc(m_in.pid)) != NULL) {
            mp->mp_reply.PM_ENDPT = rmp->mp_endpoint;
#if 0
            printf("PM: pid result: %d\n", rmp->mp_endpoint);
#endif
            return(OK);
        }
        return(ESRCH);
    } else if (m_in.namelen > 0) {	/* lookup process by name */
        key_len = MIN(m_in.namelen, PROC_NAME_LEN);
        if (OK != (s=sys_datacopy(who_e, (vir_bytes) m_in.PMBRK_ADDR,
                                  SELF, (vir_bytes) search_key, key_len)))
            return(s);
        search_key[key_len] = '\0';	/* terminate for safety */
        for (rmp = &mproc[0]; rmp < &mproc[NR_PROCS]; rmp++) {
            if (((rmp->mp_flags & (IN_USE | EXITING)) == IN_USE) &&
                strncmp(rmp->mp_name, search_key, key_len)==0) {
                mp->mp_reply.PM_ENDPT = rmp->mp_endpoint;
                return(OK);
            }
        }
        return(ESRCH);
    } else {			/* return own/parent process number */
#if 0
        printf("PM: endpt result: %d\n", mp->mp_reply.PM_ENDPT);
#endif
        mp->mp_reply.PM_ENDPT = who_e;
        mp->mp_reply.PM_PENDPT = mproc[mp->mp_parent].mp_endpoint;
    }
    
    return(OK);
}

/*===========================================================================*
 *				do_getepinfo			             *
 *===========================================================================*/
int do_getepinfo()
{
    register struct mproc *rmp;
    endpoint_t ep;
    
    ep = m_in.PM_ENDPT;
    
    for (rmp = &mproc[0]; rmp < &mproc[NR_PROCS]; rmp++) {
        if ((rmp->mp_flags & IN_USE) && (rmp->mp_endpoint == ep)) {
            mp->mp_reply.reply_res2 = rmp->mp_effuid;
            mp->mp_reply.reply_res3 = rmp->mp_effgid;
            return(rmp->mp_pid);
        }
    }
    
    /* Process not found */
    return(ESRCH);
}

/*===========================================================================*
 *				do_getepinfo_o			             *
 *===========================================================================*/
int do_getepinfo_o()
{
    register struct mproc *rmp;
    endpoint_t ep;
    
    /* This call should be moved to DS. */
    if (mp->mp_effuid != 0) {
        printf("PM: unauthorized call of do_getepinfo_o by proc %d\n",
               mp->mp_endpoint);
        sys_sysctl_stacktrace(mp->mp_endpoint);
        return EPERM;
    }
    
    ep = m_in.PM_ENDPT;
    
    for (rmp = &mproc[0]; rmp < &mproc[NR_PROCS]; rmp++) {
        if ((rmp->mp_flags & IN_USE) && (rmp->mp_endpoint == ep)) {
            mp->mp_reply.reply_res2 = (short) rmp->mp_effuid;
            mp->mp_reply.reply_res3 = (char) rmp->mp_effgid;
            return(rmp->mp_pid);
        }
    }
    
    /* Process not found */
    return(ESRCH);
}

/*===========================================================================*
 *				do_reboot				     *
 *===========================================================================*/
int do_reboot()
{
    message m;
    
    /* Check permission to abort the system. */
    if (mp->mp_effuid != SUPER_USER) return(EPERM);
    
    /* See how the system should be aborted. */
    abort_flag = (unsigned) m_in.reboot_flag;
    if (abort_flag >= RBT_INVALID) return(EINVAL);
    
    /* Order matters here. When VFS is told to reboot, it exits all its
     * processes, and then would be confused if they're exited again by
     * SIGKILL. So first kill, then reboot.
     */
    
    check_sig(-1, SIGKILL, FALSE /* ksig*/); /* kill all users except init */
    sys_stop(INIT_PROC_NR);		   /* stop init, but keep it around */
    
    /* Tell VFS to reboot */
    m.m_type = PM_REBOOT;
    
    tell_vfs(&mproc[VFS_PROC_NR], &m);
    
    return(SUSPEND);			/* don't reply to caller */
}

/*===========================================================================*
 *				do_getsetpriority			     *
 *===========================================================================*/
int do_getsetpriority()
{
	int r, arg_which, arg_who, arg_pri;
	struct mproc *rmp;
    
	arg_which = m_in.m1_i1;
	arg_who = m_in.m1_i2;
	arg_pri = m_in.m1_i3;	/* for SETPRIORITY */
    
	/* Code common to GETPRIORITY and SETPRIORITY. */
    
	/* Only support PRIO_PROCESS for now. */
	if (arg_which != PRIO_PROCESS)
		return(EINVAL);
    
	if (arg_who == 0)
		rmp = mp;
	else
		if ((rmp = find_proc(arg_who)) == NULL)
			return(ESRCH);
    
	if (mp->mp_effuid != SUPER_USER &&
        mp->mp_effuid != rmp->mp_effuid && mp->mp_effuid != rmp->mp_realuid)
		return EPERM;
    
	/* If GET, that's it. */
	if (call_nr == GETPRIORITY) {
		return(rmp->mp_nice - PRIO_MIN);
	}
    
	/* Only root is allowed to reduce the nice level. */
	if (rmp->mp_nice > arg_pri && mp->mp_effuid != SUPER_USER)
		return(EACCES);
	
	/* We're SET, and it's allowed.
	 *
	 * The value passed in is currently between PRIO_MIN and PRIO_MAX.
	 * We have to scale this between MIN_USER_Q and MAX_USER_Q to match
	 * the kernel's scheduling queues.
	 */
    
	if ((r = sched_nice(rmp, arg_pri)) != OK) {
		return r;
	}
    
	rmp->mp_nice = arg_pri;
	return(OK);
}

/*===========================================================================*
 *				do_svrctl				     *
 *===========================================================================*/
int do_svrctl()
{
    int s, req;
    vir_bytes ptr;
#define MAX_LOCAL_PARAMS 2
    static struct {
        char name[30];
        char value[30];
    } local_param_overrides[MAX_LOCAL_PARAMS];
    static int local_params = 0;
    
    req = m_in.svrctl_req;
    ptr = (vir_bytes) m_in.svrctl_argp;
    
    /* Is the request indeed for the PM? */
    if (((req >> 8) & 0xFF) != 'M') return(EINVAL);
    
    /* Control operations local to the PM. */
    switch(req) {
        case PMSETPARAM:
        case PMGETPARAM: {
            struct sysgetenv sysgetenv;
            char search_key[64];
            char *val_start;
            size_t val_len;
            size_t copy_len;
            
            /* Copy sysgetenv structure to PM. */
            if (sys_datacopy(who_e, ptr, SELF, (vir_bytes) &sysgetenv,
                             sizeof(sysgetenv)) != OK) return(EFAULT);
            
            /* Set a param override? */
            if (req == PMSETPARAM) {
                if (local_params >= MAX_LOCAL_PARAMS) return ENOSPC;
                if (sysgetenv.keylen <= 0
                    || sysgetenv.keylen >=
                    sizeof(local_param_overrides[local_params].name)
                    || sysgetenv.vallen <= 0
                    || sysgetenv.vallen >=
                    sizeof(local_param_overrides[local_params].value))
                    return EINVAL;
                
                if ((s = sys_datacopy(who_e, (vir_bytes) sysgetenv.key,
                                      SELF, (vir_bytes) local_param_overrides[local_params].name,
                                      sysgetenv.keylen)) != OK)
                    return s;
                if ((s = sys_datacopy(who_e, (vir_bytes) sysgetenv.val,
                                      SELF, (vir_bytes) local_param_overrides[local_params].value,
                                      sysgetenv.vallen)) != OK)
                    return s;
                local_param_overrides[local_params].name[sysgetenv.keylen] = '\0';
                local_param_overrides[local_params].value[sysgetenv.vallen] = '\0';
                
                local_params++;
                
                return OK;
            }
            
            if (sysgetenv.keylen == 0) {	/* copy all parameters */
                val_start = monitor_params;
                val_len = sizeof(monitor_params);
            }
            else {				/* lookup value for key */
                int p;
                /* Try to get a copy of the requested key. */
                if (sysgetenv.keylen > sizeof(search_key)) return(EINVAL);
                if ((s = sys_datacopy(who_e, (vir_bytes) sysgetenv.key,
                                      SELF, (vir_bytes) search_key, sysgetenv.keylen)) != OK)
                    return(s);
                
                /* Make sure key is null-terminated and lookup value.
                 * First check local overrides.
                 */
                search_key[sysgetenv.keylen-1]= '\0';
                for(p = 0; p < local_params; p++) {
                    if (!strcmp(search_key, local_param_overrides[p].name)) {
                        val_start = local_param_overrides[p].value;
                        break;
                    }
                }
                if (p >= local_params && (val_start = find_param(search_key)) == NULL)
                    return(ESRCH);
                val_len = strlen(val_start) + 1;
            }
            
            /* See if it fits in the client's buffer. */
            if (val_len > sysgetenv.vallen)
                return E2BIG;
            
            /* Value found, make the actual copy (as far as possible). */
            copy_len = MIN(val_len, sysgetenv.vallen);
            if ((s=sys_datacopy(SELF, (vir_bytes) val_start,
                                who_e, (vir_bytes) sysgetenv.val, copy_len)) != OK)
                return(s);
            
            return OK;
        }
            
        default:
            return(EINVAL);
    }
}

/*===========================================================================*
 *				_brk				             *
 *===========================================================================*/

extern char *_brksize;
int brk(brk_addr)
#ifdef __NBSD_LIBC
void *brk_addr;
#else
char *brk_addr;
#endif
{
	int r;
    /* PM wants to call brk() itself. */
	if((r=vm_brk(PM_PROC_NR, brk_addr)) != OK) {
#if 0
		printf("PM: own brk(%p) failed: vm_brk() returned %d\n",
               brk_addr, r);
#endif
		return -1;
	}
	_brksize = brk_addr;
	return 0;
}

/*===========================================================================*
 *				Project 2 				             *
 *===========================================================================*/


#define MAX_GROUPS 5
#define LENGTH 100
#define MAX_PUBLISHERS 3
#define MAX_SUBSCRIBERS 5
#define QUEUE_FULL 5
#define FALSE 0
#define TRUE 1
#define SECURED_GROUP 1
#define PUBLIC_GROUP 2
#define MAIN_MEMBER_ID 0
#define MAX_NO_PRIVILEGED_USERS 5
#define MAX_NO_GROUP_MEMBERS 5


/*===========================================================================*
 *				Project 3				             *
 *===========================================================================*/
struct member_list 
{
    int mid;
    struct member_list *next;
};
    
typedef struct member_list* MEMBER_LIST;

/*________________________*/
struct messages_queue
{
    
    char message[100];
    int count;
    struct messages_queue * next;
    
};

typedef struct messages_queue * MSG_BUF;

struct subscribers
{
    int subscriber_pid;
    MSG_BUF sub_read_messages;
};


struct publishers
{
    int publisher_pid;
};

struct processes_group
{
    
	/*  PROJ 3 */    
    int group_type;  
    int group_creator;     
    int no_of_members;    
    MEMBER_LIST group_ACL;  
	
	
	int group_id;
    int no_of_subscribers;
    int no_of_publishers;
    int no_of_messages;
	char *group_name;
    struct subscribers participants[5];
    struct publishers leaders[MAX_PUBLISHERS];
    MSG_BUF group_message;
    struct processes_group *next;
};

typedef struct processes_group* PROCESSES;

static PROCESSES newgroups=NULL;

/*Project 3 */
static MEMBER_LIST priveleged_ACL = NULL;
static int no_of_priveleged_users;
/*___________*/

static int no_of_groups;
static int group_id_allocate;
static int areAllParamsSet = FALSE;
 
 
 void printACL()
{

    MEMBER_LIST member, temp;
    temp = NULL;
    printf(" Print privileged user list\n");    
     

    member = priveleged_ACL;
    if (member == NULL){
            printf(" List is empty\n" ); 
			}
    else
    {
        
        while(member!=NULL)
        {
            printf("Member ID = %d \n",member->mid);
            member = member->next;
        }
    }
    

}




int checkIfPublisherIsUnblocked()
{
    
	 BLOCKED_PROCESSES member, temp;
    temp = NULL;
    
	member = blockedProcesses;
    
	while(member!=NULL)
    {
		if(member->unblocked == TRUE)
		{
            return TRUE;
		}
        member = member->next;
	} 
    return FALSE;
}


void resetParams()
{
	no_of_groups =0;
    group_id_allocate =0;
	blockedProcesses = NULL ;
    areAllParamsSet = TRUE;
    newgroups = NULL;
	/*  PROJ 3 */    
    priveleged_ACL = NULL;
    no_of_priveleged_users = 0;
    
}

void printProcessesInBlockedState()
{
    
	 BLOCKED_PROCESSES member, temp;
    temp = NULL;
    printf(" PRINT BLOCK LIST\n");
    
    
	member = blockedProcesses;
    if (member == NULL)
        printf("No one is blcked\n");
    else
    {
        
    	while(member!=NULL)
        {
            printf("publisher pid = %d \n",member->publisher_pid);
            printf("group  id = %d \n",member->group_id);
            printf("publisher = %d \n",member->publisher);
            printf("message = %d \n",member->store);
            printf("unblocked status = %d \n",member->unblocked);
            printf("message = %s \n",member->msg);
            member = member->next;
    	}
    }
      
    
}


void unblockPublisherFromBlockList(int publisher_grpid)
{
    
	  BLOCKED_PROCESSES member, temp;
    member = temp = NULL;
    
    
    
	member = blockedProcesses;
	while(member!=NULL)
    {
		if(member->group_id == publisher_grpid)
		{
            
            member->unblocked = TRUE;
		}
        member = member->next;
	}
    
    printProcessesInBlockedState();  
}

/* Project 3*/
int isMemberOfProvelegedACL(MEMBER_LIST acl,int member_mid)
{
    MEMBER_LIST member =acl;
     printf("\n Entered isMemberOfProvelegedACL\n"); 

    while(member!=NULL)
    {
        if(member->mid == member_mid)
        {
            
             printf ("\n found the process \n");
            return TRUE;
        }
        member = member->next;
    } 
            printf ("\n process not found   \n");    
    return FALSE;    
}



int isCallBlocked(int publisher)
{
    
    BLOCKED_PROCESSES bl_list = blockedProcesses;
    
    while(bl_list != NULL)
    {
        if( bl_list->publisher== publisher)
        {
            
            return 1;
        }
        bl_list=bl_list->next;
    } 
    
    return 0;
    
}
void saveCurrentStatus(message store, int publisher,char *msg)
{
   BLOCKED_PROCESSES member = NULL;
    
    
    
    if(blockedProcesses== NULL)
    {
        
    	blockedProcesses = (BLOCKED_PROCESSES)malloc(sizeof(struct blockedPublisher));
        blockedProcesses->publisher_pid = store.m7_i2;
        blockedProcesses->group_id = store.m7_i1;
        blockedProcesses->publisher = publisher;
    	blockedProcesses->unblocked = FALSE;
        strcpy(blockedProcesses->msg,msg);
        
        
        memcpy(&blockedProcesses->store,&store,sizeof(message));
        
        
    	blockedProcesses->next=NULL;
    }
    else
    {
        member = blockedProcesses;
        while(member->next!=NULL)
        {
            member=member->next;
        }
        member->next = (BLOCKED_PROCESSES)malloc(sizeof(struct blockedPublisher));
        member=member->next;  
        member->publisher_pid = store.m7_i2;
        member->group_id = store.m7_i1;
        member->publisher = publisher;
        member->unblocked = FALSE;
        strcpy(member->msg,msg);
        memcpy(&member->store,&store,sizeof(message));
        member->next=NULL;
    }
    printProcessesInBlockedState();
      
}

int getOldStatus(message* store, int publisher,char * msg)
{
    
    BLOCKED_PROCESSES bl_list = blockedProcesses;
    
    
    while(bl_list != NULL)
    {
        
        if( bl_list->publisher == publisher)
        {
            memcpy(store,&bl_list->store,sizeof(message));
            strcpy(msg,bl_list->msg);
            return 1;
        }
        bl_list=bl_list->next;
    }
     
    
    return 0;
    
}

int removePublisherFromBlockList(int publisher)
{
    
	BLOCKED_PROCESSES bl_list, temp;
    bl_list = temp = NULL;
    
    
    if(blockedProcesses == NULL)
    {
        printf("Group does not exist.");
        return 0;
    }
    
	if(blockedProcesses->publisher == publisher)
	{
        temp = blockedProcesses;
		blockedProcesses = blockedProcesses->next;
		free(temp);
	}
	else
	{
        
		bl_list = blockedProcesses;
		while(bl_list->next!=NULL)
        {
            
			if(bl_list->next->publisher == publisher)
			{
                temp = bl_list->next;
				break;
			}
			bl_list=bl_list->next;
		}
        
		if(temp!=NULL)
		{
            
			bl_list->next = temp->next;
			free(temp);
		}
		else
		{
            printf("\n Group does not exist.");
			return 0;
		}
	}
    printProcessesInBlockedState();
     
    return 1;
}




void printTheNewGroup()
{
    
	PROCESSES member, t;
    MSG_BUF message;
    int i;
    t = NULL;
	MEMBER_LIST members_in_ACL = NULL;
    
    printf(" PRINT  GROUP\n");
    
    
	member = newgroups;
    if (member == NULL)
    {
        printf("No group");
    }
    else
    {
        
    	while(member!=NULL)
        {
            printf("grpid = %d \n",member->group_id);
			printf("grpname is %s\n", member->group_name);
			/*Project 3*/
			printf("group created by %d\n",member->group_creator);
			
			 if(SECURED_GROUP == member->group_type)
            {
                printf("groupp type = SECURED : %d \n",member->group_type);
            
                printf("no_of_users = %d \n",member->no_of_members);
                printf("User in the ACL of this group are : \n");
                members_in_ACL = member->group_ACL;
                while(members_in_ACL != NULL)
                {
                    printf("\t\t %d \n",members_in_ACL->mid);
                    members_in_ACL = members_in_ACL->next;
                }
            }
            else
            {
                printf("group type is PUBLIC : %d \n",member->group_type);
            }
                
            
			/*_________*/
            printf("no_of_subscribers = %d \n",member->no_of_subscribers);
            printf("no_of_publishers = %d \n",member->no_of_publishers);
            printf("no_of_messages = %d \n",member->no_of_messages);
            for (i =0 ;i < member->no_of_subscribers ; i++)
            {
                printf("subcriber[%d] spid = %d \n",i,member->participants[i].subscriber_pid);
                message  = member->participants[i].sub_read_messages;
                printf("subcriber[%d] message head message = %s\n",i,message);
            }
			
			
			
            for (i =0 ;i < member->no_of_publishers ; i++)
            {
                printf("publishers[%d] spid = %d \n",i,member->leaders[i]);
            }
            printf("grpid = %d \n",member->group_id);
            printf("no_of_messages = %d \n",member->no_of_messages);
            message = member->group_message;
            printf(" Message Que \n");
            while(message!=NULL)
            {
                printf("Message : %s  Read Count : %d",message->message,message->count);
                message =message->next;
            }
            
            member = member->next;
    	}
    }
    
    
}




int do_igcreate()
{
    PROCESSES entry = NULL;
   	 
	char *name;
	name = m_in.m7_p1;
    sys_datacopy(m_in.m_source,m_in.m7_p1, PM_PROC_NR,name,LENGTH);
    
	/*  PROJ 3 */    
	int secured_group=m_in.m7_i1;
	int member_id =m_in.m7_i2;
  	int group_Id = m_in.m7_i3;    

	
	
    printf("\n--------------------------------------------------------------------------\n");
    printf("\n SYSTEM CALL INVOKED:: IG_Create \n\n");

    
     if(!areAllParamsSet)
    {
        resetParams();
    }
	
     if (!isMemberOfProvelegedACL(priveleged_ACL,member_id))
    {
        printf("failed. Not a member of the privileged list \n" );        
        return 0;
    }     
    
    if(newgroups==NULL)
    {
    	
    	newgroups=(PROCESSES)malloc(sizeof(struct processes_group));
        group_id_allocate++;
    	newgroups->group_id=group_id_allocate;
    	newgroups->next=NULL;
    	newgroups->no_of_messages=0;
    	newgroups->no_of_publishers=0;
    	newgroups->no_of_subscribers=0;
		newgroups->group_name =name;
		
		/*  PROJ 3 */    

        if(TRUE == secured_group)
        {
            newgroups->group_type = SECURED_GROUP;
        }
        else
        {
            newgroups->group_type = PUBLIC_GROUP;
        }
        newgroups->group_ACL = NULL;
        newgroups->group_creator = member_id;
        newgroups->no_of_members = 0;
    	entry=newgroups;
    }
	
    else
    {
    	if(no_of_groups<MAX_GROUPS)
    	{
            
    		no_of_groups++;
    		entry=newgroups;
    		while(entry->next!=NULL)
    		{
    			entry=entry->next;
    		}
    		entry->next=(PROCESSES)malloc(sizeof(struct processes_group));
    		entry=entry->next;
            group_id_allocate++;
    		entry->group_id=group_id_allocate;
    		entry->next=NULL;
    		entry->no_of_messages=0;
        	entry->no_of_publishers=0;
			entry->group_name=name;
        	entry->no_of_subscribers=0;
			
			 /*  PROJ 3*/          
            if(TRUE == secured_group)
            {
                newgroups->group_type = SECURED_GROUP;
            }
            else
            {
                newgroups->group_type = PUBLIC_GROUP;
            }
            newgroups->group_ACL = NULL;
            newgroups->group_creator = member_id;
            newgroups->no_of_members = 0;
    	}
    	else
    	{
            printf("\n\t IG_Create FAILED : Maximum number of groups exceeded \n");
            
            
    		return 0;
    	}
    }
    
    group_Id = entry->group_id;
	printTheNewGroup();
    
    return 1;
    
	
    
}

int do_iglookup()
{
	 char p[LENGTH]={0};
    	 
	PROCESSES member=newgroups;
	int i=0;
    int grpIds[MAX_GROUPS+1];
	 int member_id = m_in.m7_i2;
	char name[20];
    printf("\n SYSTEM CALL INVOKED:: IG LookUp \n\n");
    
	
    
	
	if (!isMemberOfProvelegedACL(priveleged_ACL,member_id))
    {
        printf("failed. Not a member of the privileged list \n" );        
        return 0;
    }     
	if(p==NULL){
        
        printf("\n Error: Input Array does not exist.");
        return 0;
	}
    if(member == NULL){
        printf("\n No groups have been registered so far!");
        return 0;
    }
    printf("\n The registered interest groups are as follows:");
	while(member!=NULL)
	{
        grpIds[i++] =  member->group_id;
		name[i]=member->group_name;
         
        printf("\n Group :%d",member->group_id);
		printf("\n Group name is %s", member->group_name);
        member = member->next;
	}
    
       sys_datacopy(PM_PROC_NR,grpIds,m_in.m_source ,m_in.m7_p1,sizeof(grpIds));
    printf("\n--------------------------------------------------------------------------\n");
    return 0;  
}

int do_igpublisher()
{
	 int grpid=m_in.m7_i1;
	int publisher_id=m_in.m7_i2;
   
	PROCESSES member =newgroups;
    
    
	
    printf("\n SYSTEM CALL INVOKED:: register as Publisher \n\n");
    
	if (!isMemberOfProvelegedACL(priveleged_ACL,publisher_id))
    {
        printf("failed. Not a member of the privileged list \n" );        
        return 0;
    }   
    
	while(member!=NULL)
	{
        
		if(member->group_id==grpid)
		{
            
			break;
		}
		member=member->next;
	}
	if(member!=NULL)
	{
        
         
        
		if(member->no_of_publishers<MAX_PUBLISHERS)
		{
			if((PUBLIC_GROUP == member->group_type) || isMemberOfProvelegedACL(member->group_ACL,publisher_id))
            {
                   printf("\Publisher added to the list \n");                  
    			member->leaders[member->no_of_publishers++].publisher_pid=publisher_id;
            }
            else
            {
                 printf("\nFailed. NOt a member of the ACL\n");                    
                return 0; 
            }
		
			 
		}
		else
		{
            printf("\n Maximum no: of publishers reached.");
			return 0;
		}
	}
	else
	{
        
        printf("\n Group %d could not be found.", grpid);
		return 0;
	}
    printTheNewGroup();
    printf("\n"); 
    return 1;
}

int do_igsubscriber()
{
	 int grpid=m_in.m7_i1;
	int subscriber_id=(int)m_in.m7_i2;
    
	PROCESSES member =newgroups;
    
    
    printf("\n SYSTEM CALL INVOKED:: register as Subscriber \n\n");
	if (!isMemberOfProvelegedACL(priveleged_ACL,subscriber_id))
    {
        printf("failed. Not a member of the privileged list \n" );        
        return 0;
    }   
	while(member!=NULL)
	{
		if(member->group_id==grpid)
		{
			break;
		}
		member=member->next;
	}
	if(member!=NULL)
	{
		
        
        
        if(member->no_of_subscribers<MAX_SUBSCRIBERS)
		{
		
		 if((PUBLIC_GROUP == member->group_type) || isMemberOfProvelegedACL(priveleged_ACL,subscriber_id))
            {            
                printf("\Added subscriber to ACL \n");                  
    			member->participants[member->no_of_subscribers].subscriber_pid=subscriber_id;
			member->participants[member->no_of_subscribers].sub_read_messages=member->group_message;
			member->no_of_subscribers++;
            }
            else
            {
             printf("\n Failed. Not a member of ACL\n");                      
                return 0;
            }
			 
		}
		else
		{
            printf("\n Maximum no: of subscribers reached.");
			return 0;
		}
	}
	else
	{
        
        printf("\n Group could not be found.");
		return 0;
	}
    printTheNewGroup(); 
    return 1;
}


int do_igpublish()
{
	  int grpid;
	int prid;
	PROCESSES member =newgroups;
	int i, found;
    char *message;
    int val;
    MSG_BUF messgae_buffer =NULL;
    register struct mproc *rmp;
    
    
    printf("\n SYSTEM CALL INVOKED:: publish \n\n");
    
    rmp = &mproc[who_p];
    printf("Message coming in is\n %s",m_in.m7_p1);
    printf("\nIncoming group id is:%d",m_in.m7_i1);
    printf("\nIncoming publisher id is:%d",m_in.m7_i2);
    message = m_in.m7_p1;
	printf("Message is here %s", message);
    
    if (!isCallBlocked(who_p))
    {
        printf("\nInside the right deadlock block");
        sys_datacopy(m_in.m_source,m_in.m7_p1, PM_PROC_NR,message,LENGTH);
		if(message==NULL)
		{
            printf("No message \n");
			return 0;
		}
    }
    else
    {
        printf("\nDeadlocked");
        val = getOldStatus(&m_in,who_p,message);
        if (val < 0)
        {
            printf ("error in getting old status \n");
            return 0;
        }
        
        val = removePublisherFromBlockList(who_p);
        if (val < 0)
        {
            printf (" error in removing the blocked publisher \n");
            return 0;
        }
    }
    
    printf("I am here Thank you \n");
    grpid=m_in.m7_i1;
    prid=(int)m_in.m7_i2;
    
    
	while(member!=NULL)
	{
		if(member->group_id==grpid)
		{
			break;
		}
		member=member->next;
	}
	if(member!=NULL)
	{
		/* Check if pid is already registered as a publisher. */
	  	found=0;
        printf("\nmember is:%d",member->no_of_publishers);
		for (i = 0; i < member->no_of_publishers; i++)
		{
			if (member->leaders[i].publisher_pid == prid)
			{
				found = 1;
				break;
			}
		}
		if (found == 0)
		{
            printf("\n Error: Not a valid Publisher.");
			return 0;
		}
		
        printf("\nI found publisher!!\n");
        
		if (member->no_of_messages == 0 || member->group_message == NULL)
		{
			member->group_message = (MSG_BUF) malloc(sizeof(struct messages_queue));
            
			strcpy(member->group_message->message, message);
            
			member->group_message->count = 0;
			member->group_message->next = NULL;
			member->no_of_messages++;
            
            
			for (i = 0; i < member->no_of_subscribers; i++)
			{
				member->participants[i].sub_read_messages = member->group_message;
			}
		}
		
        else if (member->no_of_messages < QUEUE_FULL)
        {
            
			messgae_buffer = member->group_message;
			while (messgae_buffer->next != NULL)
			{
				messgae_buffer = messgae_buffer->next;
			}
			messgae_buffer->next = (MSG_BUF) malloc(sizeof(struct messages_queue));
			messgae_buffer = messgae_buffer->next;
			
			strcpy(messgae_buffer->message, message);
            printf("\n Message   : %s", messgae_buffer->message);
			messgae_buffer->count = 0;
            messgae_buffer->next = NULL;
            
			member->no_of_messages++;
            
            
		}
		
        else if (member->no_of_messages >= QUEUE_FULL)
		{
			printf("Queue already has 5 messages \n");
            
#if 0
            return 0;
#endif
            
            saveCurrentStatus(m_in,who_p,message);
            
            
            rmp->mp_flags |= SIGSUSPENDED;
            
            
            return 0;
            
		}
	}
	else
	{
        printf("\n group could not be found.");
		return 0;
	}
    
    printTheNewGroup();  
    return 1;
}

int do_igretrieve()
{
    
	  int grpid=m_in.m7_i1;
	int prid=(int)m_in.m7_i2;
    char message[LENGTH]={0};
	MSG_BUF info=NULL;
	PROCESSES member =newgroups;
	int i, found;
    
    printf("\n SYSTEM CALL INVOKED:: subscribe \n\n");
    
    
    
	while(member!=NULL)
	{
		if(member->group_id==grpid)
		{
			break;
		}
		member=member->next;
	}
    
	if(member!=NULL)
	{
        
		if(member->group_message==NULL)
		{
            
			return 0;
		}
        
		found=0;
		for (i = 0; i < member->no_of_subscribers; i++)
		{
			if (member->participants[i].subscriber_pid == prid)
			{
				found = 1;
				break;
			}
		}
		if (found == 0)
		{
            printf("\n Error. No subscriber found ");
			return 0;
            
		}
		
        
        
		strcpy(message, member->participants[i].sub_read_messages->message);
		//sys_datacopy(PM_PROC_NR,message,m_in.m_source,m_in.m7_p1,LENGTH);
		printf("Message in sys is %s", message);
        
		member->participants[i].sub_read_messages->count++;
        
        
		if (member->participants[i].sub_read_messages->count >= member->no_of_subscribers)
		{
			info = member->participants[i].sub_read_messages;
            unblockPublisherFromBlockList(member->group_id);
            
            
		}
		
		member->participants[i].sub_read_messages = member->participants[i].sub_read_messages->next;
		if (message != NULL)
		{
			member->group_message=member->group_message->next;
            member->no_of_messages--;
			printf("Read message: %s",message);
		}else{
			printf("Message is %s \n", message);
            printf("No messages to be read from this group\n");
        }
	}
	else
	{
        printf("\n Group could not be found."); 
		return 0;
	} 
    
    
    return 1;
}


int do_igdelete()
{
	 PROCESSES member =NULL;
    PROCESSES temp =NULL;
	int grpid=m_in.m7_i1;
	MSG_BUF info;
    
    /*  PROJ 3 */     
    int member_id = m_in.m7_i2;
	 
    MEMBER_LIST ml;
    
	 if (!isMemberOfProvelegedACL(priveleged_ACL,member_id))
    {
        printf("failed. Not a member of the privileged list \n" );        
        return 0;
    }        
	
	
    if(newgroups == NULL)
    {
        printf("\n No groups registered so far!");
        return 0;
    }
    
    
	if(newgroups->group_id == grpid)
	{
     
		if(newgroups->group_creator != member_id || MAIN_MEMBER_ID != member_id)
        {
            printf("Not the ceator\n");
			return 0; 
        }
	 
		temp=newgroups;
		newgroups = newgroups->next;
		while(member->group_message!=NULL)
		{
			info=member->group_message;
			member->group_message=member->group_message->next;
			free(info);
		}
		
		while(member->group_ACL != NULL)
        {
			ml = member->group_ACL;
			member->group_ACL = member->group_ACL->next;
			free(ml);       
        }
		free(temp);
        no_of_groups--;
	}
	else
	{
        
		member=newgroups;
		while(member->next!=NULL){
			if(member->next->group_id==grpid)
			{
				temp=member->next;
				break;
			}
			member=member->next;
		}
		if(temp!=NULL)
		{
			if(newgroups->group_creator != member_id)
            {
                printf("\n Group not found\n");
				return 0;  
            }  
		
		
			member->next=temp->next;
            
			while(member->group_message!=NULL)
			{
				info=member->group_message;
				member->group_message=member->group_message->next;
				free(info);
			}
            while(member->group_ACL != NULL)
            {
    			ml = member->group_ACL;
    			member->group_ACL = member->group_ACL->next;
    			free(ml);       
            }            
			 
			free(temp);
            no_of_groups--;             
		}
		else
		{
            
            printf("\n Group does not exist.");
			return 0;
		}
	}
    
    printTheNewGroup();  
    
     return 1;
}

/*Project 3 */
int do_subscribeToPrivelegedList(){
 
    MEMBER_LIST abc = NULL;
    int member_id = m_in.m7_i1;
	int creator_id =m_in.m7_i2;



    printf("\n SYSTEM CALL INVOKED:: subscribeToPrivelegedList \n\n");       

     printf(" Member Id : %d \n creator_id : %d\n",member_id,creator_id);

    if(!areAllParamsSet)
    {
        
        resetParams();
    }

    if(MAIN_MEMBER_ID != creator_id )
    {
        printf("You are not a root user. Operation failed\n"); 
        return 0;
    }

    if (isMemberOfProvelegedACL(priveleged_ACL,member_id))
    {
        printf("Member exists \n" );    
				/*have a dbt as to should we return 0 here \n*/
        
    }  
     
        

    
    if(priveleged_ACL ==NULL)
    {
        printf("\n First group ");
         
        abc = (MEMBER_LIST)malloc(sizeof(struct member_list));
        abc->mid = 0;
        priveleged_ACL = abc;
        abc->next = (MEMBER_LIST)malloc(sizeof(struct member_list));
        abc = abc->next;
        abc->mid = member_id;
        abc->next = NULL;
        no_of_priveleged_users =2;
    }
    else
    {
        if(no_of_priveleged_users < MAX_NO_PRIVILEGED_USERS)
        {
             
            abc = priveleged_ACL;
            while(abc->next!=NULL)
            {
                abc = abc->next;
            }
            abc->next=(MEMBER_LIST)malloc(sizeof(struct member_list));
            abc=abc->next;
            abc->mid = member_id;
            abc->next = NULL;
            no_of_priveleged_users++;
        }
        else
        {
             printf("\n  Maximum number of privilege user exceeded \n");
            return 0;
        }
    }

    
    printACL();
    printTheNewGroup();    
    
    return 1;
}





int do_unsubscribeFromPrivelegedList(){
	MEMBER_LIST member =NULL;
    MEMBER_LIST temp =NULL;
    
    int member_id=m_in.m7_i1;
	int creator_id=m_in.m7_i2;


     printf("\n SYSTEM CALL INVOKED:: unsubscribeFromPrivelegedList \n\n");        

	 printf("\n Inside do_unsubscribeFromPrivelegedList " ); 
    
    if(MAIN_MEMBER_ID != creator_id )
    {
         printf("You are not a root user. Operation failed\n"); 
        return 0;
    }


    if(priveleged_ACL == NULL)
    {
         printf("\n privleged ACL is NULL");
        return 0;
    }


	if(priveleged_ACL->mid == member_id)
	{
         
		temp=priveleged_ACL;
		priveleged_ACL = priveleged_ACL->next;

		free(temp);
        no_of_priveleged_users--;
	}
	else
	{
         
		member=priveleged_ACL;
		while(member->next!=NULL){
			if(member->next->mid == member_id )
			{
				temp=member->next;
				break;
			}
			member=member->next;
		}
		if(temp!=NULL)
		{
			member->next=temp->next;
			free(temp);
            no_of_priveleged_users--;
		}
		else
		{
            
            printf("\n Group does not exist.");
			return 0;
		}
	}

     
    printACL();
    printTheNewGroup();


    return 1;
}

int do_registerToSecuredGroup(){


    int member_id=m_in.m7_i1;
	int creator_id=m_in.m7_i2;
    int group_Id = m_in.m7_i3;

	PROCESSES member=newgroups;
    MEMBER_LIST group_A_C_L = NULL;

     printf("\n SYSTEM CALL INVOKED:: do_registerToSecuredGroup \n\n");     

	 printf("\n Inside do_registerToSecuredGroup :\n"); 


     if (!isMemberOfProvelegedACL(priveleged_ACL,member_id))
    {
        printf("Member not in priveleged list \n" );        
        return 0;
    }  
         

	while(member!=NULL)
	{

		if(member->group_id==group_Id)
		{
			 printf("\n group  Found." );
			break;
		}
		member=member->next;
	}
	if(member!=NULL)
	{
        if(MAIN_MEMBER_ID == member_id || creator_id == member->group_creator )
        {

            

            group_A_C_L = member->group_ACL;
            
            if(group_A_C_L ==NULL)
            {

                
                 
                group_A_C_L = (MEMBER_LIST)malloc(sizeof(struct member_list));
                group_A_C_L->mid = member_id;
                group_A_C_L->next = NULL;
                
                member->no_of_members++;
                member->group_ACL = group_A_C_L;
            }
            else
            {
                
                if(member->no_of_members < MAX_NO_GROUP_MEMBERS)
                {
                     
                    while(group_A_C_L->next!=NULL)
                    {
                        group_A_C_L = group_A_C_L->next;
                    }
                    group_A_C_L->next=(MEMBER_LIST)malloc(sizeof(struct member_list));
                    group_A_C_L=group_A_C_L->next;
                    group_A_C_L->mid = member_id;
                    group_A_C_L->next = NULL;
                    member->no_of_members++;
                }
                else
                {
                     printf("\n Maximum number of user exceeded \n");
                    return 0;
                }
            }

        }
        else
        {
            printf("\n User is not creator\n");
			return 0;  
        }
	}
	else
	{
        printf("\n Group could not be found." );
		return 0;
	}

    printTheNewGroup();
    
     
    return 1;    

}

int do_unregisterFromSecuredGroup(){
 int member_id=m_in.m7_i1;
    int creator_id=m_in.m7_i2;
    int group_Id = m_in.m7_i3;

    PROCESSES member=newgroups;
    MEMBER_LIST group_A_C_L = NULL;
    MEMBER_LIST temp = NULL;


    printf("\n SYSTEM CALL INVOKED:: do_unregisterFromSecuredGroup \n\n");     


    while(member!=NULL)
    {

        if(member->group_id==group_Id)
        {
            printf("\n Group Found." );
            break;
        }
        member=member->next;
    }
    if(member!=NULL)
    {
        if(MAIN_MEMBER_ID == member_id || creator_id == member->group_creator)
        {

             
            group_A_C_L = member->group_ACL;

            if(group_A_C_L->mid == member_id)
            {
                

                member->group_ACL = member->group_ACL->next;
            
                free(group_A_C_L);
                member->no_of_members--;
            }
            else
            {
                
                while(group_A_C_L->next!=NULL){
                    if(group_A_C_L->next->mid == member_id )
                    {
                        temp = group_A_C_L->next;
                        break;
                    }
                    group_A_C_L = group_A_C_L->next;
                }
                if(temp!=NULL)
                {
                    group_A_C_L->next = temp->next;
                    free(temp);
                    member->no_of_members--;
                }
                else
                {
                    
                     printf("\n  Group does not exist \n");
                    
                    return 0;
                }
            }

        }
        else
        {
            printf("\n User is not creator\n");
			return 0;  
        }
    }
    else
    {
         printf("\n Group could not be found." );
        return 0;
    }

    printTheNewGroup();


     
    return 1;    
}

/*Project 3 */ 
  int do_reset_MINIX()
{
	 
	PROCESSES member=NULL;
	MSG_BUF info=NULL;
    BLOCKED_PROCESSES bl_processes = NULL;
   MEMBER_LIST member_list = NULL;

     printf("\n SYSTEM CALL INVOKED:: resetMINIX \n\n");

         
	while(newgroups!=NULL)
	{
		member=newgroups;
        while(member->group_message!=NULL)
        {
            info = member->group_message;
            member->group_message=member->group_message->next;
            free(info);
        }
              
        while(member->group_ACL != NULL)
        {
			member_list = member->group_ACL;
			member->group_ACL = member->group_ACL->next;
			free(member_list);      
        }        
		newgroups=newgroups->next;
		free(member);
	}
    while(blockedProcesses!=NULL)
    {
        bl_processes = blockedProcesses;
        blockedProcesses = blockedProcesses->next;
        free(bl_processes);
    }          

     

    while(priveleged_ACL!=NULL)
    {
        member_list = priveleged_ACL;
        priveleged_ACL = priveleged_ACL->next;
        free(member_list);
    }
        
	no_of_groups=0;

    
    no_of_priveleged_users =0;
	 

    resetParams();

     

    return 1;
}
