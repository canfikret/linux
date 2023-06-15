#include <stdio.h>
#include <string.h>
#include <mntent.h>

int main() {
    FILE *fp = setmntent("/proc/self/mounts", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/self/mounts");
        return 1;
    }

    struct mntent *mnt;
    while ((mnt = getmntent(fp)) != NULL) {
        if (strcmp(mnt->mnt_type, "ext4") == 0) {
            printf("Ext4 filesystem mounted: %s\n", mnt->mnt_fsname);
            // Your custom code to handle the mounted ext4 filesystem goes here
        }
    }

    endmntent(fp);

    return 0;
}
