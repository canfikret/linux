#include <stdio.h>
#include <string.h>
#include <mntent.h>
#include <cstdlib>
#include <thread>
#include <string>
#include <chrono>

bool thread_running = true;

void thread_proc()
{
    do
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
            printf("Mounted unencrypted ext4 filesystem detected. Call ADE.\n");
        }

        endmntent(fp);

        // Sleep for 1 minute until next iteration.
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
    while (thread_running);
}

int main()
{
    std::atexit([]() { thread_running = false; });

    std::thread t(thread_proc);
    t.join();

    return 0;
}
