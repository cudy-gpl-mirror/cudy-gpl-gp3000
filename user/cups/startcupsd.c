/* 
USB Printer Handler
tsaitc 2012/08/01

handles linux 2.6 hotplug event for USB Printers
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <config/autoconf.h>
#include <fcntl.h>
#include <rtk/options.h>
#include <libgen.h>
#include <assert.h>
#include <err.h>
#include <glob.h>
#include <sys/inotify.h>
#include <errno.h>
#include <linux/version.h>

/*
Environment Variables:
DEVPATH
ACTION
*/

#define BUFFER_SIZE 32
#define STARTCUPSD_DEBUG 

static const char CUPSDPID[] = "/var/run/cupsd.pid";

struct action_table {
	char *action_str;
	int (*action_func)(int, char **, const char *);
};

#define PrinterNameFile		"/tmp/printername"

static int addPrinterName(char *devpath, char *printername)
{
	FILE *fp, *fptmp;
	char *tp;
	int i;
	//devpath: /devices/platform/rtl8672-xhci.0/usb1/1-1/1-1:1.0/usb/lp0
	
	char *bname = basename(devpath);// lp0/lp1/...
	char usbpath[100], tmpStr[100], buff[1024];
	char manufacturer[100], product[100];

	memset(usbpath, 0, sizeof(usbpath));
	memset(tmpStr, 0, sizeof(tmpStr));
	memset(manufacturer, 0, sizeof(manufacturer));
	memset(product, 0, sizeof(product));
	
	if(tp=strstr(devpath,"xhci-hcd")){
		for(i=0;i<2;i++){
			tp=strstr(tp,"/");
			if(tp)
				tp+=1;
			else
				break;
		}
		if(i==2){
			strcpy(tmpStr, tp);
			tp = strstr(tmpStr, "/");
			*tp = '\0';
		}
	}

	sprintf(usbpath, "/sys/bus/usb/devices/%s/manufacturer", tmpStr);
	fp=fopen(usbpath,"r");
	if(fp)
	{
		fscanf(fp,"%s", manufacturer);
		for (i=0; manufacturer[i] != '\0'; i++) {
			if (manufacturer[i] == ' ')
				manufacturer[i] = '_';
			else if (manufacturer[i] == '\n') {
				manufacturer[i] = '\0';
				break;
			}
		}
		
		close(fp);
	}
	else {
		warnx("can not open %s\n", usbpath);
		goto err_exit;
	}

	sprintf(usbpath, "/sys/bus/usb/devices/%s/product", tmpStr);
	fp=fopen(usbpath,"r");
	if(fp)
	{
		fgets(product, sizeof(product), fp);
		for (i=0; product[i] != '\0'; i++) {
			if (product[i] == ' ')
				product[i] = '_';
			else if (product[i] == '\n') {
				product[i] = '\0';
				break;
			}
		}
		
		close(fp);
	}
	else {
		warnx("can not open %s\n", usbpath);
		goto err_exit;
	}

	//sprintf(printername, "%s_%s_%c", manufacturer, product, tmpStr[0]);
	sprintf(printername, "%s_%s", manufacturer, product);
	//warnx("%s printername %s\n", __func__, printername);

	//record printername to /tmp/printername
	fp = fopen(PrinterNameFile, "r");
	fptmp = fopen("/tmp/tmpfile.printer", "w+");
	if (!fptmp) {
		warnx("open /tmp/tmpfile.printer fail.\n");
		goto err_exit;
	}
	if (fp)
	{
		char tmpDev[100], tmpPrinter[100];
		
		while ( fgets(buff, sizeof(buff), fp) != NULL )
		{
			if (sscanf(buff, "%s %s", tmpDev, tmpPrinter) != 2)
				continue;
			if (!strcmp(tmpDev, bname))
				continue;
			if (!strcmp(tmpPrinter, printername)) {
				//a same printer has attached to RG.
				sprintf(printername, "%s_1", printername);
			}
			fprintf(fptmp, "%s %s\n", tmpDev, tmpPrinter);
		}
		fclose(fp);
	}
	fprintf(fptmp, "%s %s\n", bname, printername);

	rewind(fptmp);
	fp = fopen(PrinterNameFile, "w");
	if (fp)
	{
		while ( fgets(buff, sizeof(buff), fptmp) != NULL )
		{
			fprintf(fp, "%s\n", buff);
		}
	}
	fclose(fp);
	fclose(fptmp);

	unlink("/tmp/tmpfile.printer");
	return 1;
	
err_exit:
	return 0;
}

static int remPrinterName(char *devpath, char *printername)
{
	FILE *fp, *fptmp;
	int i;
	//devpath: /devices/platform/rtl8672-xhci.0/usb1/1-1/1-1:1.0/usb/lp0
	
	char *bname = basename(devpath);// lp0/lp1/...
	char usbpath[100], tmpStr[100], buff[1024];
	char manufacturer[100], product[100];
	char tmpDev[100], tmpPrinter[100];

	memset(usbpath, 0, sizeof(usbpath));
	memset(tmpStr, 0, sizeof(tmpStr));
	memset(manufacturer, 0, sizeof(manufacturer));
	memset(product, 0, sizeof(product));
	
	fp = fopen(PrinterNameFile, "r");
	fptmp = fopen("/tmp/tmpfile", "w+");
	if (!fptmp) {
		warnx("open /tmp/tmpfile fail.\n");
		goto err_exit;
	}
	if (!fp) {
		warnx("open %s fail.\n", PrinterNameFile);
		goto err_exit;
	}
	
	while ( fgets(buff, sizeof(buff), fp) != NULL )
	{
		if (sscanf(buff, "%s %s", tmpDev, tmpPrinter) != 2)
			continue;
		if (strcmp(tmpDev, bname)) {
			fprintf(fptmp, "%s %s\n", tmpDev, tmpPrinter);
		}
		else
			strcpy(printername, tmpPrinter);
	}
	fclose(fp);

	rewind(fptmp);
	fp = fopen(PrinterNameFile, "w");
	if (fp)
	{
		while ( fgets(buff, sizeof(buff), fptmp) != NULL )
		{
			fprintf(fp, "%s\n", buff);
		}
	}
	fclose(fp);
	fclose(fptmp);

	unlink("/tmp/tmpfile");
	return 1;
	
err_exit:
	return 0;
}


static int action_add(int argc, char **argv, char *devpath) 
{
	int ret, retry_count;
	pid_t pid;

	const char * const bname = basename(devpath);
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 30)
	char uri[BUFFER_SIZE] = "parallel:/dev/usb/";
#else
	char uri[BUFFER_SIZE] = "parallel:/dev/";
#endif
	char printername[100];

#ifdef STARTCUPSD_DEBUG
	warnx("action add: %s", devpath);
#endif

	pid = read_pid((char *)CUPSDPID);
	if (pid <= 0) {
		/* cupsd is not running */

		/* /var/cups/conf/cupsd.conf must exist before executing cupsd */
		for (retry_count = 0; retry_count < 3 
				&& access("/var/cups/conf/cupsd.conf", F_OK); retry_count++) {
			/* cupsd.conf does not exist */
#ifdef STARTCUPSD_DEBUG
			warn("%s:%d, errno %d", __FUNCTION__, __LINE__, errno);
#endif
			sleep(1 << retry_count);
		}

#ifdef CONFIG_USER_CUPS_1_4_6
		va_cmd("/bin/cupsd", 1, 0, "-f");
#else
		va_cmd("/bin/cupsd", 0, 0);
#endif
		system("iptables -A INPUT -p tcp --dport 631 -j ACCEPT");	//add accept rule when launch cupsd
		system("ip6tables -A INPUT -p tcp --dport 631 -j ACCEPT");	//add accept rule when launch cupsd
	}

	for (retry_count = 0; retry_count < 3 
			&& access("/var/run/cupsd.pid", F_OK); retry_count++) {
		/* cupsd.pid does not exist */
#ifdef STARTCUPSD_DEBUG
		warn("%s:%d, errno %d", __FUNCTION__, __LINE__, errno);
#endif
		sleep(1 << retry_count);
	}

	strcat(uri, bname);

	if (!addPrinterName(devpath, printername))
		strcpy(printername, bname);

	/* lpadmin -p lp0 -v parallel:/dev/lp0 -m Generic.ppd -E */
	//ret = va_cmd("/bin/lpadmin", 7, 0, "-p", printername, "-v", uri, "-m", "Generic.ppd", "-E");
	/* lpadmin -p lp0 -v parallel:/dev/lp0 -E */
	ret = va_cmd("/bin/lpadmin", 5, 0, "-p", printername, "-v", uri, "-E");

	return ret;
}

static int action_remove(int argc, char **argv, char *devpath) 
{
	int ret;
	glob_t globbuf;
	pid_t pid;

	const char * const bname = basename(devpath);
	char printername[100];

#ifdef STARTCUPSD_DEBUG
	warnx("action remove: %s", devpath);
#endif

	// remove printer only if cupsd exist
	pid = read_pid((char *)CUPSDPID);
	if (pid > 0) {
		if (!remPrinterName(devpath, printername))
			strcpy(printername, bname);

		/* lpadmin -x lp0 */
		ret = va_cmd("/bin/lpadmin", 2, 1, "-x", printername);
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 30)
	glob("/dev/usb/*lp*", 0, NULL, &globbuf);
#else
	glob("/dev/*lp*", 0, NULL, &globbuf);
#endif

	/* the last USB Printer will be removed */
	if (globbuf.gl_pathc <= 1) {
		pid = read_pid((char *)CUPSDPID);
		if (pid > 0) {
			/* cupsd is running */
			kill(pid, 9);
			unlink(CUPSDPID);
			system("iptables -D INPUT -p tcp --dport 631 -j ACCEPT");	//remove accept rule when exit cupsd
			system("ip6tables -D INPUT -p tcp --dport 631 -j ACCEPT");	//remove accept rule when exit cupsd
		}
	}

	globfree(&globbuf);

	return ret;
}

static const struct action_table hotplug_table[] = {
	{ "add", action_add },
	{ "remove", action_remove },	
	{ NULL, NULL },
};

int main(int argc, char **argv)
{
	const char *devpath;
	const char *action;
	const struct action_table *p;

#ifdef STARTCUPSD_DEBUG
	int fd;

	fd = open("/dev/console", O_WRONLY);
	dup2(fd, STDERR_FILENO);
	close(fd);
#endif

#if defined(CONFIG_E8B)
	char tmpBuf[256];
	int cupsEnable = 0;
	
	FILE *fp = popen("/etc/scripts/flash get CUPS_ENABLE | awk -F'=' '{print $2}'", "r");
	memset(tmpBuf, 0, sizeof(tmpBuf));
	while((fgets(tmpBuf, 256, fp)) != NULL) {
#ifdef STARTCUPSD_DEBUG
		warnx("CUPS_ENABLE %s", tmpBuf);
#endif
	}
	pclose(fp);
	cupsEnable = atoi(tmpBuf);
	if(cupsEnable==0){
		sleep(1);
		exit (0);
	}
#endif

	devpath = getenv("DEVPATH");
	assert(devpath != NULL);
	
	action = getenv("ACTION");
	assert(action != NULL);

	for (p = &hotplug_table[0]; p->action_str; p++) {
		if (strcmp(action, p->action_str) == 0) {
			return p->action_func(argc - 1, &argv[1], devpath);
		}
	}

	return 0;
}

