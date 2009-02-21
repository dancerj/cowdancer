/*BINFMTC: parameter.c forkexec.c qemuipsanitize.c qemuarch.c file.c
 *  qemubuilder: pbuilder with qemu
 *  Copyright (C) 2007-2009 Junichi Uekawa
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
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdarg.h>
#include <assert.h>
#include <termios.h>
#include <time.h>
#include <locale.h>
#include "parameter.h"
#include "qemuipsanitize.h"
#include "qemuarch.h"
#include "file.h"

/* 
 * example exit codes: 
 *
 * END OF WORK EXIT CODE=1
 * END OF WORK EXIT CODE=0
 * END OF WORK EXIT CODE=16
 */
const char* qemu_keyword="END OF WORK EXIT CODE=";

/** create a sparse ext3 block device suitable for 
    loop-mount.

   @returns -1 on error, 0 on success
 */
static int create_ext3_block_device(const char* filename, 
				    unsigned long int gigabyte)
{
  int ret = 0;
  char *s = NULL;
  char *s2 = NULL;

  /* create 10GB sparse data */
  if (create_sparse_file(filename,
			 gigabyte*1UL<<30UL))
    {
      ret=-1;
      goto out;
    }

  if ((ret=forkexeclp("mke2fs", "mke2fs", 
		      "-q",
		      "-F",
		      "-j", 
		      "-m1",
		      "-O", "sparse_super", 
		      filename, NULL)))
    {
      ret=-1;
      goto out;
    }

  if ((ret=forkexeclp("tune2fs", "tune2fs", 
		      "-c", "0", 
		      "-i", "0",
		      filename, NULL)))
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
  char *s = NULL;
  FILE* f = NULL;
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

/* minimally fix terminal I/O */
static void fix_terminal(void)
{
  struct termios t; 

  if (isatty(1)) 
    {
      tcgetattr(1, &t); 
      t.c_lflag |= ECHO;
      tcsetattr(1, TCSANOW, &t);
    }
}

static int copy_file_contents_to_temp(const char* orig, const char*temppath, const char* filename)
{
  char* s;
  int ret;

  asprintf(&s, "%s/%s", temppath, filename);
  ret=copy_file(orig, s);
  free(s);
  return ret;
}

/**
   run qemu until exit signal is received from within QEMU via serial
   console.

   exit code:
   -1: error
   0..X: return code from inside qemu
*/
static int fork_qemu(const char* hda, const char* hdb, const struct pbuilderconfig* pc)
{
  pid_t child;
  int sp[2];
  fd_set readfds;
  int exit_code=-1;
  const int buffer_size=4096;
  char* buf=malloc(buffer_size);
  size_t count;
  
  if (-1==socketpair(AF_UNIX, SOCK_STREAM,0,sp))
    {
      /* error handle? */
      return -1;
    }
  
  fflush(NULL);
  if ((child=fork()))
    {
      /* this is parent process */

      close(sp[1]);
      close(0);
      
      FD_ZERO(&readfds);
      while (1)
	{
	  FD_SET(sp[0],&readfds);
	  if (-1!=(select(sp[0]+1,&readfds, NULL, NULL, NULL)))
	    {
	      if (FD_ISSET(sp[0],&readfds))
		{
		  void* matchptr;

		  /* data available from qemu */
		  
		  /* sleep a bit to let it buffer-up a bit more. */
		  usleep(100000);
		  
		  count=read(sp[0],buf,buffer_size);
		  
		  /* this won't work sometimes, but this is a good best-effort thing. */
		  if ((matchptr=memmem(buf, count, 
				       qemu_keyword, strlen(qemu_keyword)))!=0)
		    {
		      int status;

		      exit_code = atoi(matchptr + strlen(qemu_keyword));
		      printf("\n  -> received termination message from inside qemu with exit-code %i, killing child process (qemu:%i)\n", 
			     exit_code,
			     child);
		      
		      assert(child != 0);assert(child > 0);

		      if (!kill(child, SIGTERM))
			printf("   -> successfully killed qemu\n");
		      else
			perror("   -> failed to kill qemu? :");
		      if (-1==waitpid(child, &status, 0))
			{
			  perror("qemubuilder: waitpid");
			}
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
      int argc = 0;
      const int MAX_ARGS = 30;
      char *argv[MAX_ARGS];
      int i;

      if (qemu == NULL || machine == NULL) {
	fprintf(stderr, "Your arch %s does not seem to be supported\n", pc->arch);
	exit(1);
      }

      asprintf(&mem, "%i", pc->memory_megs);
      
      asprintf(&append_command,
	       "root=/dev/%sa init=/pbuilder-run console=%s",
	       qemu_arch_diskdevice(pc),
	       qemu_arch_tty(pc->arch));
            
      dup2(sp[1],1);
      dup2(sp[1],2);
      close(sp[0]);

      argv[argc++]=strdupa(qemu);
      argv[argc++]="-nographic";
      argv[argc++]="-M";
      argv[argc++]=strdupa(machine);
      argv[argc++]="-m";
      argv[argc++]=mem;
      argv[argc++]="-kernel";
      argv[argc++]=strdupa(kernel_image);
      if (initrd && strcmp(initrd, ""))
	{
	  argv[argc++]="-initrd";
	  argv[argc++]=strdupa(initrd);
	}
      argv[argc++]="-hda";
      argv[argc++]=strdupa(hda);
      argv[argc++]="-hdb";
      argv[argc++]=strdupa(hdb);
      argv[argc++]="-append";
      argv[argc++]=append_command;
      argv[argc++]="-serial";
      argv[argc++]="stdio";
      argv[argc]=NULL;
      assert(argc < MAX_ARGS);

      printf("  forking qemu: ");
      for (i=0; i<argc; ++i) 
	{
	  printf("%s ", argv[i]);
	}
      printf("\n");
      
      execvp(argv[0], argv);
      perror("fork_qemu");
      exit (1);
    }
  else
    {
      perror("fork");
      return -1;
    }

  fix_terminal();
  return exit_code;
}

static int do_fsck(const char* devfile)
{
  /* force-running this fsck isn't a good idea; let it fail.
     If it's mounted by someone else, I don't want to touch it, 
     and chroots can be re-created any time, right?
   */
  return forkexeclp("/sbin/fsck",
		    "/sbin/fsck",
		    devfile, 
		    NULL);
}

/**
 * Invoke qemu, and run the second-stage script within QEMU.
 *
 * hostcommand1 is used from build and login and exeute
 */
static int run_second_stage_script
(
 /** save the result of this command*/
 int save_result,
 /** the command-line to invoke within QEMU */
 const char* commandline,
 const struct pbuilderconfig* pc,
 /** the commands to invoke in the host OS */
 const char* hostcommand1,
 /** the commands to invoke in the guest OS */
 const char* hostcommand2)
{
  char* script=NULL;
  char* workblockdevicepath=NULL;
  char* cowdevpath=NULL;
  char* locsave, *timestring;
  time_t currenttime;
  int ret=1;
  
  if (mkdir(pc->buildplace,0777))
    {
      /* could not create the buildplace here. */
      perror("mkdir");
      goto out;
    }

  do_fsck(pc->basepath);

  /* save/set/restore locale settings to get current time in POSIX format */
  locsave = setlocale(LC_TIME, NULL);
  (void) setlocale(LC_TIME, "POSIX");
  currenttime=time(NULL); 
  timestring=asctime(gmtime(&currenttime));
  (void) setlocale(LC_TIME, locsave);

  asprintf(&workblockdevicepath, "%s.dev", pc->buildplace);
  ret=create_ext3_block_device(workblockdevicepath, 1);
  loop_mount(workblockdevicepath, pc->buildplace);

  asprintf(&script, 
	   "#!/bin/bash\n"

	   /* define function to terminate qemu */
	   "function exit_from_qemu() {\n"
	   "sync\n"
	   "sync\n"
	   "sleep 1s\n"		/* sleep before sending dying message */
	   "echo ' -> qemu-pbuilder %s$1'\n"
	   "sleep 1s\n"
	   "halt -f -p\n"	/* just halt myself if possible */
	   "}\n"

	   /* main code */
	   "echo \n"
	   "echo ' -> qemu-pbuilder second-stage' \n"
	   //TODO: copy hook scripts
	   //"mount -n /proc /proc -t proc\n" // this is done in first stage.
	   "echo '  -> setting time to %s' \n"
	   "date --set=\"%s\"\n"
	   "echo '  -> configuring network' \n"
	   "ifconfig -a\n"
	   "export IFNAME=`/sbin/ifconfig -a | grep eth | head -n1 | awk '{print $1}'`\n"
	   "dhclient $IFNAME\n"
	   "cp $PBUILDER_MOUNTPOINT/hosts /etc/hosts\n"
	   "cp $PBUILDER_MOUNTPOINT/resolv.conf /etc/resolv.conf\n"
	   "cp $PBUILDER_MOUNTPOINT/hostname /etc/hostname\n"
	   "hostname pbuilder-$(cat /etc/hostname)\n"
	   //TODO: run G hook
	   "%s\n"
	   //TODO: I can mount /var/cache/apt/archives from some scratch space to not need this:
	   "apt-get clean || true\n"
	   "exit_from_qemu 0\n",
	   qemu_keyword,
	   timestring,
	   timestring,
	   commandline);

  create_script(pc->buildplace, "pbuilder-run",
		script);

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

  fork_qemu(cowdevpath, workblockdevicepath, pc);
  /* this will always return 0. */

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
  char* buf=NULL;
  char* filename=NULL;
  char* origdir=NULL;
  char* dscfile=canonicalize_file_name(dscfile_);
  FILE* f=fopen(dscfile,"r");

  char* memstr=0;
  size_t len=0;
  FILE* fmem=open_memstream(&memstr, &len);
  int filelist=0;

  origdir=strdup(dscfile);

  assert(strrchr(origdir,'/') != 0);
  (*(strrchr(origdir,'/')))=0;
    
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
	    filelist=0;
	  else
	    {
	      fprintf(fmem, "cp %s/%s %s/\n", 
		      origdir, filename, destdir);
	      assert(filename);
	      free(filename);
	    }
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

  if(buf) free(buf);
  if(origdir) free(origdir);
  if(dscfile) free(dscfile);
  return ret?NULL:memstr;
}

/* 
   return 0 on success, nonzero on failure.

   variable ret holds the state.
 */
int cpbuilder_create(const struct pbuilderconfig* pc)
{
  int ret=0;
  char* s=NULL;  		/* generic asprintf buffer */
  char* workblockdevicepath=NULL;
  char* t=NULL;

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
      ret=1;
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
      fprintf(stderr, "debootstrap failed with %i\n", 
	      ret);
      goto umount_out;
    }

  /* arch-dependent code here.
     create required device files.

     ttyAMA0 is probably ARM-specific 
     others are probably linux-portable as documented in linux/Documentation/devices.txt
     other OSes will require different, but hey, they probably don't even boot from ext3, 
     we'll need think of other ways to work with them.

   */
  printf(" -> Doing arch-specific /dev population\n");

  qemu_create_arch_devices(pc->buildplace, pc->arch);
  
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
	   qemu_arch_diskdevice(pc)
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

  asprintf(&s,
	   "#!/bin/bash\n"
	   /* define function to terminate qemu */
	   "function exit_from_qemu() {\n"
	   "sync\n"
	   "sync\n"
	   "sleep 1s\n"		/* sleep before sending dying message */
	   "echo ' -> qemu-pbuilder %s$1'\n"
	   "sleep 1s\n"
	   "halt -f -p\n"	/* just halt myself if possible */
	   "}\n"

	   /* start of main code */
	   "export RET=0\n"
	   "echo \n"
	   "echo ' -> qemu-pbuilder second-stage' \n"
	   "/debootstrap/debootstrap --second-stage\n"
	   "echo deb %s %s %s > /etc/apt/sources.list \n"
	   "echo 'APT::Install-Recommends \"false\"; ' > /etc/apt/apt.conf.d/15pbuilder\n"
	   //TODO: copy hook scripts
	   "mount -n proc /proc -t proc\n"
	   "mount -n sysfs /sys -t sysfs\n"
	   "mkdir /dev/pts\n"
	   "mount -n devpts /dev/pts -t devpts\n"
	   "dhclient eth0\n"
	   "cp $PBUILDER_MOUNTPOINT/hosts /etc/hosts\n"
	   "cp $PBUILDER_MOUNTPOINT/resolv.conf /etc/resolv.conf\n"
	   "cp $PBUILDER_MOUNTPOINT/hostname /etc/hostname\n"
	   "hostname pbuilder-$(cat /etc/hostname)\n"
	   //TODO: installaptlines
	   //"echo 'deb http://192.168.1.26/debian/ sid main ' > /etc/apt/sources.list\n"
	   //TODO: run G hook
	   "apt-get update || exit_from_qemu 1\n"
	   //TODO: "dpkg --purge $REMOVEPACKAGES\n"
	   //recover aptcache
	   "apt-get -y --force-yes -o DPkg::Options::=--force-confnew dist-upgrade || exit_from_qemu 1\n"
	   "apt-get install --force-yes -y build-essential dpkg-dev apt aptitude pbuilder || exit_from_qemu 1\n"
	   //TODO: EXTRAPACKAGES handling
	   //save aptcache
	   //optionally autoclean aptcache
	   //run E hook
	   //TODO: I can mount /var/cache/apt/archives from some scratch space to not need this:
	   "apt-get clean || true\n"
	   "exit_from_qemu $RET\n"
	   "bash\n",
	   qemu_keyword,
	   t=sanitize_mirror(pc->mirror), 
	   pc->distribution,
	   pc->components);
  free(t);
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
  ret=fork_qemu(pc->basepath, workblockdevicepath, pc);
  
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
int cpbuilder_build(const struct pbuilderconfig* pc, const char* dscfile)
{
  int ret;
  char* hoststr=NULL;
  char* hoststr2=NULL;
  char* commandline=NULL;

  const char* buildopt="--binary-all"; /* TODO: add --binary-arch option */
  
  hoststr=copy_dscfile(dscfile, pc->buildplace);

  asprintf(&commandline,
	   /* TODO: executehooks D: */
	   "/usr/lib/pbuilder/pbuilder-satisfydepends --control $PBUILDER_MOUNTPOINT/*.dsc --internal-chrootexec 'chroot . ' %s \n"
	   "cd $PBUILDER_MOUNTPOINT; /usr/bin/dpkg-source -x $(basename %s) \n"
	   "echo ' -> Building the package'\n"
	   /* TODO: executehooks A: */
	   "cd $PBUILDER_MOUNTPOINT/*-*/; dpkg-buildpackage -us -uc %s\n",
	   buildopt,
	   dscfile,
	   pc->debbuildopts);

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

int cpbuilder_login(const struct pbuilderconfig* pc)
{
  return run_second_stage_script(pc->save_after_login, 
				 "bash", 
				 pc, 
				 NULL, 
				 NULL);
}

/* 

Mostly a copy of pbuilder login, executes a script.

 */
int cpbuilder_execute(const struct pbuilderconfig* pc, char** av)
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
int cpbuilder_update(const struct pbuilderconfig* pc)
{
  /* TODO: --override-config */
  return run_second_stage_script
    (1,	   
     //TODO: installaptlines if required.
     //TODO: "dpkg --purge $REMOVEPACKAGES\n"
     //TODO: add error code handling.
     "apt-get update -o Acquire::PDiffs=false\n"
     "apt-get -y --force-yes -o DPkg::Options::=--force-confnew dist-upgrade\n"
     "apt-get install --force-yes -y build-essential dpkg-dev apt aptitude pbuilder\n"
     //TODO: EXTRAPACKAGES handling
     //optionally autoclean aptcache
     //run E hook
     , pc, 
     NULL, NULL);
}

int cpbuilder_help(void)
{
  printf("qemubuilder [operation] [options]\n"
	 "operation:\n"
	 " --build\n"
	 " --create\n"
	 " --update\n"
	 " --login\n"
	 " --execute\n"
	 " --help\n"
	 " --dumpconfig\n"
	 "options:\n"
	 " --basepath:\n"
	 " --buildplace:\n"
	 " --distribution:\n"
	 " ... and other pbuilder options \n"
	 );
  return 0;
}

int main(int ac, char** av)
{
  return parse_parameter(ac, av, "qemu");
}
