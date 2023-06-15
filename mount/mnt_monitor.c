// https://cdn.kernel.org/pub/linux/utils/util-linux/v2.37/libmount-docs/libmount-Monitor.html
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <mntent.h>
#include <libmount/libmount.h>

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

            // Find if the change is for a mount point

            mnt_table_parse_fstab(fstab, filename);
            if (fstab == NULL)
            {
                fprintf(stderr, "Failed to parse fstab file: %s\n", filename);
                continue;
            }

            int ret = mnt_table_set_iter(fstab, itr, MNT_ITER_FORWARD);
            if (ret != 0)
            {
                fprintf(stderr, "Failed to set iterator for fstab file: %s\n, errno=%d", filename, ret);
                continue;
            }

            if (itr == NULL)
            {
                fprintf(stderr, "Failed to get iterator for fstab file: %s\n", filename);
                continue;
            }

            struct libmnt_fs *fs;
            while (mnt_table_next_fs(fstab, itr, &fs) == 0)
            {
                if (mnt_fs_get_source(fs) == NULL)
                {
                    fprintf(stderr, "Failed to get source for fstab file: %s\n", filename);
                    continue;
                }

                printf(" %s: %s\n", filename, mnt_fs_get_source(fs));
            }
        }
    }

    // Cleanup and free resources
    mnt_free_table(fstab);
    mnt_free_iter(itr);
    // mnt_unref_context(ctx);
    mnt_unref_monitor(mn);

    return 0;
}
