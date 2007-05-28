/*BINFMTC: 
 *  qemubuilder: pbuilder with qemu
 *  Copyright (C) 2007 Junichi Uekawa
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

const char* qemu_keyword="END OF WORK EXIT CODE=";

typedef struct pbuilderconfig 
{
  int mountproc;
  int mountdev;
  int mountdevpts;
  int save_after_login;
  char* buildplace;		/* /var/cache/pbuilder/build/XXX.$$ */
  char* buildresult;		/* /var/cache/pbuilder/result/ */
  char* basepath;		/* /var/cache/pbuilder/cow */
  char* arch;
  char* mirror;
  char* distribution;

  /* more qemu-isque options */
  char* kernel_image;
  char* initrd;
  int memory_megs;		/* megabytes of memory */

  enum {
    pbuilder_do_nothing=0,
    pbuilder_help,
    pbuilder_build,
    pbuilder_create,
    pbuilder_update,
    pbuilder_execute,
    pbuilder_login
  } operation;
}pbuilderconfig;

/* 
   
The pbuilder command-line to pass

0: pbuilder
1: build/create/login etc.
offset: the next command

The last-command will be
PBUILDER_ADD_PARAM(NULL);

 */
#define MAXPBUILDERCOMMANDLINE 256
char* pbuildercommandline[MAXPBUILDERCOMMANDLINE];
int offset=2;
#define PBUILDER_ADD_PARAM(a) \
 if(offset<(MAXPBUILDERCOMMANDLINE-1)) \
 {pbuildercommandline[offset++]=a;} \
 else \
 {pbuildercommandline[offset]=NULL; fprintf(stderr, "pbuilder-command-line: Max command-line exceeded\n");}

/*
  execlp that does fork.
  
  NULL-terminated list of parameters.

  cf. execl from FreeBSD sources, and glibc posix/execl.c, 
  and cygwin exec.cc

  @return < 0 for failure, exit code for other cases.
 */
static int
forkexeclp (const char *path, const char *arg0, ...)
{
  int i, ret;
  va_list args;
  const char *argv[1024];
  pid_t pid;
  int status;

  va_start(args, arg0);
  argv[0] = arg0;
  i = 1;
  do
    {
      argv[i] = va_arg(args, const char *);
      
      if ( i >= 1023 ) 
	{
	  return -1;
	}
      
    }
  while (argv[i++] != NULL);
  va_end (args);

  switch(pid=fork())
    {
    case 0:
      execvp(path, (char*const*)argv);
      perror("pbuilder: execlp");
      exit(EXIT_FAILURE);
    case -1:
      /* error condition in fork(); something is really wrong */
      perror("pbuilder: fork");
      return -1;
    default:
      /* parent process, waiting for termination */
      waitpid(pid, &status, 0);
      if (!WIFEXITED(status))
	{
	  /* something unexpected */
	  return -1;
	}
      ret = WEXITSTATUS(status);
    }
  return ret;
}

/**
 * arch-specific routine; the console device to make
 */
static const char* qemu_arch_serialdevice(const char* arch)
{
  if (!strcmp(arch, "arm"))
    {
      return "mknod dev/console c 204 64; ";
    }
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64") ||
	   !strcmp(arch, "mips") ||
	   !strcmp(arch, "mipsel")
	   )
    {
      return "mknod dev/console c 4 64; ";
    }
  else
    return NULL;
}

/**
 * arch-specific routine; disk device name to use
 */
static const char* qemu_arch_diskdevice(const char* arch)
{
  if (!strcmp(arch, "arm"))
    {
      return "sd";
    }
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64")||
	   !strcmp(arch, "mips")||
	   !strcmp(arch, "mipsel")
	   )
    {
      return "hd";
    }
  else
    return NULL;
}


/**
 * arch-specific routine; qemu command to use.
 */
static const char* qemu_arch_qemu(const char* arch)
{
  if (!strcmp(arch, "arm"))
    {
      return "qemu-system-arm";
    }
  if (!strcmp(arch, "mips"))
    return "qemu-system-mips";
  else if (!strcmp(arch, "mipsel"))
    return "qemu-system-mipsel";
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64"))
    {
      FILE*f=popen("dpkg --print-architecture", "r");
      char*s;
      fscanf(f, "%as", &s);
      pclose(f);
      if(!strcmp(s,arch) && !(system("which kvm")))
	return "kvm";
      else if(!strcmp(s, "i386"))
	return "qemu";
      else 
	return "qemu-system-x86_64";
    }
  else
    return NULL;
}

/**
 * arch-specific routine; the machine spec for this arch
 */
static const char* qemu_arch_qemumachine(const char* arch)
{
  if (!strcmp(arch, "arm"))
    return "versatilepb";
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64"))
    return "pc";
  else if (!strcmp(arch, "mips")||
	   !strcmp(arch, "mipsel"))
    return "mips";
  else
    return NULL;
}


/**
 * arch-specific routine; the serial device
 */
static const char* qemu_arch_tty(const char* arch)
{
  if (!strcmp(arch, "arm"))
    {
      return "ttyAMA0";
    }
  else if (!strcmp(arch, "i386") ||
	   !strcmp(arch, "amd64")||
	   !strcmp(arch, "mips")||
	   !strcmp(arch, "mipsel"))
    {
      return "ttyS0";
    }
  else
    return NULL;
}

/** create a sparse ext3 block device suitable for 
    loop-mount.

   @returns -1 on error, 0 on success
 */
static int create_ext3_block_device(const char* filename, int gigabyte)
{
  int ret;
  char *s;
  char *s2;

  /* create 10GB sparse data */
  if (0>asprintf(&s, "of=%s", filename))
    {
      /* out of memory */
      return -1;
    }
  if (0>asprintf(&s2, "seek=%i", gigabyte * 1024))
    {
      /* out of memory */
      return -1;
    }
  ret=forkexeclp("dd", "dd", "if=/dev/zero", s,
		 "bs=1M",
		 s2, "count=1", NULL);
  if (ret)
    {
      ret=-1;
      goto out;
    }

  free(s);
  s=NULL;
  if (0>asprintf(&s, "yes | mke2fs -j -O sparse_super %s", filename))
    {
      /* out of memory */
      ret=-1;
      goto out;
    }
  if ((ret=system(s)))
    {
      ret=-1;
    }

 out:
  if(s) free(s);
  if(s2) free(s2);
  return ret;
}

/** loopback mount file system.
    @returns 0 on success
*/
static int loop_mount(const char* device, const char* mountpoint)
{
  int ret=forkexeclp("mount", "mount", "-o", "loop",
	     device, mountpoint, NULL);
  return ret;
}

/**
   loopback umount file system

   @returns 0 on success
 */
static int loop_umount(const char* device)
{
  int ret=forkexeclp("umount", "umount", device,
		     NULL);
  return ret;
}

/** create a script here. 
    
@returns -1 on failure, 0 on success.
*/
static int create_script(const char* mountpoint, const char* relative_path, const char* scriptcontent)
{
  char *s;
  FILE* f;
  int ret=-1;
  
  asprintf(&s, "%s/%s", mountpoint, relative_path);
  if(!(f=fopen(s, "w")))
    goto fail;
  if(EOF==fputs(scriptcontent, f))
    goto fail;
  if(EOF==fclose(f))
    goto fail;
  if(chmod(s, 0700))
    goto fail;
  ret=0;
 fail:
  free(s);
  return ret;
}



/**
   copy file contents to temporary filesystem, so that it can be used inside qemu.
 */
static int copy_file_internal(const char* orig, const char*dest)
{
  const int buffer_size=4096;
  char *buf = malloc (buffer_size);
  int ret=-1;
  FILE* fin;
  FILE* fout;
  size_t count;
  
  
  fin=fopen(orig, "r");
  fout=fopen(dest, "w");
  if (!fin)
    {
      goto out;
    }
  if (!fout)
    {
      goto out;
    }
  while((count=fread(buf, 1, buffer_size, fin)))
    {
      fwrite(buf, 1, count, fout);
    }
  if(ferror(fin))
    {
      /* TODO: handle error */
      goto out;
    }
  fclose(fin);
  fclose(fout);
  ret=0;
 out:
  return ret;
}

static int copy_file_contents_to_temp(const char* orig, const char*temppath, const char* filename)
{
  char* s;
  int ret;

  asprintf(&s, "%s/%s", temppath, filename);
  ret=copy_file_internal(orig, s);
  free(s);
  return ret;
}

/**
   run qemu until exit signal is given.

*/
static int fork_qemu(const char* hda, const char* hdb, const struct pbuilderconfig* pc)
{
  pid_t child;
  int sp[2];
  fd_set readfds;
  int retval;
  const int buffer_size=4096;
  char* buf=malloc(buffer_size);
  size_t count;
  
  if (-1==socketpair(AF_UNIX, SOCK_STREAM,0,sp))
    {
      /* error handle? */
      return -1;
    }
  
  if ((child=fork()))
    {
      /* this is parent process */
      close(sp[1]);
      FD_ZERO(&readfds);
      while (1)
	{
	  FD_SET(0,&readfds);
	  FD_SET(sp[0],&readfds);
	  if (-1!=(retval=select(sp[0]+1,&readfds, NULL, NULL, NULL)))
	    {
	      if (FD_ISSET(0,&readfds))
		{
		  /* data available from user, send it off to qemu */
		  count=read(0,buf,buffer_size);
		  write(sp[0],buf,count);
		}
	      if (FD_ISSET(sp[0],&readfds))
		{
		  /* data available from qemu */
		  
		  /* sleep a bit to let it buffer-up a bit more. */
		  usleep(100000);
		  
		  count=read(sp[0],buf,buffer_size);
		  
		  /* this won't work sometimes, since things are more split-up than this,
		     but this is a good best-effort thing. */
		  if (memmem(buf, count, qemu_keyword, strlen(qemu_keyword)))
		    {
		      int status;
		      
		      printf("  -> received termination signal, sending exit monitor signal to qemu\n");
		      write(sp[0],"x",2);
		      sleep (1);

		      /* try to send kill signal after graceful exit */
		      printf("  -> killing child process (qemu)\n");
		      if (!kill(child, SIGTERM))
			printf("   -> killed (qemu)\n");
		      else
			perror("   -> ");
		      waitpid(child, &status, 0);
		      printf("  -> child process terminated with status: %x\n", status);
		      break;
		    }
		  write(1,buf,count);
		}
	    }
	  else
	    {
	      perror("select");
	      break;
	    }
	}
    }
  else if(child == 0)
    {
      /* this is the child process */
      const char* qemu = qemu_arch_qemu(pc->arch); 
      const char* machine = qemu_arch_qemumachine(pc->arch);
      char* append_command;
      const char* kernel_image = pc->kernel_image;
      const char* initrd = pc->initrd;
      char* mem;

      asprintf(&mem, "%i\n", pc->memory_megs);
      
      asprintf(&append_command,
	       "root=/dev/%sa init=/pbuilder-run console=%s",
	       qemu_arch_diskdevice(pc->arch),
	       qemu_arch_tty(pc->arch));
            
      dup2(sp[1],0);
      dup2(sp[1],1);
      dup2(sp[1],2);
      close(sp[0]);
      
      if (initrd)
	{
	  execlp(qemu, qemu, 
		 "-nographic",
		 "-M", machine, 
		 "-m", mem,
		 "-kernel", kernel_image, 
		 "-initrd", initrd, 
		 "-hda", hda,
		 "-hdb", hdb, 
		 "-append", append_command, 
		 "-serial", "stdio", NULL);
	}
      else
	{
	  execlp(qemu, qemu, 
		 "-nographic",
		 "-M", machine, 
		 "-m", mem,
		 "-kernel", kernel_image, 
		 "-hda", hda,
		 "-hdb", hdb, 
		 "-append", append_command, 
		 "-serial", "stdio", NULL);
	}
      perror("execlp");
      exit (1);
    }
  else
    {
      perror("fork");
      return -1;
    }
  return 0;
}

static int do_fsck(const char* devfile)
{
  return forkexeclp("/sbin/fsck",
		    "/sbin/fsck",
		    devfile, 
		    NULL);
}

static int run_second_stage_script
(int save_result,
 const char* commandline,
 const struct pbuilderconfig* pc,
 const char* hostcommand1,
 const char* hostcommand2)
{
  char* script;
  char* workblockdevicepath;
  char* cowdevpath;
  
  int ret=1;
  
  if (mkdir(pc->buildplace,0777))
    {
      /* could not create the buildplace here. */
      perror("mkdir");
      goto out;
    }

  do_fsck(pc->basepath);

  asprintf(&workblockdevicepath, "%s.dev", pc->buildplace);
  ret=create_ext3_block_device(workblockdevicepath, 1);
  loop_mount(workblockdevicepath, pc->buildplace);

  asprintf(&script, 
	   "#!/bin/bash\n"
	   "echo \n"
	   "echo ' -> qemu-pbuilder second-stage' \n"
	   //TODO: copy hook scripts
	   //"mount -n /proc /proc -t proc\n" // this is done in first stage.
	   "dhclient eth0\n"
	   "cp $PBUILDER_MOUNTPOINT/hosts /etc/hosts\n"
	   "cp $PBUILDER_MOUNTPOINT/resolv.conf /etc/resolv.conf\n"
	   "cp $PBUILDER_MOUNTPOINT/hostname /etc/hostname\n"
	   "hostname pbuilder-$(cat /etc/hostname)\n"
	   //TODO: run G hook
	   "%s\n"
	   "apt-get clean || true\n"
	   "sync\n"
	   "sync\n"
	   "while sleep 3s; do\n"
	   "  echo ' -> qemu-pbuilder END OF WORK EXIT CODE=0'\n"
	   "done\n",
	   commandline
	   );

  /* this is specific to my configuration, fix it later. */
  create_script(pc->buildplace, "pbuilder-run",
		script);
  /* TODO: can I do 'date --set' from output of 'LC_ALL=C date' */

  /* TODO: copy /etc/hosts etc. to inside chroot, or it won't know the hosts info. */
  copy_file_contents_to_temp("/etc/hosts", pc->buildplace, "hosts");
  copy_file_contents_to_temp("/etc/hostname", pc->buildplace, "hostname");
  copy_file_contents_to_temp("/etc/resolv.conf", pc->buildplace, "resolv.conf");

  /* do I not need to copy /etc/pbuilderrc, and ~/.pbuilderrc to inside chroot? */
  /* TODO: hooks probably need copying here. */
  /* TODO: recover aptcache */

  if(hostcommand1) 
    {
      printf("running host command: %s\n", hostcommand1);
      system(hostcommand1);
    }

  loop_umount(workblockdevicepath);

  asprintf(&cowdevpath, "%s.cowdev", pc->buildplace);
  ret=forkexeclp("qemu-img", "qemu-img", 
		 "create",
		 "-f",
		 "qcow", 
		 "-b", 
		 pc->basepath, 
		 cowdevpath, 
		 NULL);

  // this will have wrong time within qemu, for ARM
  // how to workaround?
  fork_qemu(cowdevpath, workblockdevicepath, pc);

  /* commit the change here */
  if (save_result)
    {
      printf(" -> commit change to qemu image\n");
      ret=forkexeclp("qemu-img", "qemu-img", 
		     "commit",
		     cowdevpath, 
		     NULL);  
    }

  /* after-run */
  loop_mount(workblockdevicepath, pc->buildplace);
  printf(" -> running post-run process\n");
  if(hostcommand2) 
    {
      printf("running host command: %s\n", hostcommand2);
      system(hostcommand2);
    }
  loop_umount(workblockdevicepath);
  rmdir(pc->buildplace);
  printf(" -> clean up COW device files\n");
  unlink(workblockdevicepath);
  unlink(cowdevpath);
  ret=0;

 out:
  if(workblockdevicepath) free(workblockdevicepath);
  if(cowdevpath) free(cowdevpath);
	   if(script) free(script);
  return ret;

}

/* 
   @return shell command to copy the dsc file.
 */
static char* copy_dscfile(const char* dscfile_, const char* destdir)
{
  int ret=1;
  size_t bufsiz=0;
  char* buf;
  char* filename;
  char* origdir;
  char* dscfile=canonicalize_file_name(dscfile_);
  FILE* f=fopen(dscfile,"r");

  char* memstr=0;
  size_t len=0;
  FILE* fmem=open_memstream(&memstr, &len);
  int filelist=0;

  origdir=strdup(dscfile);
  (*(strrchr(origdir,'/')))=0;	/* if this isn't working, I'm dead */
    
  fprintf(fmem, "cp %s %s/\n", 
	  dscfile, destdir);

  while (getline(&buf,&bufsiz,f)>0)
    {
      if (strrchr(buf,'\n'))
	{
	  *(strrchr(buf,'\n'))=0;
	}
      if(filelist)
	{
	  if(sscanf(buf, " %*s %*s %as", &filename)!=1)
	    goto out;
	  fprintf(fmem, "cp %s/%s %s/\n", 
		  origdir, filename, destdir);
	  assert(filename);
	  free(filename);
	}  
      if (!(buf[0]==' ')&&
	  !strcmp(buf,"Files: "))
	{
	  filelist=1;
	}
      
    }
  ret=0;
  assert(fmem);
  assert(f);
  fclose(fmem);
  fclose(f);

out:
  if(buf) free(buf);
  if(origdir) free(origdir);
  if(dscfile) free(dscfile);
  return ret?NULL:memstr;
}

int qemubuilder_create(const struct pbuilderconfig* pc)
{
  int ret;
  char* s = NULL;  
  char* workblockdevicepath;

  if((ret=unlink(pc->basepath)))
    {
      //ignore this
    }
  
  /* 3GB should be enough to install any Debian system; hopefully */
  ret=create_ext3_block_device(pc->basepath, 3);

  if (ret)
    {
      goto out;
    }
  
  if (mkdir(pc->buildplace,0777))
    {
      /* could not create the buildplace here. */
      perror("mkdir");
      goto out;
    }

  ret=loop_mount(pc->basepath, pc->buildplace);
  if (ret)
    {
      goto out;
    }

  printf(" -> Invoking debootstrap\n");
  ret=forkexeclp("debootstrap", "debootstrap", 
		 "--arch", 
		 pc->arch, 
		 "--foreign",
		 pc->distribution,
		 pc->buildplace, 
		 pc->mirror, 
		 NULL);
  if (ret)
    {
      goto umount_out;
    }

  /* arch-dependent code here.
     create required device files.
   */

  /* 
     ttyAMA0 is probably ARM-specific 
     others are probably linux-portable as documented in linux/Documentation/devices.txt
     other OSes will require different, but hey, they probably don't even boot from ext3, 
     we'll need think of other ways to work with them.
   */
  asprintf(&s, 
	   "cd %s; "
	   "%s"
	   "mknod dev/ttyS0 c 4 64; "
	   "mknod dev/ttyAMA0 c 204 64; "
	   "mknod dev/sda b 8 0; mknod dev/sdb b 8 16; "
	   "mknod dev/hda b 3 0; mknod dev/hdb b 3 64; ", 
	   pc->buildplace,
	   qemu_arch_serialdevice(pc->arch)
	   );
  if ((ret=system(s)))
    {
      goto umount_out;
    }
  free(s);
  
  /* this 'sdb' part is arch specific. */
  asprintf(&s,
	   "#!/bin/bash\n"
	   "echo \n"
	   "echo ' -> qemu-pbuilder first-stage' \n"
	   "export PATH='/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin'\n"
	   "mount -n /proc /proc -t proc\n"
	   "mount -n -o rw,remount /\n"
	   "cp /proc/mounts /etc/mtab\n"
	   "export PBUILDER_MOUNTPOINT=/var/cache/pbuilder/pbuilder-mnt\n"
	   "mkdir -p $PBUILDER_MOUNTPOINT\n"
	   "mount -n -t ext3 /dev/%sb $PBUILDER_MOUNTPOINT \n"
	   "$PBUILDER_MOUNTPOINT/pbuilder-run \n",
	   qemu_arch_diskdevice(pc->arch)
	   );
  
  
  create_script(pc->buildplace, 
		"pbuilder-run",
		s);
  free(s); s=0;

  ret=loop_umount(pc->buildplace);

  /* create the temporary device for command-execution */
  asprintf(&workblockdevicepath, "%s.dev", pc->buildplace);
  ret=create_ext3_block_device(workblockdevicepath, 1);

  loop_mount(workblockdevicepath, pc->buildplace);

  /* this is specific to my configuration, fix it later. */
  asprintf(&s,
	   "#!/bin/bash\n"
	   "export RET=0\n"
	   "echo \n"
	   "echo ' -> qemu-pbuilder second-stage' \n"
	   "/debootstrap/debootstrap --second-stage\n"
	   "echo deb %s %s main contrib non-free > /etc/apt/sources.list \n"
	   //TODO: copy hook scripts
	   "mount -n /proc /proc -t proc\n"
	   "dhclient eth0\n"
	   "cp $PBUILDER_MOUNTPOINT/hosts /etc/hosts\n"
	   "cp $PBUILDER_MOUNTPOINT/resolv.conf /etc/resolv.conf\n"
	   "cp $PBUILDER_MOUNTPOINT/hostname /etc/hostname\n"
	   "hostname pbuilder-$(cat /etc/hostname)\n"
	   //TODO: installaptlines
	   //"echo 'deb http://192.168.1.26/debian/ sid main ' > /etc/apt/sources.list\n"
	   //TODO: run G hook
	   "apt-get update\n"
	   //TODO: "dpkg --purge $REMOVEPACKAGES\n"
	   //recover aptcache
	   "apt-get -y --force-yes -o DPkg::Options::=--force-confnew dist-upgrade\n"
	   "apt-get install --force-yes -y build-essential dpkg-dev apt pbuilder\n"
	   //TODO: EXTRAPACKAGES handling
	   //save aptcache
	   //optionally autoclean aptcache
	   //run E hook
	   "apt-get clean || true\n"
	   "export RET=0"
	   ":EXIT"
	   "sync\n"
	   "sync\n"
	   "while sleep 3s; do\n"
	   "  echo \" -> qemu-pbuilder END OF WORK EXIT CODE=$RET\"\n"
	   "done\n"
	   "bash\n",
	   pc->mirror, 
	   pc->distribution);
  create_script(pc->buildplace,
		"pbuilder-run",
		s);

  /* TODO: can I do 'date --set' from output of 'LC_ALL=C date' */

  /* TODO: copy /etc/hosts etc. to inside chroot, or it won't know the hosts info. */
  copy_file_contents_to_temp("/etc/hosts", pc->buildplace, "hosts");
  copy_file_contents_to_temp("/etc/hostname", pc->buildplace, "hostname");
  copy_file_contents_to_temp("/etc/resolv.conf", pc->buildplace, "resolv.conf");

  /* do I not need to copy /etc/pbuilderrc, and ~/.pbuilderrc to inside chroot? */
  /* TODO: hooks probably need copying here. */
  /* TODO: recover aptcache */

  loop_umount(workblockdevicepath);
  rmdir(pc->buildplace);
  
  // this will have wrong time. how to workaround?

  fork_qemu(pc->basepath, workblockdevicepath, pc);
  
  unlink(workblockdevicepath);
  

 out:
  if(workblockdevicepath) 
    {
      free(workblockdevicepath);
    }
  if(s) free(s);
  return ret;

 umount_out:
  loop_umount(pc->buildplace);
  if(s) free(s);
  return ret;
}

/*
 * @return: return code of pbuilder, or <0 on failure
 */
int qemubuilder_build(const struct pbuilderconfig* pc, const char* dscfile)
{
  int ret;
  char* hoststr;
  char* hoststr2;
  char* commandline;

  const char* buildopt="--binary-all"; /* TODO: add --binary-arch option */
  
  hoststr=copy_dscfile(dscfile, pc->buildplace);

  asprintf(&commandline,
	   /* TODO: executehooks D: */
	   "/usr/lib/pbuilder/pbuilder-satisfydepends --control $PBUILDER_MOUNTPOINT/*.dsc --internal-chrootexec 'chroot . ' %s \n"
	   "cd $PBUILDER_MOUNTPOINT; /usr/bin/dpkg-source -x $(basename %s) \n"
	   "echo ' -> Building the package'\n"
	   /* TODO: executehooks A: */
	   "cd $PBUILDER_MOUNTPOINT/*-*/; dpkg-buildpackage -us -uc \n",
	   buildopt,
	   dscfile);

  /* Obscure assumption!: assume _ is significant for package name and
     no other file will have _. */

  asprintf(&hoststr2, 
	   "cp -p \"%s\"/*_* \"%s\" 2>/dev/null || true",
	   pc->buildplace, pc->buildresult);
  
  ret=run_second_stage_script
    (0,
     commandline, pc, 
     hoststr,
     hoststr2);

  if(hoststr2) free(hoststr2);
  if(hoststr) free(hoststr);
  if(commandline) free(commandline);
  return ret;
}

int qemubuilder_login(const struct pbuilderconfig* pc)
{
  return run_second_stage_script(pc->save_after_login, "bash", pc, NULL, NULL);
}

/* 

Mostly a copy of pbuilder login, executes a script.

 */
int qemubuilder_execute(const struct pbuilderconfig* pc, char** av)
{
  char* hostcommand;
  char* runcommandline;
  int ret;
  
  asprintf(&hostcommand, "cp %s %s/run\n", av[0], pc->buildplace);
  /* TODO: add options too */
  asprintf(&runcommandline, "sh $PBUILDER_MOUNTPOINT/run");
  ret=run_second_stage_script(pc->save_after_login, runcommandline, pc, 
			      hostcommand, NULL);
  free(hostcommand);
  free(runcommandline);
  return ret;
}

/**
   implement pbuilder update

   @return 0 on success, other values on failure.
 */
int qemubuilder_update(const struct pbuilderconfig* pc)
{
  return run_second_stage_script
    (1,	   
     //TODO: installaptlines if required.
     //TODO: "dpkg --purge $REMOVEPACKAGES\n"
     "apt-get update\n"
     "apt-get -y --force-yes -o DPkg::Options::=--force-confnew dist-upgrade\n"
     "apt-get install --force-yes -y build-essential dpkg-dev apt pbuilder\n"
     //TODO: EXTRAPACKAGES handling
     //optionally autoclean aptcache
     //run E hook
     , pc, 
     NULL, NULL);
}

int qemubuilder_help(void)
{
  printf("pbuilder [operation] [options]\n"
	 "operation:\n"
	 " --build\n"
	 " --create\n"
	 " --update\n"
	 " --login\n"
	 " --execute\n"
	 " --help\n"
	 "options:\n"
	 " --basepath:\n"
	 " --buildplace:\n"
	 " --distribution:\n"
	 " ... and other pbuilder options \n"
	 );
  return 0;
}

int load_config_file(const char* config, pbuilderconfig* pc)
{
  char *s;
  FILE* f;

  char* buf=NULL;
  size_t bufsiz=0;
  char* delim;

  asprintf(&s, "env - bash -c '. %s; set '", config);
  f=popen(s, "r");
  while (getline(&buf,&bufsiz,f)>0)
    {
      if (strrchr(buf,'\n'))
	{
	  *(strrchr(buf,'\n'))=0;
	}
      
      if ((delim=strchr(buf,'=')))
	{
	  /* assuming config entry */
	  *(delim++)=0;
	  if (!strcmp(buf, "MIRRORSITE"))
	    {
	      pc->mirror=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "BUILDRESULT"))
	    {
	      pc->buildresult=strdup(delim);
	    }
	  else if (!strcmp(buf, "DISTRIBUTION"))
	    {
	      pc->distribution=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "KERNEL_IMAGE"))
	    {
	      pc->kernel_image=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "INITRD"))
	    {
	      pc->initrd=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "MEMORY_MEGS"))
	    {
	      pc->memory_megs=atoi(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "ARCH"))
	    {
	      pc->arch=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	  else if (!strcmp(buf, "BASEPATH"))
	    {
	      pc->basepath=strdup(delim);
	      //printf("DEBUG: %s, %s\n", buf, delim);
	    }
	}
    }
  if(buf) free(buf);
  if(s) free(s);
  pclose(f);
  return 0;
}

int main(int ac, char** av)
{
  int c;			/* option */
  int index_point;
  char * cmdstr=NULL;
  
  static pbuilderconfig pc;

  static struct option long_options[]=
  {
    {"basepath", required_argument, 0, 'b'},
    {"buildplace", required_argument, 0, 'B'},
    {"mountproc", no_argument, &pc.mountproc, 1},
    {"mountdev", no_argument, &pc.mountdev, 1},
    {"mountdevpts", no_argument, &pc.mountdevpts, 1},
    {"nomountproc", no_argument, &pc.mountproc, 0},
    {"nomountdev", no_argument, &pc.mountdev, 0},
    {"nomountdevpts", no_argument, &pc.mountdevpts, 0},
    {"save-after-login", no_argument, &pc.save_after_login, 1},
    {"save-after-exec", no_argument, &pc.save_after_login, 1},
    {"build", no_argument, (int*)&pc.operation, pbuilder_build},
    {"create", no_argument, (int*)&pc.operation, pbuilder_create},
    {"update", no_argument, (int*)&pc.operation, pbuilder_update},
    {"login", no_argument, (int*)&pc.operation, pbuilder_login},
    {"execute", no_argument, (int*)&pc.operation, pbuilder_execute},
    {"help", no_argument, (int*)&pc.operation, pbuilder_help},
    {"version", no_argument, 0, 'v'},
    {"configfile", required_argument, 0, 'c'},
    {"mirror", required_argument, 0, 0},
    {"buildresult", required_argument, 0, 0},

    /* verbatim options, synced as of pbuilder 0.153 */
    {"othermirror", required_argument, 0, 'M'},
    {"http-proxy", required_argument, 0, 'M'},
    {"aptcache", required_argument, 0, 'M'},
    {"extrapackages", required_argument, 0, 'M'},
    {"hookdir", required_argument, 0, 'M'},
    {"debemail", required_argument, 0, 'M'},
    {"debbuildopts", required_argument, 0, 'M'},
    {"logfile", required_argument, 0, 'M'},
    {"aptconfdir", required_argument, 0, 'M'},
    {"timeout", required_argument, 0, 'M'},
    {"bindmounts", required_argument, 0, 'M'},
    {"debootstrapopts", required_argument, 0, 'M'},
    {"debootstrap", required_argument, 0, 'M'},
    {"distribution", required_argument, 0, 'M'},

    {"removepackages", no_argument, 0, 'm'},
    {"override-config", no_argument, 0, 'm'},
    {"pkgname-logfile", no_argument, 0, 'm'},
    {"binary-arch", no_argument, 0, 'm'},
    {"preserve-buildplace", no_argument, 0, 'm'},
    {"debug", no_argument, 0, 'm'},
    {"autocleanaptcache", no_argument, 0, 'm'},

    {0,0,0,0}
  };

  /* define pc to be clear. */
  memset (&pc, 0, sizeof(pbuilderconfig));
  /* default command-line component */
  pbuildercommandline[0]="pbuilder";

  load_config_file("/usr/share/pbuilder/pbuilderrc", &pc);
  load_config_file("/etc/pbuilderrc", &pc);
  load_config_file("~/.pbuilderrc", &pc);

  /* load config files here. */
  while((c = getopt_long (ac, av, "b:d:Mmhv", long_options, &index_point)) != -1)
    {
      switch (c)
	{
	case 'b':		/* basepath */
	  if (pc.operation == pbuilder_create)
	    {
	      if (mkdir(optarg, 0777)<0)
		{
		  perror("mkdir");
		  return 1;
		}
	    }
	  else if (!pc.operation)
	    {
	      fprintf(stderr, "need to specify operation before --basepath option\n");
	      return 1;
	    }
	  if (!(pc.basepath = canonicalize_file_name(optarg)))
	    {
	      fprintf(stderr, "cannot canonicalize filename %s, does not exist\n", optarg);
	      return 1;
	    }
	  break;
	case 'B':		/* buildplace */
	  pc.buildplace = strdup(optarg);
	  break;
	case 'c':		/* --config */
	  load_config_file(optarg, &pc);
	  break;
	case 'M':		/* pass through to pbuilder: duplicate with param */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PBUILDER_ADD_PARAM(cmdstr);
	  PBUILDER_ADD_PARAM(strdup(optarg));
	  break;
	case 'm':		/* pass through to pbuilder: duplicate without param */
	  if (0>asprintf(&cmdstr, "--%s", long_options[index_point].name))
	    {
	      /* error */
	      fprintf(stderr, "out of memory constructing command-line options\n");
	      exit (1);
	    }
	  PBUILDER_ADD_PARAM(cmdstr);
	  break;
	case 0:
	  /* other cases with long option with flags, this is expected
	     behavior, so ignore it, for most of the time.
	  */
	  
	  /* handle specific options which also give 0. */
	  if (!strcmp(long_options[index_point].name,"mirror"))
	    {
	      pc.mirror=strdup(optarg);
	    }
	  else if (!strcmp(long_options[index_point].name,"buildresult"))
	    {
	      pc.buildresult=strdup(optarg);
	    }
	  break;
	case 'h':		/* -h */
	case 'v':		/* -v --version */
	  pc.operation=pbuilder_help;
	  break;
	default:
	  fprintf(stderr, "Unhandled option\n");
	  /* Error case. */
	  return 1;
	}
    }

  /* set default values */
  if (!pc.basepath) 
    pc.basepath=strdup("/var/cache/pbuilder/base.qemu");
  if (!pc.buildplace)
    {
      mkdir("/var/cache/pbuilder/build",0777); /* create if it does not exist */
      asprintf(&(pc.buildplace), "/var/cache/pbuilder/build/qemu.%i", (int)getpid());
    }
  if (!pc.distribution)
    pc.distribution=strdup("sid");
  if (!pc.memory_megs)
    pc.memory_megs=128;

  switch(pc.operation)
    {
    case pbuilder_build:
      return qemubuilder_build(&pc, av[optind]);
      
    case pbuilder_create:
      return qemubuilder_create(&pc);
      
    case pbuilder_update:
      return qemubuilder_update(&pc);
      
    case pbuilder_login:
      return qemubuilder_login(&pc);

    case pbuilder_execute:
      return qemubuilder_execute(&pc, &av[optind]);

    case pbuilder_help:
      return qemubuilder_help();

    default:			
      fprintf (stderr, "E: No operation specified\n");
      return 1;
    }
  
  return 0;
}
