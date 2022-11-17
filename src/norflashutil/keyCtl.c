#include <keyutils.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <stdio.h>

#define INFN_KEY_START "<Key_data_start>"
#define INFN_KEY_END "<Key_data_end>"
#define INFINERA_KEY_TYPE "infinera_assymetric"
#define KEYNAME_USER_SPACE "infinera_"

key_serial_t add_key(const char* type,
                     const char* description,
                     const void* payload,
                     size_t plen,
                     key_serial_t ringid)
{
    return syscall(__NR_add_key,
                   type, description, payload, plen, ringid);
}

long revoke_key(key_serial_t id)
{
    return keyctl(KEYCTL_REVOKE, id);
}

int infn_keyctl_add(const char* iskbuff, const char* keyName, char* errorMsg)
{
    key_serial_t dest;
    int ret = -1;

    dest = find_key_by_type_and_desc("keyring", ".infinera_builtin_trusted_keys", 0);

    if(dest == -1)
    {
        strcpy(errorMsg, "Keyring .infinera_builtin_trusted_keys not found");
        return ret;
    }

    const char* start = strstr(iskbuff, INFN_KEY_START);
    const char* end = strstr(start, INFN_KEY_END);

    size_t len = (end - start + strlen(INFN_KEY_END) + 1);

    key_serial_t K = find_key_by_type_and_desc(KEYNAME_USER_SPACE, keyName, dest);

    if(K == -1)
    {
        ret = add_key(INFINERA_KEY_TYPE, keyName, start, len, dest);

        if(ret < 0)
        {
            strcpy(errorMsg, "Failed to add key to keyring");
        }
    }
    else
    {
        strcpy(errorMsg, "Key is already present in keyring");
    }

    return ret;
}

int infn_keyctl_revoke(const char* keyName, char* errorMsg)
{
    key_serial_t ring;
    key_serial_t dest;
    int ret = -1;

    ring = find_key_by_type_and_desc("keyring", ".infinera_builtin_trusted_keys", 0);

    if(ring == -1)
    {
        strcpy(errorMsg, "Keyring .infinera_builtin_trusted_keys not found");
        return ret;
    }

    dest = find_key_by_type_and_desc(KEYNAME_USER_SPACE, keyName, ring);

    if(dest == -1)
    {
        strcpy(errorMsg, "Key not found");
        return ret;
    }

    ret = revoke_key(dest);

    if(ret  == -1)
    {
        strcpy(errorMsg, "Key is IN USE");
    }

    return ret;
}