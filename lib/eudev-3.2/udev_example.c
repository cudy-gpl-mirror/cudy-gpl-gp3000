#include <libudev.h>
#include <stdio.h>

static struct udev_device*
get_child(
     struct udev* udev, struct udev_device* parent, const char* subsystem)
{
  struct udev_device* child = NULL;
  struct udev_enumerate *enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_parent(enumerate, parent);
  udev_enumerate_add_match_subsystem(enumerate, subsystem);
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char *path = udev_list_entry_get_name(entry);
    child = udev_device_new_from_syspath(udev, path);
    break;
  }

  udev_enumerate_unref(enumerate);
  return child;
}

static struct udev_device*
get_usb_child_devices(struct udev_device* parent, const char* subsystem, char* devPathStr)
{
  struct udev_enumerate *enumerate = udev_enumerate_new(udev_device_get_udev(parent));
  struct udev_device* dev = NULL;

  //udev_enumerate_add_match_parent(enumerate, parent);
  if(devPathStr==NULL || devPathStr == "")
  	return NULL;
  if(subsystem!="")
  	udev_enumerate_add_match_subsystem(enumerate, subsystem);
  udev_enumerate_scan_devices(enumerate);
  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;
  udev_list_entry_foreach(entry, devices) {
  		if(!strstr(udev_list_entry_get_name(entry), devPathStr)){
			continue;
  		}
		
//		printf("%s=%s\n", udev_list_entry_get_name(entry), udev_list_entry_get_value(entry));
		
		const char* path = udev_list_entry_get_name(entry);
    	dev = udev_device_new_from_syspath(udev_device_get_udev(parent), path);
#if 0
		struct udev_list_entry *properties = 0;
		struct udev_list_entry *property = 0;
		properties = udev_device_get_properties_list_entry(dev);
		//printf("                    =====================properties=========================\n");
		udev_list_entry_foreach(property, properties) {
			printf("                    %s=%s\n", udev_list_entry_get_name(property), udev_list_entry_get_value(property));
		}
		//printf("                    ==================================================\n");
#endif
		break;
  }
  
  return dev;
}

static struct udev_enumerate*
enumerate_usb_child_subsystem(struct udev_device* parent)
{
  struct udev_enumerate *enumerate = udev_enumerate_new(udev_device_get_udev(parent));

  //udev_enumerate_add_match_parent(enumerate, parent);
  udev_enumerate_scan_subsystems(enumerate);
  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;
  udev_list_entry_foreach(entry, devices) {
		printf("%s=%s\n", udev_list_entry_get_name(entry), udev_list_entry_get_value(entry));
  }
  
  return enumerate;
}

static void enumerate_usb_serial_device()
{
  struct udev* udev = udev_new();
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_subsystem(enumerate, "usb");
  udev_enumerate_add_match_property(enumerate, "DEVTYPE", "usb_device");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char* path = udev_list_entry_get_name(entry);
    struct udev_device* dev = udev_device_new_from_syspath(udev, path);

    //struct udev_device* block = get_child(udev, scsi, "block");
    //struct udev_device* scsi_disk = get_child(udev, scsi, "scsi_disk");

	if(udev_device_get_devnode(dev)){
		char iface[128] = {0};

		//printf("=====================devices=========================\n");
		sprintf(iface, "%s-%s/%s-%s:%s.", 
			    udev_device_get_sysattr_value(dev, "busnum"), 
				udev_device_get_sysattr_value(dev, "devpath"),
				udev_device_get_sysattr_value(dev, "busnum"), 
				udev_device_get_sysattr_value(dev, "devpath"),
				udev_device_get_sysattr_value(dev, "bNumConfigurations"));
		
		struct udev_device* child = get_usb_child_devices(dev, "tty", iface);
		if(!child) continue;
		//printf("=====================subsystems=========================\n");
		//enumerate_usb_child_subsystem(dev);

		printf("usb path: %s\n", path);
	    printf("block = %s, usb = %s:%s, manufacturer = %s, devpath= %s\n",
	          udev_device_get_devnode(dev),
	          udev_device_get_sysattr_value(dev, "idVendor"),
	          udev_device_get_sysattr_value(dev, "idProduct"),
	          udev_device_get_sysattr_value(dev, "manufacturer"),
	          udev_device_get_sysattr_value(dev, "devpath"));

		struct udev_list_entry *properties = 0;
		struct udev_list_entry *property = 0;
		properties = udev_device_get_properties_list_entry(dev);
		printf("=====================properties=========================\n");
		udev_list_entry_foreach(property, properties) {
			printf("%s=%s\n", udev_list_entry_get_name(property), udev_list_entry_get_value(property));
		}
		printf("\nchild properties:\n");
		properties = udev_device_get_properties_list_entry(child);
		udev_list_entry_foreach(property, properties) {
			printf("      %s=%s\n", udev_list_entry_get_name(property), udev_list_entry_get_value(property));
		}
		printf("==================================================\n");
		#if 0	
		int i=0;
		for(i=0; i < atoi(udev_device_get_sysattr_value(dev, "bNumConfigurations")); i++){
			sprintf(iface, "%s-%s:%s.%d", 
				udev_device_get_sysattr_value(dev, "busnum"), 
				udev_device_get_sysattr_value(dev, "devpath"),
				udev_device_get_sysattr_value(dev, "bNumConfigurations"),
				i);
			child = get_child_subsystem(udev, dev, iface);
			if(udev_device_get_devnode(child)){
			    printf("bInterfaceClass = %s, bInterfaceNumber = %s, bInterfaceProtocol = %s, bInterfaceSubClass=%s\n",
			          udev_device_get_sysattr_value(child, "bInterfaceClass"),
			          udev_device_get_sysattr_value(child, "bInterfaceNumber"),
			          udev_device_get_sysattr_value(child, "bInterfaceProtocol"),
			          udev_device_get_sysattr_value(child, "bInterfaceSubClass"));
			}
		}
		#endif
	}

    udev_device_unref(dev);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
}


static void enumerate_usb_mass_storage()
{
  struct udev* udev = udev_new();
  struct udev_enumerate* enumerate = udev_enumerate_new(udev);

  udev_enumerate_add_match_subsystem(enumerate, "scsi");
  udev_enumerate_add_match_property(enumerate, "DEVTYPE", "scsi_device");
  udev_enumerate_scan_devices(enumerate);

  struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
  struct udev_list_entry *entry;

  udev_list_entry_foreach(entry, devices) {
    const char* path = udev_list_entry_get_name(entry);
    struct udev_device* scsi = udev_device_new_from_syspath(udev, path);
	printf("usb path: %s\n", path);

    struct udev_device* block = get_child(udev, scsi, "block");
    struct udev_device* scsi_disk = get_child(udev, scsi, "scsi_disk");

    struct udev_device* usb
      = udev_device_get_parent_with_subsystem_devtype(
          scsi, "usb", "usb_device");

    if (block && scsi_disk && usb) {
        printf("block = %s, usb = %s:%s, scsi = %s\n",
          udev_device_get_devnode(block),
          udev_device_get_sysattr_value(usb, "idVendor"),
          udev_device_get_sysattr_value(usb, "idProduct"),
          udev_device_get_sysattr_value(scsi, "vendor"));
    }

    if (block)
      udev_device_unref(block);

    if (scsi_disk)
      udev_device_unref(scsi_disk);

    udev_device_unref(scsi);
  }

  udev_enumerate_unref(enumerate);
  udev_unref(udev);
}

int main()
{
  enumerate_usb_serial_device();
  enumerate_usb_mass_storage();
  return 0;
}
