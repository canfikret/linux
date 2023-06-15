// https://cdn.kernel.org/pub/linux/utils/util-linux/v2.37/libmount-docs/libmount-Monitor.html
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <mntent.h>
#include <libmount/libmount.h>
#include <string.h>
#include <thread>
#include <chrono>
#include <string>

void run_azure_disk_encryption()
{
    bool unencrypted_ext4_mounted = false;
    FILE *fp = setmntent("/proc/self/mounts", "r");
    if (fp == NULL)
    {
        perror("Failed to open /proc/self/mounts");
        return;
    }

    struct mntent *mnt;
    while ((mnt = getmntent(fp)) != NULL)
    {
        if (strcmp(mnt->mnt_type, "ext4") == 0)
        {
            printf("Ext4 filesystem mounted: %s\n", mnt->mnt_fsname);

            // if mounted ext4 filesystem is not encrypted, set unencrypted_ext4_mounted to true
            // device name that doesn't contain mapper in the name and mnt_dir is not empty and not /boot
            std::string device_name(mnt->mnt_fsname);
            std::string mnt_dir(mnt->mnt_dir);
            if (device_name.find("mapper") == std::string::npos &&
                !mnt_dir.empty() &&
                mnt_dir != "/boot")
            {
                unencrypted_ext4_mounted = true;
                printf("Unencrypted ext4 filesystem %s mounted on %s\n", mnt->mnt_fsname, mnt->mnt_dir);
            }
        }
    }

    if (unencrypted_ext4_mounted)
    {
        printf("Mounted unencrypted ext4 filesystem detected. Calling ADE.\n");
        std::system("echo 'Calling ADE'");
    }
    else
    {
        printf("No mounted unencrypted ext4 filesystem detected. Skipping ADE call\n");
    }

    endmntent(fp);
}

int main(int argc, char *argv[])
{
    // Get libmount version and features
    const char *version = NULL;
    const char **features = NULL, **feature = NULL;
    mnt_get_library_version(&version);
    mnt_get_library_features(&features);
    printf(("%s, libmount %s"),
           argv[0],
           version);
    feature = features;
    while (feature && *feature)
    {
        fputs(feature == features ? ": " : ", ", stdout);
        fputs(*feature++, stdout);
    }
    fputs(")\n", stdout);

    const char *filename;
    struct libmnt_monitor *mn = mnt_new_monitor();
    struct libmnt_table *fstab = mnt_new_table();
    struct libmnt_iter *itr = NULL;

    mnt_monitor_enable_kernel(mn, true);

    printf("waiting for changes...\n");
    while (mnt_monitor_wait(mn, -1) > 0)
    {
        while (mnt_monitor_next_change(mn, &filename, NULL) == 0)
        {
            printf(" %s: change detected\n", filename);

            // if /etc/fstab is changed, check if there is any unencrypted ext4 filesystem mounted
            // and if there is, call ADE
            run_azure_disk_encryption();
        }
    }

    // Cleanup and free resources
    mnt_free_table(fstab);
    mnt_free_iter(itr);
    // mnt_unref_context(ctx);
    mnt_unref_monitor(mn);

    return 0;
}
